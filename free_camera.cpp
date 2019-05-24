#include "free_camera.hpp"

namespace rf
{
void FreeCamera::Setup(glm::vec3 const & from, glm::vec3 const & to)
{
  glm::vec3 dir = glm::normalize(to - from);
  m_angles.x = acos(dir.y);
  m_angles.y = acos(dir.x);

  glm::quat q1(1.0f, 0.0f, 0.0f, 0.0f);
  glm::rotate(q1, m_angles.x, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::quat q2(1.0f, 0.0f, 0.0f, 0.0f);
  glm::rotate(q2, m_angles.y, glm::vec3(1.0f, 0.0f, 0.0f));

  m_position = from;
  m_orientation = q1 * q2;
  UpdateView();
}

void FreeCamera::OnKeyButton(int key, bool pressed)
{
  if (key == GLFW_KEY_W || key == GLFW_KEY_UP)
  {
    m_moveForward = pressed;
  }
  else if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
  {
    m_moveBackward = pressed;
  }
  else if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)
  {
    m_moveLeft = pressed;
  }
  else if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)
  {
    m_moveRight = pressed;
  }
}

void FreeCamera::OnMouseButton(double xpos, double ypos, int button,
                               bool pressed)
{
  if (button == GLFW_MOUSE_BUTTON_1)
  {
    if (pressed)
    {
      m_rotationMode = true;
      m_lastMousePosition.x = static_cast<float>(xpos);
      m_lastMousePosition.y = static_cast<float>(ypos);
      m_currentMousePosition = m_lastMousePosition;
      m_updateTime = 0.0;
    }
    else
    {
      m_rotationMode = false;
    }
  }
}

void FreeCamera::OnMouseMove(double xpos, double ypos)
{
  if (m_rotationMode)
  {
    m_currentMousePosition = glm::vec2(static_cast<float>(xpos),
                                       static_cast<float>(ypos));
  }
}

void FreeCamera::Update(double elapsedTime, uint32_t screenWidth,
                        uint32_t screenHeight)
{
  bool needUpdateView = false;
  if (m_rotationMode)
  {
    float constexpr kPeriod = 1.0f / 60.0f;
    m_updateTime += elapsedTime;
    if (m_updateTime >= kPeriod)
    {
      glm::vec2 delta = m_currentMousePosition - m_lastMousePosition;
      if (fabs(delta.x) > kEps || fabs(delta.y) > kEps)
      {
        float w = m_znear * tan(m_fov);
        float h = w / m_aspect;

        float ax = atan2(w * (2.0f * delta.x / screenWidth), m_znear);
        float ay = atan2(h * (2.0f * delta.y / screenHeight), m_znear);

        glm::vec2 oldAngles = m_angles;
        m_angles.x -= static_cast<float>(ax * 100.0 * m_updateTime);
        m_angles.y += static_cast<float>(ay * 100.0 * m_updateTime);
        if (m_angles.y > 89.9f)
          m_angles.y = 89.9f;
        if (m_angles.y < -89.9)
          m_angles.y = -89.9f;

        glm::vec2 anglesDelta = m_angles - oldAngles;
        if (fabs(anglesDelta.x) > kEps || fabs(anglesDelta.y) > kEps)
        {
          glm::quat q1(1.0f, 0.0f, 0.0f, 0.0f);
          glm::rotate(q1, m_angles.x, glm::vec3(0.0f, 1.0f, 0.0f));
          glm::quat q2(1.0f, 0.0f, 0.0f, 0.0f);
          glm::rotate(q2, m_angles.y, glm::vec3(1.0f, 0.0f, 0.0f));
          m_orientation = q1 * q2;
          needUpdateView = true;
        }
        m_lastMousePosition = m_currentMousePosition;
      }

      m_updateTime -= kPeriod;
    }
  }

  if (m_moveForward)
  {
    glm::vec3 dir = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    m_position += (dir * m_speed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (m_moveBackward)
  {
    glm::vec3 dir = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    m_position -= (dir * m_speed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (m_moveLeft)
  {
    glm::vec3 dir = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    m_position += (dir * m_speed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (m_moveRight)
  {
    glm::vec3 dir = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    m_position -= (dir * m_speed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (needUpdateView)
    UpdateView();
}
}  // namespace rf
