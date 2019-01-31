#include "camera.hpp"

namespace rf
{
void Camera::Initialize(uint32_t width, uint32_t height)
{
  UpdateView();
  UpdateAspect(width, height);
}

void Camera::UpdateAspect(uint32_t width, uint32_t height)
{
  m_aspect = static_cast<float>(width) / height;
  UpdateProjection();
}

void Camera::UpdateView()
{
  glm::vec3 dir = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
  m_view = glm::lookAt(m_position, m_position + dir, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::UpdateProjection()
{
  m_projection = glm::perspective(m_fov, m_aspect, m_znear, m_zfar);
}
}  // namespace rf