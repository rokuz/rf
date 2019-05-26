#define API_OPENGL
#include "rf.hpp"

#include <gtest/gtest.h>

TEST(Mesh, Smoke)
{
  rf::gl::Mesh mesh;

  EXPECT_EQ(true, mesh.InitializeAsSphere(1.0f));
}
