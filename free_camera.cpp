#include "free_camera.hpp"

#include "logger.hpp"

namespace rf
{
void FreeCamera::Setup(glm::vec3 const & from, glm::vec3 const & to)
{
  glm::vec3 dir = glm::normalize(to - from);
  m_angles.x = RadToDeg(acos(dir.z));
  m_angles.y = RadToDeg(atan2(dir.y, dir.x));

  auto const q1 = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                              DegToRad(m_angles.x), glm::vec3(0.0f, 1.0f, 0.0f));
  auto const q2 = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                              DegToRad(m_angles.y), glm::vec3(1.0f, 0.0f, 0.0f));

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

void FreeCamera::OnMouseButton(float xpos, float ypos, int button,
                               bool pressed)
{
  if (button == GLFW_MOUSE_BUTTON_1)
  {
    if (pressed)
    {
      m_rotationMode = true;
      m_lastMousePosition.x = xpos;
      m_lastMousePosition.y = ypos;
      m_currentMousePosition = m_lastMousePosition;
      m_updateTime = 0.0;
    }
    else
    {
      m_rotationMode = false;
    }
  }
}

void FreeCamera::OnMouseMove(float xpos, float ypos)
{
  if (m_rotationMode)
    m_currentMousePosition = glm::vec2(xpos, ypos);
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
        m_angles.x -= static_cast<float>(ax * m_rotationSpeed * m_updateTime);
        m_angles.y += static_cast<float>(ay * m_rotationSpeed * m_updateTime);
        if (m_angles.y > 89.9f)
          m_angles.y = 89.9f;
        if (m_angles.y < -89.9f)
          m_angles.y = -89.9f;

        glm::vec2 anglesDelta = m_angles - oldAngles;
        if (fabs(anglesDelta.x) > kEps || fabs(anglesDelta.y) > kEps)
        {
          auto const q1 = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                                      DegToRad(m_angles.x), glm::vec3(0.0f, 1.0f, 0.0f));
          auto const q2 = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                                      DegToRad(m_angles.y), glm::vec3(1.0f, 0.0f, 0.0f));
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
    m_position += (dir * m_moveSpeed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (m_moveBackward)
  {
    glm::vec3 dir = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    m_position -= (dir * m_moveSpeed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (m_moveLeft)
  {
    glm::vec3 dir = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    m_position += (dir * m_moveSpeed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (m_moveRight)
  {
    glm::vec3 dir = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    m_position -= (dir * m_moveSpeed * static_cast<float>(elapsedTime));
    needUpdateView = true;
  }

  if (needUpdateView)
    UpdateView();
}
}  // namespace rf
