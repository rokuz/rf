#include "mesh.hpp"
#include "rf.hpp"

namespace rf::gl
{
namespace
{
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

void CreateVertexArray(uint32_t componentsMask)
{
  uint32_t const vertexSize = GetVertexSizeInBytes(componentsMask);
  uint32_t offset = 0;
  uint32_t index = 0;
  ForEachAttribute(componentsMask, [&offset, &index, vertexSize](MeshVertexAttribute attr)
  {
    glVertexAttribPointer(index, GetAttributeElementsCount(attr), GL_FLOAT, GL_FALSE, vertexSize,
                          BUFFER_OFFSET(offset));
    glEnableVertexAttribArray(index);
    offset += GetAttributeSizeInBytes(attr);
    index++;
  });
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

bool Mesh::Initialize(std::string const & fileName)
{
  Destroy();

  if (!LoadMesh(fileName))
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

bool Mesh::InitializeAsSphere(float radius, uint32_t componentsMask)
{
  if (!GenerateSphere(radius, componentsMask))
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
                             uint32_t vSegments, uint32_t componentsMask)
{
  if (!GeneratePlane(width, height, widthSegments, heightSegments, uSegments,
                     vSegments, componentsMask))
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

void Mesh::InitBuffers()
{
  uint32_t vbOffset = 0;
  uint32_t ibOffset = 0;
  uint32_t const vertexSize = GetVertexSizeInBytes(m_componentsMask);
  std::vector<uint8_t> vb(vertexSize * m_verticesCount, 0);
  std::vector<uint32_t> ib(m_indicesCount, 0);
  FillGpuBuffers(m_rootNode, vb.data(), ib.data(), vbOffset, ibOffset, m_componentsMask);

  // Fill gl buffers.
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

SinglePointMesh::SinglePointMesh()
  : m_vertexBuffer(0)
{}

SinglePointMesh::~SinglePointMesh()
{
  Destroy();
}

void SinglePointMesh::Initialize()
{
  glGenBuffers(1, &m_vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  m_vertexArray = std::make_unique<VertexArray>(MeshVertexAttribute::Position);
  glBufferData(GL_ARRAY_BUFFER, GetAttributeSizeInBytes(MeshVertexAttribute::Position), 0, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SinglePointMesh::Render()
{
  m_vertexArray->Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glDrawArrays(GL_POINTS, 0, 1);
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
