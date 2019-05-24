#include "window.hpp"

#include "logger.hpp"

namespace rf
{
uint8_t constexpr kOpenGLMajor = 4;
uint8_t constexpr kOpenGLMinor = 1;

Window * g_window = nullptr;

void KeyCallback(GLFWwindow * window, int key, int scancode, int action, int)
{
  if (!g_window)
    return;

  switch (key)
  {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, 1);
      break;
    default:
      if (g_window->m_onKeyButtonHandler)
        g_window->m_onKeyButtonHandler(key, scancode, action != GLFW_RELEASE);
  }
}

void MouseCallback(GLFWwindow * window, int button, int action, int)
{
  if (!g_window)
    return;

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  if (g_window->m_onMouseButtonHandler)
    g_window->m_onMouseButtonHandler(xpos, ypos, button, action != GLFW_RELEASE);
}

void MouseMoveCallback(GLFWwindow *, double xpos, double ypos)
{
  if (!g_window)
    return;

  if (g_window->m_onMouseMoveHandler)
    g_window->m_onMouseMoveHandler(xpos, ypos);
}

Window::Window()
{
  assert(g_window == nullptr && "The only window is supported simultaneously");
  g_window = this;
}

Window::~Window()
{
  glfwTerminate();
  g_window = nullptr;
}

bool Window::Initialize(uint32_t screenWidth, uint32_t screenHeight)
{
  glfwSetErrorCallback([](int error, char const * description)
  {
    Logger::ToLogWithFormat(Logger::Error, "GLFW: (%d): %s", error, description);
  });

  if (!glfwInit())
    return false;

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kOpenGLMajor);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kOpenGLMinor);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 0);

  m_window = glfwCreateWindow(static_cast<int>(screenWidth),
                              static_cast<int>(screenHeight),
                              "Rendering framework", nullptr, nullptr);
  if (m_window == nullptr)
  {
    glfwTerminate();
    return false;
  }

  glfwSetKeyCallback(m_window, KeyCallback);
  glfwSetMouseButtonCallback(m_window, MouseCallback);
  glfwSetCursorPosCallback(m_window, MouseMoveCallback);

  glfwMakeContextCurrent(m_window);

#ifdef WINDOWS_PLATFORM
  if (gl3wInit() < 0)
  {
    Logger::ToLog("Error: OpenGL initialization failed.");
    glfwTerminate();
    return false;
  }

  if (!gl3wIsSupported(kOpenGLMajor, kOpenGLMinor))
  {
    Logger::ToLogWithFormat("Error: OpenGL version %d.%d is not supported.",
                            kOpenGLMajor, kOpenGLMinor);
    glfwTerminate();
    return false;
  }
#endif

  return true;
}

bool Window::Loop()
{
  if (!m_startTime)
    m_startTime = glfwGetTime();

  if (glfwWindowShouldClose(m_window))
    return false;

  auto const currentTime = glfwGetTime();
  double const elapsedTime = currentTime - m_startTime.value();
  m_startTime = currentTime;

  if (elapsedTime != 0.0)
  {
    m_timeSinceLastFpsUpdate += elapsedTime;
    m_framesCounter++;
    m_fpsStorage += (1.0 / elapsedTime);

    if (m_timeSinceLastFpsUpdate >= 1.0f)
    {
      assert(m_framesCounter != 0.0);
      m_averageFps = m_fpsStorage / static_cast<double>(m_framesCounter);
      m_timeSinceLastFpsUpdate -= 1.0f;
      m_framesCounter = 0;
      m_fpsStorage = 0;
    }
  }

  if (m_onFrameHandler)
    m_onFrameHandler(currentTime, elapsedTime, m_averageFps);

  glfwSwapBuffers(m_window);
  glfwPollEvents();

  return true;
}

void Window::SetOnFrameHandler(OnFrameHandler && handler)
{
  m_onFrameHandler = std::move(handler);
}

void Window::SetOnKeyButtonHandler(OnKeyButtonHandler && handler)
{
  m_onKeyButtonHandler = std::move(handler);
}

void Window::SetOnMouseButtonHandler(OnMouseButtonHandler && handler)
{
  m_onMouseButtonHandler = std::move(handler);
}

void Window::SetOnMouseMoveHandler(OnMouseMoveHandler && handler)
{
  m_onMouseMoveHandler = std::move(handler);
}
}  // namespace rf
