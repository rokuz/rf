#define API_OPENGL
#include "rf.hpp"

#include <gtest/gtest.h>

TEST(Mesh, Smoke)
{
  rf::gl::Mesh mesh;

  auto const desiredAttributesMask = rf::MeshVertexAttribute::Position |
                                     rf::MeshVertexAttribute::Normal |
                                     rf::MeshVertexAttribute::UV0 |
                                     rf::MeshVertexAttribute::Tangent;
  EXPECT_EQ(true, mesh.InitializeAsSphere(1.0f, desiredAttributesMask));
  EXPECT_EQ(desiredAttributesMask, mesh.GetAttributesMask());

  EXPECT_EQ(true, mesh.InitializeAsPlane(10.0f, 10.0f, 1, 1, 1, 1, desiredAttributesMask));
  EXPECT_EQ(desiredAttributesMask, mesh.GetAttributesMask());
}
