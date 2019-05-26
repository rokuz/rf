#pragma once

#include "common.hpp"

namespace rf
{
class Window
{
public:
  using OnKeyButtonHandler = std::function<void(int key, int scancode, bool pressed)>;
  using OnMouseButtonHandler = std::function<void(float xpos, float ypos,
                                                 int button, bool pressed)>;
  using OnMouseMoveHandler = std::function<void(float xpos, float ypos)>;

  using OnFrameHandler = std::function<void(double timeSinceStart, double elapsedTime,
                                            double averageFps)>;

  Window();
  ~Window();

  bool InitializeForOpenGL(uint32_t screenWidth, uint32_t screenHeight,
                           std::string const & title,
                           uint8_t openGLMajorVersion,
                           uint8_t openGLMinorVersion);
  bool Loop();

  uint32_t GetScreenWidth() const { return m_screenWidth; }
  uint32_t GetScreenHeight() const { return m_screenHeight; }

  void SetOnFrameHandler(OnFrameHandler && handler);
  void SetOnKeyButtonHandler(OnKeyButtonHandler && handler);
  void SetOnMouseButtonHandler(OnMouseButtonHandler && handler);
  void SetOnMouseMoveHandler(OnMouseMoveHandler && handler);

private:
  GLFWwindow * m_window = nullptr;
  std::optional<double> m_startTime;
  uint32_t m_screenWidth = 0;
  uint32_t m_screenHeight = 0;

  OnKeyButtonHandler m_onKeyButtonHandler;
  OnMouseButtonHandler m_onMouseButtonHandler;
  OnMouseMoveHandler m_onMouseMoveHandler;
  OnFrameHandler m_onFrameHandler;

  double m_fpsStorage = 0.0;
  double m_timeSinceLastFpsUpdate = 0.0;
  double m_averageFps = 0.0;
  uint32_t m_framesCounter = 0;

  friend void KeyCallback(GLFWwindow *, int, int, int, int);
  friend void MouseCallback(GLFWwindow *, int, int, int);
  friend void MouseMoveCallback(GLFWwindow *, double, double);
};
}  // namespace rf
