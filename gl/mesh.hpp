#pragma once
#define API_OPENGL

#include "base_mesh.hpp"

namespace rf::gl
{
class VertexArray
{
public:
  VertexArray();
  ~VertexArray();

  void BindVertexAttributes(uint32_t attributesMask);

  void Bind();
  void Unbind();

private:
  GLuint m_vertexArray = 0;
  uint32_t m_lastStartIndex = 0;
};

class Mesh : public BaseMesh
{
public:
  Mesh() = default;
  ~Mesh() override;

  bool Initialize(std::string && fileName, uint32_t desiredAttributesMask = 0xffffffff);
  bool InitializeAsSphere(float radius, uint32_t attributesMask = Position | Normal | UV0 | Tangent);
  bool InitializeAsPlane(float width, float height, uint32_t widthSegments = 1,
                         uint32_t heightSegments = 1, uint32_t uSegments = 1, uint32_t vSegments = 1,
                         uint32_t attributesMask = Position | Normal | UV0 | Tangent);
  bool InitializeAsTerrain(std::vector<uint8_t> const & heightmap,
                           uint32_t heightmapWidth, uint32_t heightmapHeight,
                           float minAltitude, float maxAltitude, float width, float height,
                           uint32_t attributesMask = Position | Normal | UV0 | Tangent);
  bool InitializeAsTerrain(std::vector<glm::vec3> const & positions, std::vector<glm::vec2> const & borders,
                           uint32_t attributesMask = Position | Normal | UV0 | Tangent);

  void RenderGroup(int index, uint32_t instancesCount = 1) const;

private:
  void Destroy();
  void InitBuffers();

  std::unique_ptr<VertexArray> m_vertexArray;
  GLuint m_vertexBuffer = 0;
  GLuint m_indexBuffer = 0;
};

class SinglePointMesh
{
public:
  SinglePointMesh();
  ~SinglePointMesh();
  void Initialize();
  void Render();
  void RenderInstanced(uint32_t instancesCount);

private:
  void Destroy();

  std::unique_ptr<VertexArray> m_vertexArray;
  GLuint m_vertexBuffer = 0;
};
}  // namespace rf::gl
