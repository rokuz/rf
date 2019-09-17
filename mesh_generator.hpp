#pragma once

#include "common.hpp"
#include "base_mesh.hpp"

namespace rf
{
class MeshGenerator
{
public:
  bool GenerateSphere(float radius, uint32_t componentsMask, BaseMesh::MeshGroup & meshGroup);
  bool GeneratePlane(float width, float height, uint32_t componentsMask,
                     BaseMesh::MeshGroup & meshGroup, uint32_t widthSegments = 1,
                     uint32_t heightSegments = 1, uint32_t uSegments = 1, uint32_t vSegments = 1);
  bool GenerateTerrain(std::vector<uint8_t> const & heightmap,
                       uint32_t heightmapWidth, uint32_t heightmapHeight,
                       uint32_t componentsMask, float minAltitude, float maxAltitude,
                       float width, float height, BaseMesh::MeshGroup & meshGroup);
};
}  // namespace rf
