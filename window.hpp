#pragma once

#include "common.hpp"

namespace rf
{
class Window
{
public:
  using OnKeyButtonHandler = std::function<void(int key, int scancode, bool pressed)>;
  using OnMouseButtonHandler =std::function<void(double xpos, double ypos,
                                                 int button, bool pressed)>;
  using OnMouseMoveHandler = std::function<void(double xpos, double ypos)>;

  using OnFrameHandler = std::function<void(double timeSinceStart, double elapsedTime,
                                            double averageFps)>;

  Window();
  ~Window();

  bool Initialize(uint32_t screenWidth, uint32_t screenHeight);
  bool Loop();

  void SetOnFrameHandler(OnFrameHandler && handler);
  void SetOnKeyButtonHandler(OnKeyButtonHandler && handler);
  void SetOnMouseButtonHandler(OnMouseButtonHandler && handler);
  void SetOnMouseMoveHandler(OnMouseMoveHandler && handler);

private:
  GLFWwindow * m_window = nullptr;
  std::optional<double> m_startTime;

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
