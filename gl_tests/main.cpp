#define API_OPENGL
#include "rf.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  rf::Logger::ToLogWithFormat(rf::Logger::Info,
    "Rendering framework test suite (OpenGL) started at %s.",
    rf::Utils::CurrentTimeDate().c_str());

  // We initialize window to have OpenGL context.
  uint8_t constexpr kOpenGLMajor = 4;
  uint8_t constexpr kOpenGLMinor = 1;
  rf::Window window;
  window.InitializeForOpenGL(100, 100, "", kOpenGLMajor, kOpenGLMinor);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
