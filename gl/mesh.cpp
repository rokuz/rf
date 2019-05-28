#include "mesh.hpp"
#include "rf.hpp"

namespace rf::gl
{
namespace
{
uint32_t BindAttributes(uint32_t startIndex, uint32_t componentsMask)
{
  uint32_t const vertexSize = GetVertexSizeInBytes(componentsMask);
  uint32_t offset = 0;
  uint32_t index = startIndex;
  ForEachAttribute(componentsMask, [&offset, &index, vertexSize](MeshVertexAttribute attr)
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

void VertexArray::BindVertexAttributes(uint32_t componentsMask)
{
  m_lastStartIndex = BindAttributes(m_lastStartIndex, componentsMask);
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

  if (m_bonesIndicesBuffer != 0)
  {
    glDeleteBuffers(1, &m_bonesIndicesBuffer);
    m_bonesIndicesBuffer = 0;
  }

  if (m_indexBuffer != 0)
  {
    glDeleteBuffers(1, &m_indexBuffer);
    m_indexBuffer = 0;
  }

  m_vertexArray.reset();

  DestroyMesh();
}

bool Mesh::Initialize(std::string && fileName)
{
  Destroy();

  if (!LoadMesh(std::move(fileName)))
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
                   (GLvoid const *)(group.m_startIndex * sizeof(uint32_t)));
  }
  else
  {
    glDrawElementsInstanced(GL_TRIANGLES, group.m_indicesCount, GL_UNSIGNED_INT,
                            (GLvoid const *)(group.m_startIndex * sizeof(uint32_t)),
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
  uint32_t componentsMask = m_componentsMask;
  if (m_componentsMask & MeshVertexAttribute::BoneIndices)
    componentsMask &= (~MeshVertexAttribute::BoneIndices);

  m_vertexArray = std::make_unique<VertexArray>();
  m_vertexArray->Bind();

  uint32_t vbOffset = 0;
  uint32_t ibOffset = 0;
  std::vector<uint8_t> vb(GetVertexSizeInBytes(componentsMask) * m_verticesCount, 0);
  std::vector<uint32_t> ib(m_indicesCount, 0);
  FillGpuBuffers(m_rootNode, vb.data(), ib.data(), vbOffset, ibOffset,
                 true /* fillIndexBuffer */, componentsMask);

  // Fill OpenGL buffers.
  glGenBuffers(1, &m_vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vb.size(), vb.data(), GL_STATIC_DRAW);
  m_vertexArray->BindVertexAttributes(componentsMask);

  if (m_componentsMask & MeshVertexAttribute::BoneIndices)
  {
    componentsMask = MeshVertexAttribute::BoneIndices;
    vbOffset = 0;
    ibOffset = 0;
    std::vector<uint8_t> bi(GetVertexSizeInBytes(componentsMask) * m_verticesCount, 0);
    FillGpuBuffers(m_rootNode, bi.data(), nullptr, vbOffset, ibOffset,
                   false /* fillIndexBuffer */, componentsMask);

    glGenBuffers(1, &m_bonesIndicesBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_bonesIndicesBuffer);
    glBufferData(GL_ARRAY_BUFFER, bi.size(), bi.data(), GL_STATIC_DRAW);
    m_vertexArray->BindVertexAttributes(componentsMask);
  }

  glGenBuffers(1, &m_indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib.size() * sizeof(uint32_t), ib.data(), GL_STATIC_DRAW);

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
