#pragma once

#include "camera.hpp"

namespace rf
{
class FreeCamera : public Camera
{
public:
  FreeCamera() = default;

  void Setup(glm::vec3 const & from, glm::vec3 const & to);

  void OnKeyButton(int key, bool pressed);
  void OnMouseButton(float xpos, float ypos, int button, bool pressed);
  void OnMouseMove(float xpos, float ypos);

  void Update(double elapsedTime, uint32_t screenWidth, uint32_t screenHeight);

  void SetMoveSpeed(float speed)
  {
    m_moveSpeed = speed;
  }

  void SetRotationSpeed(float speed)
  {
    m_rotationSpeed = speed;
  }

private:
  bool m_moveForward = false;
  bool m_moveBackward = false;
  bool m_moveLeft = false;
  bool m_moveRight = false;
  bool m_rotationMode = false;
  float m_moveSpeed = 10.0f;
  float m_rotationSpeed = 1000.0f;
  glm::vec2 m_lastMousePosition;
	glm::vec2 m_currentMousePosition;
	glm::vec2 m_angles;
  double m_updateTime = 0.0;
};
}  // namespace rf
