#pragma once

#include "common.hpp"

namespace rf
{
class Camera
{
public:
  Camera() = default;
  virtual ~Camera() = default;

  void Initialize(uint32_t width, uint32_t height);
  void UpdateAspect(uint32_t width, uint32_t height);

  glm::mat4x4 const & GetView() const
  {
    return m_view;
  }

  glm::mat4x4 const & GetProjection() const
  {
    return m_projection;
  }

  void SetPosition(glm::vec3 const & position)
  {
    m_position = position;
		UpdateView();
  }

  glm::vec3 const & GetPosition() const
  {
    return m_position;
  }

  void SetOrientation(glm::quat const & orientation)
  {
    m_orientation = orientation;
		UpdateView();
  }

  glm::quat const & GetOrientation() const
  {
    return m_orientation;
  }

  float GetFov() const { return m_fov; }
  float GetZNear() const { return m_znear; }
  float GetZFar() const { return m_zfar; }
  float GetAspectRatio() const { return m_aspect; }

  void SetZFar(float zf)
  {
    m_zfar = zf;
    UpdateProjection();
  }

protected:
	void UpdateView();
	void UpdateProjection();

	float m_fov = 60.0f * kPi / 180.0f;
	float m_znear = 0.1f;
	float m_zfar = 1000.0f;
	float m_aspect = 1.0f;

  glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  glm::vec3 m_position;

  glm::mat4x4 m_view;
  glm::mat4x4 m_projection;
};
}  // namespace rf
