#define API_OPENGL

#include "window.hpp"

#include "logger.hpp"

namespace rf
{
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

  if (g_window->m_onMouseButtonHandler)
  {
    float sw, sh;
    glfwGetWindowContentScale(window, &sw, &sh);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    g_window->m_onMouseButtonHandler(static_cast<float>(xpos * sw),
                                     static_cast<float>(ypos * sh),
                                     button, action != GLFW_RELEASE);
  }
}

void MouseMoveCallback(GLFWwindow * window, double xpos, double ypos)
{
  if (!g_window)
    return;

  if (g_window->m_onMouseMoveHandler)
  {
    float sw, sh;
    glfwGetWindowContentScale(window, &sw, &sh);

    g_window->m_onMouseMoveHandler(static_cast<float>(xpos * sw),
                                   static_cast<float>(ypos * sh));
  }
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

bool Window::InitializeForOpenGL(uint32_t screenWidth, uint32_t screenHeight,
                                 std::string const & title,
                                 uint8_t openGLMajorVersion,
                                 uint8_t openGLMinorVersion)
{
  glfwSetErrorCallback([](int error, char const * description)
  {
    Logger::ToLogWithFormat(Logger::Error, "GLFW: (%d): %s", error, description);
  });

  if (!glfwInit())
    return false;

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, openGLMajorVersion);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, openGLMinorVersion);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 0);

  std::string t = title.empty() ? "Rendering framework" : title;
  m_window = glfwCreateWindow(static_cast<int>(screenWidth),
                              static_cast<int>(screenHeight),
                              t.c_str(), nullptr, nullptr);
  if (m_window == nullptr)
  {
    glfwTerminate();
    return false;
  }

  int w, h;
  float sw, sh;
  glfwGetWindowSize(m_window, &w, &h);
  glfwGetWindowContentScale(m_window, &sw, &sh);

  m_screenWidth = static_cast<uint32_t>(sw * w);
  m_screenHeight = static_cast<uint32_t>(sh * h);

  glfwSetKeyCallback(m_window, KeyCallback);
  glfwSetMouseButtonCallback(m_window, MouseCallback);
  glfwSetCursorPosCallback(m_window, MouseMoveCallback);

  glfwMakeContextCurrent(m_window);

#ifdef WINDOWS_PLATFORM
  if (gl3wInit() < 0)
  {
    Logger::ToLog(Logger::Error, "OpenGL initialization failed.");
    glfwTerminate();
    return false;
  }

  if (!gl3wIsSupported(openGLMajorVersion, openGLMinorVersion))
  {
    Logger::ToLogWithFormat(Logger::Error, "OpenGL version %d.%d is not supported.",
                            openGLMajorVersion, openGLMinorVersion);
    glfwTerminate();
    return false;
  }
#endif

  if (sw != 1.0f || sh != 1.0f)
  {
    Logger::ToLogWithFormat(Logger::Info,
      "The operation system rescales window with scale factors = (%.1f; %.1f).", sw, sh);
  }

  Logger::ToLogWithFormat(Logger::Info, "OpenGL %d.%d context created with framebuffer size %dx%d.",
    openGLMajorVersion, openGLMinorVersion, m_screenWidth, m_screenHeight);

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
