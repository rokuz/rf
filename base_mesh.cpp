#include "base_mesh.hpp"
#include "mesh_generator.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>

#include "rf.hpp"

namespace rf
{
void ForEachAttribute(uint32_t componentsMask,
                      std::function<void(MeshVertexAttribute)> const & func)
{
  for (uint32_t i = 0; i < kAttributesCount; i++)
  {
    if ((componentsMask & kAllAttributes[i]) != 0)
      func(kAllAttributes[i]);
  }
}

void ForEachAttributeWithCheck(uint32_t componentsMask,
                               std::function<bool(MeshVertexAttribute)> const & func)
{
  for (uint32_t i = 0; i < kAttributesCount; i++)
  {
    if ((componentsMask & kAllAttributes[i]) != 0)
    {
      if (!func(kAllAttributes[i]))
        return;
    }
  }
}

namespace
{
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct BoneData
{
  float data[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

uint32_t GetAttributeElements(MeshVertexAttribute attr)
{
  switch (attr)
  {
    case Position:
    case Normal:
    case Tangent:
      return 3;
    case UV0:
    case UV1:
    case UV2:
    case UV3:
      return 2;
    case Color:
    case BoneIndices:
    case BoneWeights:
      return 4;
  }
  CHECK(false, "Unknown vertex attribute.");
  return 0;
}

class MeshLogStream : public Assimp::LogStream
{
public:
  explicit MeshLogStream(std::string const & filename) : m_filename(filename)
  {}

  void write(char const * message) override
  {
    Logger::ToLogWithFormat(Logger::Error, "Assimp: (%s): %s", m_filename.c_str(), message);
  }

private:
  std::string m_filename;
};

class MeshLogGuard
{
public:
  explicit MeshLogGuard(std::string const & filename)
  {
    using namespace Assimp;
    m_stream = std::make_unique<MeshLogStream>(filename);
    DefaultLogger::create("assimp.log", Assimp::Logger::NORMAL);
    DefaultLogger::get()->attachStream(m_stream.get(), Assimp::Logger::Err);
  }

  ~MeshLogGuard()
  {
    using namespace Assimp;
    DefaultLogger::get()->detatchStream(m_stream.get());
    DefaultLogger::kill();
  }

private:
  std::unique_ptr<MeshLogStream> m_stream;
};

MaterialColor MakeColor(aiColor3D const & color)
{
  return glm::vec3(color.r, color.g, color.b);
}

glm::mat4x4 GetMatrix44(aiMatrix4x4 const & m)
{
  return glm::mat4x4(m.a1, m.b1, m.c1, m.d1,
                     m.a2, m.b2, m.c2, m.d2,
                     m.a3, m.b3, m.c3, m.d3,
                     m.a4, m.b4, m.c4, m.d4);
}

glm::vec3 GetVector3(aiVector3D const & v)
{
  return glm::vec3(v.x, v.y, v.z);
}

glm::quat GetQuaternion(aiQuaternion const & q)
{
  return glm::quat(q.w, q.x, q.y, q.z);
}

uint32_t GetAttributeSize(MeshVertexAttribute attr)
{
  return GetAttributeElements(attr) * sizeof(float);
}

void CopyVertexBuffer(BaseMesh::MeshGroup & group, MeshVertexAttribute attr, void const * data,
                      uint32_t numVertices)
{
  auto const sizeInBytes = numVertices * GetAttributeSize(attr);
  group.m_vertexBuffers[attr].resize(sizeInBytes);
  memcpy(group.m_vertexBuffers[attr].data(), data, sizeInBytes);
}

template <typename TData>
void CopyVertexBufferPartially(BaseMesh::MeshGroup & group, MeshVertexAttribute attr,
                               TData const * data, uint32_t numVertices)
{
  uint32_t const attrSize = GetAttributeSize(attr);
  ASSERT(attrSize <= sizeof(TData), "Invalid data size");

  size_t const sizeInBytes = numVertices * attrSize;
  group.m_vertexBuffers[attr].resize(sizeInBytes);
  ByteArray & buffer = group.m_vertexBuffers[attr];
  uint32_t offset = 0;
  for (uint32_t i = 0; i < numVertices; i++)
  {
    memcpy(buffer.data() + offset, data + i, attrSize);
    offset += attrSize;
  }
}

void LoadNode(std::unique_ptr<BaseMesh::MeshNode> & meshNode, aiScene const * scene,
              aiNode const * node, uint32_t & componentsMask, uint32_t & verticesCount,
              uint32_t & indicesCount, int & groupIndex, BoneIndicesCollection & bonesIndices)
{
  meshNode->m_name = std::string(node->mName.C_Str());
  meshNode->m_transform = GetMatrix44(node->mTransformation);
  meshNode->m_groups.reserve(node->mNumMeshes);
  for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; meshIndex++)
  {
    aiMesh const * mesh = scene->mMeshes[node->mMeshes[meshIndex]];
    uint32_t const numVertices = mesh->mNumVertices;

    BaseMesh::MeshGroup group;
    if (mesh->HasPositions())
      CopyVertexBuffer(group, MeshVertexAttribute::Position, mesh->mVertices, numVertices);

    if (mesh->HasNormals())
      CopyVertexBuffer(group, MeshVertexAttribute::Normal, mesh->mNormals, numVertices);

    if (mesh->HasTangentsAndBitangents())
      CopyVertexBuffer(group, MeshVertexAttribute::Tangent, mesh->mTangents, numVertices);

    if (mesh->HasVertexColors(0))
      CopyVertexBuffer(group, MeshVertexAttribute::Color, mesh->mColors[0], numVertices);

    MeshVertexAttribute uvs[] = {UV0, UV1, UV2, UV3};
    for (uint32_t i = 0; i < 4; i++)
    {
      if (mesh->HasTextureCoords(i))
        CopyVertexBufferPartially(group, uvs[i], mesh->mTextureCoords[i], numVertices);
    }

    if (mesh->HasBones())
    {
      std::vector<uint8_t> bonesUsage(numVertices, 0);
      std::vector<BoneData> boneWeights(numVertices);
      std::vector<BoneData> boneIndices(numVertices);
      for (uint32_t i = 0; i < mesh->mNumBones; i++)
      {
        aiBone * bone = mesh->mBones[i];
        std::string boneName = std::string(bone->mName.C_Str());

        // create new bone index
        if (bonesIndices.find(boneName) == bonesIndices.end())
        {
          auto const newBoneIndex = static_cast<uint32_t>(bonesIndices.size());
          if (newBoneIndex >= kMaxBonesNumber)
          {
            Logger::ToLogWithFormat(Logger::Warning, "Maximum number of bones (%d) is exceeded.",
                                    kMaxBonesNumber);
            continue;
          }
          bonesIndices.insert(std::make_pair(boneName, newBoneIndex));
        }

        uint32_t boneIndex = bonesIndices[boneName];
        group.m_boneOffsets.insert(std::make_pair(boneIndex, GetMatrix44(bone->mOffsetMatrix)));
        for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++)
        {
          uint32_t vertexId = bone->mWeights[weightIndex].mVertexId;
          ASSERT(vertexId < numVertices, "");

          uint8_t b = bonesUsage[vertexId];
          if (b >= kMaxBonesPerVertex)
          {
            b = kMaxBonesPerVertex - 1;
            Logger::ToLogWithFormat(Logger::Warning,
              "Maximum number of bones per vertex (%d) is exceeded.\n",
              kMaxBonesPerVertex);
          }
          bonesUsage[vertexId]++;

          boneWeights[vertexId].data[b] = bone->mWeights[weightIndex].mWeight;
          boneIndices[vertexId].data[b] = static_cast<float>(boneIndex);
        }
      }

      if (!boneWeights.empty() && !boneIndices.empty())
      {
        CopyVertexBuffer(group, MeshVertexAttribute::BoneWeights, boneWeights.data(), numVertices);
        CopyVertexBuffer(group, MeshVertexAttribute::BoneIndices, boneIndices.data(), numVertices);
      }
    }

    for (auto const & p : group.m_vertexBuffers)
      componentsMask |= p.first;


    for (uint32_t i = 0; i < numVertices; i++)
    {
      group.m_boundingBox.extend(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                           mesh->mVertices[i].z));
    }
    // TODO: calculate correct AABB for rigged mesh.
    if (mesh->HasBones())
      group.m_boundingBox.scale(glm::vec3(1.5f, 1.5f, 1.5f), group.m_boundingBox.getCenter());

    group.m_indexBuffer.reserve(mesh->mNumFaces * 3);
    for (int faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
    {
      aiFace face = mesh->mFaces[faceIndex];
      if (face.mNumIndices != 3)
        continue;

      group.m_indexBuffer.push_back(face.mIndices[0]);
      group.m_indexBuffer.push_back(face.mIndices[1]);
      group.m_indexBuffer.push_back(face.mIndices[2]);
    }

    group.m_materialIndex = static_cast<int>(mesh->mMaterialIndex);
    group.m_verticesCount = numVertices;
    group.m_indicesCount = static_cast<uint32_t>(group.m_indexBuffer.size());
    group.m_groupIndex = groupIndex++;

    verticesCount += group.m_verticesCount;
    indicesCount += group.m_indicesCount;

    meshNode->m_groups.push_back(std::move(group));
  }

  meshNode->m_children.reserve(node->mNumChildren);
  for (uint32_t nodeIndex = 0; nodeIndex < node->mNumChildren; nodeIndex++)
  {
    auto childNode = std::make_unique<BaseMesh::MeshNode>();
    LoadNode(childNode, scene, node->mChildren[nodeIndex], componentsMask, verticesCount,
             indicesCount, groupIndex, bonesIndices);
    meshNode->m_children.push_back(std::move(childNode));
  }
}

uint32_t GetVertexSize(uint32_t componentsMask)
{
  uint32_t size = 0;
  ForEachAttribute(componentsMask,
                   [&size](MeshVertexAttribute attr) { size += GetAttributeSize(attr); });
  return size;
}

uint32_t GetAttributeOffset(uint32_t componentsMask, MeshVertexAttribute attr)
{
  uint32_t offset = 0;
  ForEachAttributeWithCheck(componentsMask, [&offset, attr](MeshVertexAttribute a) {
    if (attr == a)
      return false;
    offset += GetAttributeSize(a);
    return true;
  });
  return offset;
}

void CreateVertexArray(uint32_t componentsMask)
{
  uint32_t const vertexSize = GetVertexSize(componentsMask);
  uint32_t offset = 0;
  uint32_t index = 0;
  ForEachAttribute(componentsMask, [&offset, &index, vertexSize](MeshVertexAttribute attr) {
    glVertexAttribPointer(index, GetAttributeElements(attr), GL_FLOAT, GL_FALSE, vertexSize,
                          BUFFER_OFFSET(offset));
    glEnableVertexAttribArray(index);
    offset += GetAttributeSize(attr);
    index++;
  });
}

void FillGPUBuffers(std::unique_ptr<BaseMesh::MeshNode> & meshNode, uint8_t * vbPtr, uint32_t * ibPtr,
                    uint32_t & vbOffset, uint32_t & ibOffset, uint32_t componentsMask)
{
  if (!meshNode->m_groups.empty())
  {
    uint32_t const vertexSize = GetVertexSize(componentsMask);
    for (size_t i = 0; i < meshNode->m_groups.size(); i++)
    {
      BaseMesh::MeshGroup & group = meshNode->m_groups[i];

      // fill vertex buffer
      uint8_t * ptr = vbPtr + vbOffset;
      ForEachAttribute(
          componentsMask, [&group, &ptr, &componentsMask, vertexSize](MeshVertexAttribute attr) {
            if (group.m_vertexBuffers.find(attr) != group.m_vertexBuffers.end())
            {
              uint32_t const attrSize = GetAttributeSize(attr);
              uint32_t const offset = GetAttributeOffset(componentsMask, attr);
              ByteArray & vb = group.m_vertexBuffers[attr];
              for (uint32_t j = 0; j < group.m_verticesCount; j++)
                memcpy(ptr + j * vertexSize + offset, vb.data() + j * attrSize, attrSize);
            }
          });
      group.m_vertexBuffers.clear();

      // fill index buffer
      for (uint32_t j = 0; j < group.m_indicesCount; j++)
        group.m_indexBuffer[j] += (vbOffset / vertexSize);
      memcpy(ibPtr + ibOffset, group.m_indexBuffer.data(), group.m_indicesCount * sizeof(uint32_t));
      group.m_indexBuffer.clear();
      group.m_startIndex = ibOffset;

      ibOffset += group.m_indicesCount;
      vbOffset += (group.m_verticesCount * vertexSize);
    }
  }

  for (size_t i = 0; i < meshNode->m_children.size(); i++)
    FillGPUBuffers(meshNode->m_children[i], vbPtr, ibPtr, vbOffset, ibOffset, componentsMask);
}

glm::mat4x4 CalculateTransform(int index, std::unique_ptr<BaseMesh::MeshNode> const & meshNode,
                               glm::mat4x4 const & m, bool & found)
{
  glm::mat4x4 t = meshNode->m_transform * m;
  for (size_t i = 0; i < meshNode->m_groups.size(); i++)
  {
    if (meshNode->m_groups[i].m_groupIndex == index)
    {
      found = true;
      return t;
    }
  }

  for (size_t i = 0; i < meshNode->m_children.size(); i++)
  {
    glm::mat4x4 r = CalculateTransform(index, meshNode->m_children[i], t, found);
    if (found)
      return r;
  }

  return t;
}

BaseMesh::MeshGroup const & FindMeshGroup(int index, std::unique_ptr<BaseMesh::MeshNode> const & meshNode)
{
  static BaseMesh::MeshGroup invalidGroup;
  for (size_t i = 0; i < meshNode->m_groups.size(); i++)
  {
    if (meshNode->m_groups[i].m_groupIndex == index)
      return meshNode->m_groups[i];
  }

  for (size_t i = 0; i < meshNode->m_children.size(); i++)
  {
    BaseMesh::MeshGroup const & r = FindMeshGroup(index, meshNode->m_children[i]);
    if (r.m_groupIndex >= 0)
      return r;
  }

  return invalidGroup;
}

template <typename TKeys>
bool FindInterpolationIndices(double animTime, TKeys const & keys, size_t & startIndex,
                              size_t & endIndex)
{
  if (keys.empty())
    return false;

  if (keys.size() == 1)
  {
    startIndex = 0;
    endIndex = 0;
    return true;
  }

  for (size_t i = 0; i + 1 < keys.size(); i++)
  {
    if (animTime <= keys[i + 1].first)
    {
      startIndex = i;
      endIndex = i + 1;
      return true;
    }
  }

  return false;
}

template <typename TKeys, typename TResult>
TResult InterpolateKeys(double animTime, TKeys const & keys, TResult const & defaultValue)
{
  size_t startIndex = 0;
  size_t endIndex = 0;
  if (FindInterpolationIndices(animTime, keys, startIndex, endIndex))
  {
    TResult result;
    if (startIndex == endIndex)
    {
      result = keys[startIndex].second;
    }
    else
    {
      double const delta = keys[endIndex].first - keys[startIndex].first;
      double const k = (animTime - keys[startIndex].first) / delta;
      if constexpr(std::is_same<glm::quat, TResult>::value)
        result = glm::slerp(keys[startIndex].second, keys[endIndex].second, static_cast<float>(k));
      else
        result = glm::mix(keys[startIndex].second, keys[endIndex].second, static_cast<float>(k));
    }
    return result;
  }
  return defaultValue;
}

glm::mat4x4 CalculateBoneAnimation(BoneAnimation const & boneAnim, double animTime)
{
  glm::vec3 pos = InterpolateKeys(animTime, boneAnim.m_translationKeys, glm::vec3());
  glm::quat rot = InterpolateKeys(animTime, boneAnim.m_rotationKeys, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
  glm::vec3 sc = InterpolateKeys(animTime, boneAnim.m_scaleKeys, glm::vec3(1.0f, 1.0f, 1.0f));

  return glm::scale(glm::translate(glm::mat4x4(rot), pos), sc);
}

bool FindBonesInHierarchy(std::unique_ptr<BaseMesh::MeshNode> & node, BoneIndicesCollection const & bonesIndices)
{
  if (bonesIndices.find(node->m_name) != bonesIndices.end())
    return true;

  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if (FindBonesInHierarchy(*it, bonesIndices))
      return true;
  }

  return false;
}
}  // namespace

VertexArray::VertexArray(uint32_t componentsMask)
{
  glGenVertexArrays(1, &m_vertexArray);
  glBindVertexArray(m_vertexArray);
  CreateVertexArray(componentsMask);
  glBindVertexArray(0);
}

VertexArray::~VertexArray()
{
  if (m_vertexArray != 0)
  {
    glDeleteVertexArrays(1, &m_vertexArray);
    m_vertexArray = 0;
  }
}

void VertexArray::Bind()
{
  glBindVertexArray(m_vertexArray);
}

BaseMesh::BaseMesh()
  : m_vertexBuffer(0),
    m_indexBuffer(0),
    m_isLoaded(false),
    m_verticesCount(0),
    m_indicesCount(0),
    m_componentsMask(0),
    m_groupsCount(0)
{
}

BaseMesh::~BaseMesh()
{
  Destroy();
}

void BaseMesh::Destroy()
{
  if (m_vertexBuffer != 0)
  {
    glDeleteBuffers(1, &m_vertexBuffer);
    m_vertexBuffer = 0;
  }

  if (m_indexBuffer != 0)
  {
    glDeleteBuffers(1, &m_indexBuffer);
    m_indexBuffer = 0;
  }

  m_vertexArray.reset();

  m_animations.clear();
  m_materials.clear();
  m_bonesIndices.clear();
  m_rootNode.reset();
  m_bonesRootNode.reset();

  m_verticesCount = 0;
  m_indicesCount = 0;
  m_componentsMask = 0;
  m_groupsCount = 0;
  m_isLoaded = false;
}

bool BaseMesh::Initialize(std::string const & fileName)
{
  Destroy();

  if (!LoadMesh(fileName))
    return false;

  m_isLoaded = true;
  return true;
}

glm::mat4x4 BaseMesh::GetGroupTransform(int index, glm::mat4x4 const & transform) const
{
  if (index < 0 || index >= m_groupsCount)
    return glm::mat4x4();
  bool found = false;
  return CalculateTransform(index, m_rootNode, transform, found);
}

AABB const & BaseMesh::GetGroupBoundingBox(int index) const
{
  static AABB emptyBox;
  if (index < 0 || index >= m_groupsCount)
    return emptyBox;

  MeshGroup const & group = FindMeshGroup(index, m_rootNode);
  if (group.m_groupIndex < 0)
    return emptyBox;

  return group.m_boundingBox;
}

AABB BaseMesh::GetBoundingBox() const
{
  if (m_groupsCount == 0)
    return {};
  AABB box;
  for (int i = 0; i < m_groupsCount; i++)
    box.extend(GetGroupBoundingBox(i));
  return box;
}

std::shared_ptr<MeshMaterial> BaseMesh::GetGroupMaterial(int index) const
{
  if (index < 0 || index >= m_groupsCount)
    return nullptr;
  MeshGroup const & group = FindMeshGroup(index, m_rootNode);
  if (group.m_groupIndex < 0 || group.m_materialIndex < 0)
    return nullptr;

  auto it = m_materials.find(group.m_materialIndex);
  return it != m_materials.end() ? it->second : nullptr;
}

size_t BaseMesh::GetAnimationsCount() const
{
  return m_animations.size();
}

glm::mat4x4 BaseMesh::FindBoneAnimation(uint32_t boneIndex, size_t animIndex, double animTime,
                                    bool & found)
{
  for (size_t i = 0; i < m_animations[animIndex]->m_boneAnimations.size(); i++)
  {
    BoneAnimation & boneAnim = m_animations[animIndex]->m_boneAnimations[i];
    if (boneAnim.m_boneIndex == boneIndex)
    {
      found = true;
      return CalculateBoneAnimation(boneAnim, animTime);
    }
  }

  found = false;
  return glm::mat4x4();
}

void BaseMesh::CalculateBonesTransform(size_t animIndex, double animTime, BaseMesh::MeshGroup const & group,
                                   std::unique_ptr<BaseMesh::MeshNode> const & meshNode,
                                   glm::mat4x4 const & parentTransform,
                                   std::vector<glm::mat4x4> & bonesTransforms)
{
  glm::mat4x4 t = meshNode->m_transform * parentTransform;
  auto it = m_bonesIndices.find(meshNode->m_name);
  if (it != m_bonesIndices.end())
  {
    bool found = false;
    glm::mat4x4 boneTransform = FindBoneAnimation(it->second, animIndex, animTime, found);
    if (found)
      t = boneTransform * parentTransform;

    auto boneOffsetIt = group.m_boneOffsets.find(it->second);
    if (boneOffsetIt != group.m_boneOffsets.end())
    {
      bonesTransforms[it->second] = boneOffsetIt->second * t;
    }
  }

  for (size_t i = 0; i < meshNode->m_children.size(); i++)
  {
    CalculateBonesTransform(animIndex, animTime, group, meshNode->m_children[i], t,
                            bonesTransforms);
  }
}

void BaseMesh::GetBonesTransforms(int groupIndex, size_t animIndex, double timeSinceStart, bool cycled,
                              std::vector<glm::mat4x4> & bonesTransforms)
{
  if (bonesTransforms.size() < kMaxBonesNumber)
    bonesTransforms.resize(kMaxBonesNumber);

  memset(bonesTransforms.data(), 0, sizeof(glm::mat4x4) * kMaxBonesNumber);

  if (animIndex >= m_animations.size() || m_bonesRootNode == nullptr)
    return;

  MeshGroup const & group = FindMeshGroup(groupIndex, m_rootNode);
  if (group.m_groupIndex < 0)
    return;

  double timeInTicks = m_animations[animIndex]->m_ticksPerSecond * timeSinceStart;
  double animTime = cycled ? std::fmod(timeInTicks, m_animations[animIndex]->m_durationInTicks)
                           : std::min(timeInTicks, m_animations[animIndex]->m_durationInTicks);

  CalculateBonesTransform(animIndex, animTime, group, m_bonesRootNode, glm::mat4x4(), bonesTransforms);
}

void BaseMesh::RenderGroup(int index, uint32_t instancesCount) const
{
  if (index < 0 || index >= m_groupsCount || instancesCount == 0)
    return;

  MeshGroup const & group = FindMeshGroup(index, m_rootNode);
  if (group.m_groupIndex < 0 || group.m_indicesCount == 0)
    return;

  m_vertexArray->Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
  if (instancesCount == 1)
  {
    glDrawElements(GL_TRIANGLES, group.m_indicesCount, GL_UNSIGNED_INT,
                   (GLvoid const *)(group.m_startIndex * sizeof(uint32_t)));
  }
  else
  {
    glDrawElementsInstanced(GL_TRIANGLES, group.m_indicesCount, GL_UNSIGNED_INT,
                            (GLvoid const *)(group.m_startIndex * sizeof(unsigned int)),
                            instancesCount);
  }
}

bool BaseMesh::LoadMesh(std::string const & filename)
{
  using namespace Assimp;
  MeshLogGuard logGuard(filename);

  Importer importer;
  unsigned int postProcessFlags = aiProcess_GenNormals | aiProcess_CalcTangentSpace |
                                  aiProcess_JoinIdenticalVertices | aiProcess_Triangulate |
                                  aiProcess_ValidateDataStructure | aiProcess_SortByPType;
  aiScene const * scene = importer.ReadFile(filename, postProcessFlags);
  if (scene == nullptr)
  {
    Logger::ToLogWithFormat(Logger::Error, "Could not load mesh from '%s'.", filename.c_str());
    return false;
  }

  // materials
  for (uint32_t i = 0; i < scene->mNumMaterials; i++)
  {
    auto mat = std::make_shared<MeshMaterial>();
    aiMaterial * material = scene->mMaterials[i];
    aiString diffuseName;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseName) == AI_SUCCESS)
      mat->diffuseTexture = diffuseName.C_Str();
    aiString normalsName;
    if (material->GetTexture(aiTextureType_NORMALS, 0, &normalsName) == AI_SUCCESS)
      mat->normalsTexture = normalsName.C_Str();
    aiString specularName;
    if (material->GetTexture(aiTextureType_SPECULAR, 0, &specularName) == AI_SUCCESS)
      mat->specularTexture = specularName.C_Str();

    aiColor3D diffuseColor(0.0f, 0.0f, 0.0f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
      mat->diffuseColor = MakeColor(diffuseColor);
    aiColor3D ambientColor(0.0f, 0.0f, 0.0f);
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor) == AI_SUCCESS)
      mat->ambientColor = MakeColor(ambientColor);
    aiColor3D specularColor(0.0f, 0.0f, 0.0f);
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS)
      mat->specularColor = MakeColor(specularColor);

    if (mat->isValid())
      m_materials.insert(make_pair(i, mat));
  }

  // load mesh nodes
  m_rootNode = std::make_unique<MeshNode>();
  LoadNode(m_rootNode, scene, scene->mRootNode, m_componentsMask, m_verticesCount, m_indicesCount,
           m_groupsCount, m_bonesIndices);
  if (m_groupsCount <= 0)
  {
    Logger::ToLogWithFormat(Logger::Error, "No meshes in '%s'.", filename.c_str());
    return false;
  }
  if (m_componentsMask == 0)
  {
    Logger::ToLogWithFormat(Logger::Error, "Vertices format of mesh '%s' is invalid.",
                            filename.c_str());
    return false;
  }

  // find bones root node
  for (auto it = m_rootNode->m_children.begin(); it != m_rootNode->m_children.end(); ++it)
  {
    if (FindBonesInHierarchy(*it, m_bonesIndices))
    {
      m_bonesRootNode = std::move(*it);
      m_rootNode->m_children.erase(it);
    }
  }

  // animations
  for (uint32_t animIndex = 0; animIndex < scene->mNumAnimations; animIndex++)
  {
    aiAnimation * animation = scene->mAnimations[animIndex];
    auto anim = std::make_unique<MeshAnimation>();
    anim->m_name = std::string(animation->mName.C_Str());
    anim->m_durationInTicks = animation->mDuration;
    anim->m_ticksPerSecond = (animation->mTicksPerSecond != 0.0) ? animation->mTicksPerSecond : 1.0;
    anim->m_boneAnimations.reserve(animation->mNumChannels);

    bool failed = false;
    for (uint32_t channel = 0; channel < animation->mNumChannels; channel++)
    {
      aiNodeAnim * animNode = animation->mChannels[channel];
      BoneAnimation boneAnim;
      auto it = m_bonesIndices.find(std::string(animNode->mNodeName.C_Str()));
      if (it != m_bonesIndices.end())
      {
        boneAnim.m_boneIndex = it->second;
      }
      else
      {
        Logger::ToLogWithFormat(Logger::Error, "Bone '%s' is not found in mesh '%s'.",
                                animNode->mNodeName.C_Str(), filename.c_str());
        failed = true;
        break;
      }

      for (uint32_t i = 0; i < animNode->mNumPositionKeys; i++)
        boneAnim.m_translationKeys.emplace_back(animNode->mPositionKeys[i].mTime,
                                                GetVector3(animNode->mPositionKeys[i].mValue));

      for (uint32_t i = 0; i < animNode->mNumRotationKeys; i++)
        boneAnim.m_rotationKeys.emplace_back(animNode->mRotationKeys[i].mTime,
                                             GetQuaternion(animNode->mRotationKeys[i].mValue));

      for (uint32_t i = 0; i < animNode->mNumScalingKeys; i++)
        boneAnim.m_scaleKeys.emplace_back(animNode->mScalingKeys[i].mTime,
                                          GetVector3(animNode->mScalingKeys[i].mValue));

      anim->m_boneAnimations.push_back(std::move(boneAnim));
    }

    if (!failed)
      m_animations.push_back(std::move(anim));
  }

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

bool BaseMesh::InitializeAsSphere(float radius, uint32_t componentsMask)
{
  MeshGenerator generator;
  BaseMesh::MeshGroup meshGroup;
  if (!generator.GenerateSphere(radius, componentsMask, meshGroup))
    return false;

  m_rootNode = std::make_unique<MeshNode>();
  m_componentsMask = componentsMask;
  m_verticesCount = meshGroup.m_verticesCount;
  m_indicesCount = meshGroup.m_indicesCount;
  m_groupsCount = 1;
  m_rootNode->m_groups.push_back(std::move(meshGroup));

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

bool BaseMesh::InitializeAsPlane(float width, float height, uint32_t widthSegments, uint32_t heightSegments,
                             uint32_t uSegments, uint32_t vSegments, uint32_t componentsMask)
{
  MeshGenerator generator;
  BaseMesh::MeshGroup meshGroup;
  if (!generator.GeneratePlane(width, height, componentsMask, meshGroup, widthSegments,
                               heightSegments, uSegments, vSegments))
  {
    return false;
  }

  m_rootNode = std::make_unique<MeshNode>();
  m_componentsMask = componentsMask;
  m_verticesCount = meshGroup.m_verticesCount;
  m_indicesCount = meshGroup.m_indicesCount;
  m_groupsCount = 1;
  m_rootNode->m_groups.push_back(std::move(meshGroup));

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

void BaseMesh::InitBuffers()
{
  uint32_t vbOffset = 0;
  uint32_t ibOffset = 0;
  uint32_t const vertexSize = GetVertexSize(m_componentsMask);
  std::vector<uint8_t> vb(vertexSize * m_verticesCount, 0);
  std::vector<uint32_t> ib(m_indicesCount, 0);
  FillGPUBuffers(m_rootNode, vb.data(), ib.data(), vbOffset, ibOffset, m_componentsMask);

  // fill gl buffers
  glGenBuffers(1, &m_vertexBuffer);
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    m_vertexArray = std::make_unique<VertexArray>(m_componentsMask);
    glBufferData(GL_ARRAY_BUFFER, vb.size(), vb.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  glGenBuffers(1, &m_indexBuffer);
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib.size() * sizeof(uint32_t), ib.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

PointMesh::PointMesh()
  : m_vertexBuffer(0)
{}

PointMesh::~PointMesh()
{
  Destroy();
}

void PointMesh::Initialize()
{
  glGenBuffers(1, &m_vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  m_vertexArray = std::make_unique<VertexArray>(MeshVertexAttribute::Position);
  glBufferData(GL_ARRAY_BUFFER, GetAttributeSize(MeshVertexAttribute::Position), 0, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PointMesh::Render()
{
  m_vertexArray->Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glDrawArrays(GL_POINTS, 0, 1);
}

void PointMesh::Destroy()
{
  if (m_vertexBuffer != 0)
  {
    glDeleteBuffers(1, &m_vertexBuffer);
    m_vertexBuffer = 0;
  }
  m_vertexArray.reset();
}
}  // namespace rf
