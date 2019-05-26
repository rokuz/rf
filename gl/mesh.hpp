#pragma once
#define API_OPENGL

#include "base_mesh.hpp"

namespace rf::gl
{
class VertexArray
{
public:
  explicit VertexArray(uint32_t componentsMask);
  ~VertexArray();

  void Bind();

private:
  GLuint m_vertexArray = 0;
};

class Mesh : public BaseMesh
{
public:
  Mesh() = default;
  ~Mesh() override;

  bool Initialize(std::string const & fileName);
  bool InitializeAsSphere(float radius, uint32_t componentsMask = Position | Normal | UV0 | Tangent);
  bool InitializeAsPlane(float width, float height, uint32_t widthSegments = 1,
                         uint32_t heightSegments = 1, uint32_t uSegments = 1, uint32_t vSegments = 1,
                         uint32_t componentsMask = Position | Normal | UV0 | Tangent);

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

private:
  void Destroy();

  std::unique_ptr<VertexArray> m_vertexArray;
  GLuint m_vertexBuffer = 0;
};
}  // namespace rf::gl
