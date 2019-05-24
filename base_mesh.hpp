#pragma once

#include "common.hpp"

namespace rf
{
using MaterialColor = std::optional<glm::vec3>;
MaterialColor const kInvalidColor = MaterialColor();

struct MeshMaterial
{
  std::string diffuseTexture;
  std::string normalsTexture;
  std::string specularTexture;
  MaterialColor diffuseColor = kInvalidColor;
  MaterialColor specularColor = kInvalidColor;
  MaterialColor ambientColor = kInvalidColor;

  bool isValid() const
  {
    return !diffuseTexture.empty() || !normalsTexture.empty() || !specularTexture.empty() ||
           diffuseColor || specularColor || ambientColor;
  }
};

using MaterialCollection = std::unordered_map<uint32_t, std::shared_ptr<MeshMaterial>>;

enum MeshVertexAttribute : uint32_t
{
  Position = 1 << 0,
  Normal = 1 << 1,
  UV0 = 1 << 2,
  UV1 = 1 << 3,
  UV2 = 1 << 4,
  UV3 = 1 << 5,
  Tangent = 1 << 6,
  Color = 1 << 7,
  BoneIndices = 1 << 8,
  BoneWeights = 1 << 9
};

std::string const kAttributesNames[] = {"aPosition",    "aNormal",     "aUV0",     "aUV1",
                                        "aUV2",         "aUV3",        "aTangent", "aColor",
                                        "aBoneIndices", "aBoneWeights"};

MeshVertexAttribute const kAllAttributes[] = {Position, Normal,  UV0,   UV1,         UV2,
                                              UV3,      Tangent, Color, BoneIndices, BoneWeights};
uint32_t constexpr kAttributesCount = sizeof(kAllAttributes) / sizeof(kAllAttributes[0]);

uint32_t constexpr kMaxBonesNumber = 64;
uint32_t constexpr kMaxBonesPerVertex = 4;

using IndexBuffer32 = std::vector<uint32_t>;
using VertexBufferCollection = std::unordered_map<MeshVertexAttribute, ByteArray>;

struct BoneAnimation
{
  uint32_t m_boneIndex = 0;
  std::vector<std::pair<double, glm::vec3>> m_translationKeys;
  std::vector<std::pair<double, glm::vec3>> m_scaleKeys;
  std::vector<std::pair<double, glm::quat>> m_rotationKeys;
};

struct MeshAnimation
{
  std::string m_name;
  double m_durationInTicks = 0.0;
  double m_ticksPerSecond = 0.0;
  std::vector<BoneAnimation> m_boneAnimations;
};

using MeshAnimations = std::vector<std::unique_ptr<MeshAnimation>>;

using BoneIndicesCollection = std::unordered_map<std::string, uint32_t>;

class VertexArray
{
public:
  explicit VertexArray(uint32_t componentsMask);
  ~VertexArray();

  void Bind();

private:
  GLuint m_vertexArray = 0;
};

class BaseMesh
{
public:
  BaseMesh();
  virtual ~BaseMesh();

  bool Initialize(std::string const & fileName);
  bool InitializeAsSphere(float radius, uint32_t componentsMask = Position | Normal | UV0 | Tangent);
  bool InitializeAsPlane(float width, float height, uint32_t widthSegments = 1,
                   uint32_t heightSegments = 1, uint32_t uSegments = 1, uint32_t vSegments = 1,
                   uint32_t componentsMask = Position | Normal | UV0 | Tangent);

  int GetGroupsCount() const
  {
    return m_groupsCount;
  }

  glm::mat4x4 GetGroupTransform(int index, glm::mat4x4 const & transform) const;
  AABB const & GetGroupBoundingBox(int index) const;
  std::shared_ptr<MeshMaterial> GetGroupMaterial(int index) const;
  AABB GetBoundingBox() const;
  void RenderGroup(int index, uint32_t instancesCount = 1) const;

  size_t GetAnimationsCount() const;
  void GetBonesTransforms(int groupIndex, size_t animIndex, double timeSinceStart, bool cycled,
                          std::vector<glm::mat4x4> & bonesTransforms);

  struct MeshGroup
  {
    VertexBufferCollection m_vertexBuffers;
    IndexBuffer32 m_indexBuffer;
    AABB m_boundingBox;
    int m_groupIndex = -1;
    uint32_t m_verticesCount = 0;
    uint32_t m_indicesCount = 0;
    uint32_t m_startIndex = 0;
    int m_materialIndex = -1;
    std::unordered_map<uint32_t, glm::mat4x4> m_boneOffsets;
  };

  struct MeshNode
  {
    std::string m_name;
    std::vector<MeshGroup> m_groups;
    glm::mat4x4 m_transform;
    std::vector<std::unique_ptr<MeshNode>> m_children;
  };

private:
  std::unique_ptr<VertexArray> m_vertexArray;
  GLuint m_vertexBuffer = 0;
  GLuint m_indexBuffer = 0;

  uint32_t m_verticesCount = 0;
  uint32_t m_indicesCount = 0;
  uint32_t m_componentsMask = 0;
  int m_groupsCount = 0;

  MaterialCollection m_materials;
  MeshAnimations m_animations;
  BoneIndicesCollection m_bonesIndices;

  std::unique_ptr<MeshNode> m_rootNode;
  std::unique_ptr<MeshNode> m_bonesRootNode;

  bool m_isLoaded = false;

  bool LoadMesh(std::string const & filename);
  void Destroy();
  void InitBuffers();
  glm::mat4x4 FindBoneAnimation(uint32_t boneIndex, size_t animIndex, double animTime, bool & found);
  void CalculateBonesTransform(size_t animIndex, double animTime, const BaseMesh::MeshGroup & group,
                               std::unique_ptr<BaseMesh::MeshNode> const & meshNode,
                               glm::mat4x4 const & parentTransform,
                               std::vector<glm::mat4x4> & bonesTransforms);
};

class PointMesh
{
public:
  PointMesh();
  ~PointMesh();
  void Initialize();
  void Render();

private:
  void Destroy();

  std::unique_ptr<VertexArray> m_vertexArray;
  GLuint m_vertexBuffer = 0;
};

extern void ForEachAttribute(uint32_t componentsMask,
                             std::function<void(MeshVertexAttribute)> const & func);
extern void ForEachAttributeWithCheck(uint32_t componentsMask,
                                      std::function<bool(MeshVertexAttribute)> const & func);
}  // namespace rf
