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
  void OnMouseButton(double xpos, double ypos, int button, bool pressed);
  void OnMouseMove(double xpos, double ypos);

  void Update(double elapsedTime, uint32_t screenWidth, uint32_t screenHeight);

  void SetSpeed(float speed)
  {
    m_speed = speed;
  }

private:
  bool m_moveForward = false;
  bool m_moveBackward = false;
  bool m_moveLeft = false;
  bool m_moveRight = false;
  bool m_rotationMode = false;
  float m_speed = 50.0f;
  glm::vec2 m_lastMousePosition;
	glm::vec2 m_currentMousePosition;
	glm::vec2 m_angles;
  double m_updateTime = 0.0;
};
}  // namespace rf