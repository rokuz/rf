#include "rf.hpp"

#include <gtest/gtest.h>

TEST(Camera, Smoke)
{
  rf::Camera camera;
  camera.Initialize(1024, 768);
  camera.SetPosition(glm::vec3(1.0f, 1.0f, 1.0f));

  glm::quat q;
  glm::rotate(q, kPi * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
  camera.SetOrientation(q);

  glm::mat4x4 v = camera.GetView();

  // Check for column-major'ness and perspective'ness.
  glm::mat4x4 p = camera.GetProjection();
  EXPECT_FLOAT_EQ(p[2][3], -1.0f);
  float zf = camera.GetZFar();
  float zn = camera.GetZNear();
  EXPECT_FLOAT_EQ(p[2][2], -(zf + zn) / (zf - zn));
  EXPECT_FLOAT_EQ(p[3][2], -2.0f * zn * zf / (zf - zn));
}

TEST(FreeCamera, Smoke)
{
  rf::FreeCamera camera;

}
