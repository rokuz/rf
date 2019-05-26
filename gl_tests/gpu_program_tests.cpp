#define API_OPENGL
#include "rf.hpp"

#include <gtest/gtest.h>

TEST(GpuProgram, Smoke)
{
  static char const * kVertexShaderCode = {
      "#version 410 core\n"
      "layout(location = 0) in vec3 aPosition;\n"
      "layout(location = 1) in vec2 aUV0;\n"
      "out vec2 vUV0;\n"
      "void main()\n"
      "{\n"
      "  gl_Position = vec4(aPosition, 1.0);\n"
      "  vUV0 = aUV0;\n"
      "}"};
  static char const * kFragmentShaderCode = {
      "#version 410 core\n"
      "in vec2 vUV0\n;"
      "out vec4 oColor;\n"
      "uniform sampler2D uTexture;\n"
      "void main()\n"
      "{\n"
      "  oColor = texture(uTexture, vUV0);\n"
      "}"};

  rf::gl::GpuProgram program;
  EXPECT_EQ(true, program.Initialize({kVertexShaderCode, "", kFragmentShaderCode}, false /* areFiles */));
}
