#pragma once

#include "common.hpp"
#include "mesh.hpp"

namespace rf
{
class MeshGenerator
{
public:
  bool GenerateSphere(float radius, uint32_t componentsMask, Mesh::MeshGroup & meshGroup);
  bool GeneratePlane(float width, float height, uint32_t componentsMask,
                     Mesh::MeshGroup & meshGroup, uint32_t widthSegments = 1,
                     uint32_t heightSegments = 1, uint32_t uSegments = 1, uint32_t vSegments = 1);
};
}  // namespace rf
