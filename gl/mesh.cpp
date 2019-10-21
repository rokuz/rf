#include "mesh.hpp"
#include "rf.hpp"

namespace rf::gl
{
namespace
{
uint32_t BindAttributes(uint32_t startIndex, uint32_t attributesMask)
{
  uint32_t const vertexSize = GetVertexSizeInBytes(attributesMask);
  uint32_t offset = 0;
  uint32_t index = startIndex;
  ForEachAttribute(attributesMask, [&offset, &index, vertexSize](MeshVertexAttribute attr)
  {
    switch (GetAttributeUnderlyingType(attr))
    {
      case MeshAttributeUnderlyingType::Float:
        glVertexAttribPointer(index, GetAttributeElementsCount(attr), GL_FLOAT, GL_FALSE, vertexSize,
                              reinterpret_cast<void const *>(offset));
        break;
      case MeshAttributeUnderlyingType::UnsignedInteger:
        glVertexAttribIPointer(index, GetAttributeElementsCount(attr), GL_UNSIGNED_INT, vertexSize,
                               reinterpret_cast<void const*>(offset));
        break;
      default:
        CHECK(false, ("Unknown underlying type."));
    }
    glEnableVertexAttribArray(index);

    offset += GetAttributeSizeInBytes(attr);
    index++;
  });
  return index;
}
}  // namespace

VertexArray::VertexArray()
{
  glGenVertexArrays(1, &m_vertexArray);
}

VertexArray::~VertexArray()
{
  if (m_vertexArray != 0)
  {
    glDeleteVertexArrays(1, &m_vertexArray);
    m_vertexArray = 0;
  }
}

void VertexArray::BindVertexAttributes(uint32_t attributesMask)
{
  m_lastStartIndex = BindAttributes(m_lastStartIndex, attributesMask);
}

void VertexArray::Bind()
{
  glBindVertexArray(m_vertexArray);
}

void VertexArray::Unbind()
{
  glBindVertexArray(0);
}

Mesh::~Mesh()
{
  Destroy();
}

void Mesh::Destroy()
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

  DestroyMesh();
}

bool Mesh::Initialize(std::string && fileName, uint32_t desiredAttributesMask)
{
  Destroy();

  if (!LoadMesh(std::move(fileName), desiredAttributesMask))
    return false;

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

void Mesh::RenderGroup(int index, uint32_t instancesCount) const
{
  if (index < 0 || index >= m_groupsCount || instancesCount == 0)
    return;

  MeshGroup const & group = FindCachedMeshGroup(index);
  if (group.m_groupIndex < 0 || group.m_indicesCount == 0)
    return;

  m_vertexArray->Bind();
  if (instancesCount == 1)
  {
    glDrawElements(GL_TRIANGLES, group.m_indicesCount, GL_UNSIGNED_INT,
                   reinterpret_cast<void const *>(group.m_startIndex * sizeof(uint32_t)));
  }
  else
  {
    glDrawElementsInstanced(GL_TRIANGLES, group.m_indicesCount, GL_UNSIGNED_INT,
                            (GLvoid const *)(group.m_startIndex * sizeof(uint32_t)),
                            instancesCount);
  }
}

bool Mesh::InitializeAsSphere(float radius, uint32_t attributesMask)
{
  if (!GenerateSphere(radius, attributesMask))
    return false;

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

bool Mesh::InitializeAsPlane(float width, float height, uint32_t widthSegments,
                             uint32_t heightSegments, uint32_t uSegments,
                             uint32_t vSegments, uint32_t attributesMask)
{
  if (!GeneratePlane(width, height, widthSegments, heightSegments, uSegments,
                     vSegments, attributesMask))
  {
    return false;
  }

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

bool Mesh::InitializeAsTerrain(std::vector<uint8_t> const & heightmap,
                               uint32_t heightmapWidth, uint32_t heightmapHeight,
                               float minAltitude, float maxAltitude, float width, float height,
                               uint32_t attributesMask)
{
  if (!GenerateTerrain(heightmap, heightmapWidth, heightmapHeight, minAltitude, maxAltitude,
                       width, height, attributesMask))
  {
    return false;
  }

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

bool Mesh::InitializeAsTerrain(std::vector<glm::vec3> const & positions,
                               std::vector<glm::vec2> const & borders,
                               uint32_t attributesMask)
{
  if (!GenerateTerrain(positions, borders, attributesMask))
    return false;

  InitBuffers();
  if (glCheckError())
  {
    Destroy();
    return false;
  }

  return true;
}

bool Mesh::InitializeWithPositions(std::vector<glm::vec3> const & positions, IndexBuffer32 const & indexBuffer)
{
  AABB aabb;
  for (auto const & p : positions)
    aabb.extend(p);

  ByteArray array(positions.size() * sizeof(glm::vec3));
  memcpy(array.data(), reinterpret_cast<float const *>(positions.data()), array.size());
  VertexBufferCollection vertexBuffers;
  vertexBuffers.emplace(MeshVertexAttribute::Position, std::move(array));
  return InitializeWithBuffers(vertexBuffers, positions.size(), indexBuffer, aabb);
}

bool Mesh::InitializeWithBuffers(VertexBufferCollection const & vertexBuffers, uint32_t verticesCount,
                                 IndexBuffer32 const & indexBuffer, AABB const & aabb)
{
  uint32_t attributesMask = 0;
  for (auto const & v : vertexBuffers)
    attributesMask |= v.first;

  BaseMesh::MeshGroup meshGroup;
  meshGroup.m_vertexBuffers = vertexBuffers;
  meshGroup.m_indexBuffer = indexBuffer;
  meshGroup.m_boundingBox = aabb;
  meshGroup.m_groupIndex = 0;
  meshGroup.m_verticesCount = verticesCount;
  meshGroup.m_indicesCount = static_cast<uint32_t>(indexBuffer.size());

  m_rootNode = std::make_unique<MeshNode>();
  m_attributesMask = attributesMask;
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

void Mesh::InitBuffers()
{
  m_vertexArray = std::make_unique<VertexArray>();
  m_vertexArray->Bind();

  uint32_t vbOffset = 0;
  uint32_t ibOffset = 0;
  std::vector<uint8_t> vb(GetVertexSizeInBytes(m_attributesMask) * m_verticesCount, 0);
  std::vector<uint32_t> ib(m_indicesCount, 0);
  FillGpuBuffers(m_rootNode, vb.data(), ib.data(), vbOffset, ibOffset,
                 true /* fillIndexBuffer */, m_attributesMask);

  // Fill OpenGL buffers.
  glGenBuffers(1, &m_vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vb.size(), vb.data(), GL_STATIC_DRAW);

  m_vertexArray->BindVertexAttributes(m_attributesMask);

  glGenBuffers(1, &m_indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib.size() * sizeof(uint32_t),
               ib.data(), GL_STATIC_DRAW);

  m_vertexArray->Unbind();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

SinglePointMesh::SinglePointMesh()
  : m_vertexBuffer(0)
{}

SinglePointMesh::~SinglePointMesh()
{
  Destroy();
}

void SinglePointMesh::Initialize()
{
  m_vertexArray = std::make_unique<VertexArray>();
  m_vertexArray->Bind();
  glGenBuffers(1, &m_vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, GetAttributeSizeInBytes(MeshVertexAttribute::Position),
               0, GL_STATIC_DRAW);
  m_vertexArray->BindVertexAttributes(MeshVertexAttribute::Position);
  m_vertexArray->Unbind();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SinglePointMesh::Render()
{
  m_vertexArray->Bind();
  glDrawArrays(GL_POINTS, 0, 1);
}

void SinglePointMesh::RenderInstanced(uint32_t instancesCount)
{
  m_vertexArray->Bind();
  glDrawArraysInstanced(GL_POINTS, 0, 1, instancesCount);
}

void SinglePointMesh::Destroy()
{
  if (m_vertexBuffer != 0)
  {
    glDeleteBuffers(1, &m_vertexBuffer);
    m_vertexBuffer = 0;
  }
  m_vertexArray.reset();
}
}  // namespace rf::gl
