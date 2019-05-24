#include "rf.hpp"

#include <gtest/gtest.h>

TEST(Mesh, Smoke)
{
  rf::BaseMesh mesh;

  EXPECT_EQ(true, mesh.InitializeAsSphere(1.0f));
}
