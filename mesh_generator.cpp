#include "mesh_generator.hpp"
#include "mesh_simplifier.hpp"

#include "logger.hpp"

#include "3party/delaunator-cpp/include/delaunator.hpp"

namespace rf
{
namespace
{
void InitIcosahedron(float radius, std::vector<glm::vec3> & positions,
                     std::vector<uint32_t> & indices)
{
  positions.reserve(12);

  auto addVertex = [&positions, &radius](glm::vec3 const & v) {
    auto v2 = glm::normalize(v);
    v2 *= radius;
    positions.push_back(v2);
  };

  float const t = (1.0f + sqrt(5.0f)) / 2.0f;

  addVertex(glm::vec3(-1, t, 0));
  addVertex(glm::vec3(1, t, 0));
  addVertex(glm::vec3(-1, -t, 0));
  addVertex(glm::vec3(1, -t, 0));

  addVertex(glm::vec3(0, -1, t));
  addVertex(glm::vec3(0, 1, t));
  addVertex(glm::vec3(0, -1, -t));
  addVertex(glm::vec3(0, 1, -t));

  addVertex(glm::vec3(t, 0, -1));
  addVertex(glm::vec3(t, 0, 1));
  addVertex(glm::vec3(-t, 0, -1));
  addVertex(glm::vec3(-t, 0, 1));

  indices = {0, 11, 5,  0, 5,  1, 0, 1, 7, 0, 7,  10, 0, 10, 11, 1, 5, 9, 5, 11,
             4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3,  9,  4, 3,  4,  2, 3, 2, 6, 3,
             6, 8,  3,  8, 9,  4, 9, 5, 2, 4, 11, 6,  2, 10, 8,  6, 7, 9, 8, 1};
}

uint32_t SplitIcosahedronEdge(float radius, std::vector<glm::vec3> & positions, uint32_t index1,
                              uint32_t index2)
{
  glm::vec3 pos = glm::normalize(positions[index1] + positions[index2]);
  pos *= radius;
  positions.push_back(std::move(pos));
  return static_cast<uint32_t>(positions.size()) - 1;
}

void TesselateIcosahedron(float radius, std::vector<glm::vec3> & positions,
                          std::vector<uint32_t> & indices, uint32_t tesselationLevel)
{
  for (uint32_t i = 0; i < tesselationLevel; i++)
  {
    std::vector<uint32_t> newIndicies;
    newIndicies.reserve(indices.size() * 12);

    for (size_t j = 0; j < indices.size(); j += 3)
    {
      uint32_t a = SplitIcosahedronEdge(radius, positions, indices[j], indices[j + 1]);
      uint32_t b = SplitIcosahedronEdge(radius, positions, indices[j + 1], indices[j + 2]);
      uint32_t c = SplitIcosahedronEdge(radius, positions, indices[j + 2], indices[j]);

      newIndicies.push_back(indices[j]);
      newIndicies.push_back(a);
      newIndicies.push_back(c);

      newIndicies.push_back(indices[j + 1]);
      newIndicies.push_back(b);
      newIndicies.push_back(a);

      newIndicies.push_back(indices[j + 2]);
      newIndicies.push_back(c);
      newIndicies.push_back(b);

      newIndicies.push_back(a);
      newIndicies.push_back(b);
      newIndicies.push_back(c);
    }

    indices = newIndicies;
  }
}

glm::vec2 GetIcosahedronUV(glm::vec3 const & position)
{
  glm::vec3 p = glm::normalize(position);
  glm::vec2 uv;
  uv.x = 0.5f + atan2(p.z, p.x) / (2.0f * kPi);
  uv.y = 0.5f + asin(p.y) / kPi;
  return uv;
}

std::vector<uint32_t> GetWrappedTriangles(std::vector<glm::vec2> const & uv,
                                          std::vector<uint32_t> const & indices)
{
  std::vector<uint32_t> result;
  for (size_t i = 0; i < indices.size(); i += 3)
  {
    glm::vec3 uvA = glm::vec3(uv[indices[i]].x, uv[indices[i]].y, 0.0);
    glm::vec3 uvB = glm::vec3(uv[indices[i + 1]].x, uv[indices[i + 1]].y, 0.0);
    glm::vec3 uvC = glm::vec3(uv[indices[i + 2]].x, uv[indices[i + 2]].y, 0.0);
    float const z = ((uvB - uvA) * (uvC - uvA)).z;
    if (z > 0)
      result.push_back(static_cast<uint32_t>(i));
  }
  return result;
}

void FixWrappedTriangles(std::vector<glm::vec2> & uv, std::vector<glm::vec3> & positions,
                         std::vector<uint32_t> & indices)
{
  auto wrappedIndices = GetWrappedTriangles(uv, indices);

  std::map<uint32_t, uint32_t> reindexed;

  auto fixUv = [&reindexed, &uv, &positions](uint32_t & index) {
    if (uv[index].x < 0.25f)
    {
      auto it = reindexed.find(index);
      if (it == reindexed.end())
      {
        positions.push_back(positions[index]);

        glm::vec2 texCoord = uv[index];
        texCoord.x += 1.0f;
        uv.push_back(texCoord);

        uint32_t newIndex = static_cast<uint32_t>(positions.size()) - 1;
        reindexed[index] = newIndex;
        index = newIndex;
      }
      else
      {
        index = it->second;
      }
    }
  };

  for (size_t i = 0; i < wrappedIndices.size(); i++)
  {
    uint32_t i1 = indices[wrappedIndices[i]];
    fixUv(i1);
    uint32_t i2 = indices[wrappedIndices[i] + 1];
    fixUv(i2);
    uint32_t i3 = indices[wrappedIndices[i] + 2];
    fixUv(i3);

    indices[wrappedIndices[i]] = i1;
    indices[wrappedIndices[i] + 1] = i2;
    indices[wrappedIndices[i] + 2] = i3;
  }
}

void InitPlane(float width, float height, uint32_t widthSegments, uint32_t heightSegments,
               uint32_t uSegments, uint32_t vSegments, std::vector<glm::vec3> & positions,
               std::vector<glm::vec2> & uv, std::vector<uint32_t> & indices)
{
  uint32_t sx = widthSegments + 1;
  uint32_t sy = heightSegments + 1;
  uint32_t verticesCount = sx * sy;
  uint32_t indicesCount = widthSegments * heightSegments * 6;

  positions.reserve(verticesCount);
  uv.reserve(verticesCount);
  indices.reserve(indicesCount);

  for (uint32_t y = 0; y < sy; y++)
  {
    float pz = (static_cast<float>(y) / static_cast<float>(sy - 1) - 0.5f) * height;
    for (uint32_t x = 0; x < sx; x++)
    {
      float px = (static_cast<float>(x) / static_cast<float>(sx - 1) - 0.5f) * width;
      positions.emplace_back(glm::vec3(px, 0, pz));
      uv.emplace_back(glm::vec2(static_cast<float>(x) * uSegments / static_cast<float>(sx - 1),
                              static_cast<float>(y) * vSegments / static_cast<float>(sy - 1)));
    }
  }

  for (uint32_t y = 0; y < heightSegments; y++)
  {
    uint32_t offset = y * sx;
    for (uint32_t x = 0; x < widthSegments; x++)
    {
      indices.push_back(offset + x);
      indices.push_back(offset + x + sx);
      indices.push_back(offset + x + sx + 1);
      indices.push_back(offset + x + sx + 1);
      indices.push_back(offset + x + 1);
      indices.push_back(offset + x);
    }
  }
}

template <typename T>
void CopyToVertexBuffer(ByteArray & vb, std::vector<T> const & v)
{
  size_t const sz = v.size() * sizeof(T);
  vb.resize(sz);
  memcpy(vb.data(), v.data(), sz);
}

bool IsPointInside(std::vector<glm::vec2> const & borders, glm::vec2 const & pt)
{
  if (borders.empty())
    return true;

  uint32_t rCross = 0;
  uint32_t lCross = 0;
  size_t const numPoints = borders.size();

  auto prev = borders[numPoints - 1] - pt;
  for (size_t i = 0; i < numPoints; ++i)
  {
    auto const cur = borders[i] - pt;
    if (fabs(glm::length(cur)) < 1e-9)
      return true;

    bool const rCheck = ((cur.y > 0) != (prev.y > 0));
    bool const lCheck = ((cur.y < 0) != (prev.y < 0));
    if (rCheck || lCheck)
    {
      auto const delta = prev.y - cur.y;
      auto const cp = cur.x * prev.y - cur.y * prev.x;
      if (cp != 0.0)
      {
        bool const prevGreaterCur = delta > 0.0;
        if (rCheck && ((cp > 0.0) == prevGreaterCur))
          ++rCross;
        if (lCheck && ((cp > 0.0) != prevGreaterCur))
          ++lCross;
      }
    }
    prev = cur;
  }

  if ((rCross & 1) != (lCross & 1))
    return true;

  return static_cast<bool>(rCross & 1);
}
}  // namespace

bool MeshGenerator::GenerateSphere(float radius, uint32_t componentsMask,
                                   BaseMesh::MeshGroup & meshGroup)
{
  if (componentsMask == 0)
  {
    Logger::ToLog(Logger::Error, "Can't generate sphere, components mask is invalid.");
    return false;
  }

  if (radius <= 0)
  {
    Logger::ToLog(Logger::Error, "Can't generate sphere, radius must be more than 0.");
    return false;
  }

  std::vector<glm::vec3> positions;
  std::vector<uint32_t> indices;
  InitIcosahedron(radius, positions, indices);
  TesselateIcosahedron(radius, positions, indices, 4);

  std::vector<glm::vec2> uv(positions.size());
  for (size_t i = 0; i < positions.size(); i++)
    uv[i] = GetIcosahedronUV(positions[i]);

  FixWrappedTriangles(uv, positions, indices);

  bool failed = false;
  ForEachAttributeWithCheck(
      componentsMask,
      [&meshGroup, &failed, &positions, &uv](MeshVertexAttribute attr) {
        if (attr == MeshVertexAttribute::Position)
        {
          for (size_t i = 0; i < positions.size(); i++)
            meshGroup.m_boundingBox.extend(positions[i]);

          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], positions);
        }
        else if (attr == MeshVertexAttribute::Normal)
        {
          std::vector<glm::vec3> normals(positions.size());
          for (size_t i = 0; i < normals.size(); i++)
            normals[i] = glm::normalize(positions[i]);

          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], normals);
        }
        else if (attr == MeshVertexAttribute::Tangent)
        {
          std::vector<glm::vec3> tangents(positions.size());
          for (size_t i = 0; i < tangents.size(); i++)
          {
            glm::vec3 n = glm::normalize(positions[i]);
            if (fabs(n.y + 1.0) < kEps)
              tangents[i] = glm::vec3(-1, 0, 0);
            else if (fabs(n.y - 1.0) < kEps)
              tangents[i] = glm::vec3(1, 0, 0);
            else
              tangents[i] = glm::vec3(0, 1, 0) * n;
          }
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], tangents);
        }
        else if (attr == MeshVertexAttribute::UV0)
        {
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], uv);
        }
        else
        {
          failed = true;
          Logger::ToLog(Logger::Error,
            "Can't generate sphere, components mask contains unsupported attributes.");
          return false;
        }

        return true;
      });

  if (!failed)
  {
    meshGroup.m_groupIndex = 0;
    meshGroup.m_verticesCount = static_cast<uint32_t>(positions.size());
    meshGroup.m_indexBuffer = std::move(indices);
    meshGroup.m_indicesCount = static_cast<uint32_t>(meshGroup.m_indexBuffer.size());
  }

  return !failed;
}

bool MeshGenerator::GeneratePlane(float width, float height, uint32_t componentsMask,
                                  BaseMesh::MeshGroup & meshGroup, uint32_t widthSegments,
                                  uint32_t heightSegments, uint32_t uSegments, uint32_t vSegments)
{
  if (componentsMask == 0)
  {
    Logger::ToLog(Logger::Error, "Can't generate plane, components mask is invalid.");
    return false;
  }

  if (width <= 0 || height <= 0 || widthSegments <= 0 || heightSegments <= 0 || uSegments <= 0 ||
      vSegments <= 0)
  {
    Logger::ToLog(Logger::Error, "Can't generate plane, plane parameters are invalid.");
    return false;
  }

  std::vector<glm::vec3> positions;
  std::vector<glm::vec2> uv;
  std::vector<uint32_t> indices;
  InitPlane(width, height, widthSegments, heightSegments, uSegments, vSegments, positions, uv,
            indices);

  bool failed = false;
  ForEachAttributeWithCheck(
      componentsMask,
      [&meshGroup, &failed, &positions, &uv](MeshVertexAttribute attr) {
        if (attr == MeshVertexAttribute::Position)
        {
          for (auto const & p : positions)
            meshGroup.m_boundingBox.extend(p);

          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], positions);
        }
        else if (attr == MeshVertexAttribute::Normal)
        {
          std::vector<glm::vec3> normals(positions.size());
          for (auto & n : normals)
            n = glm::vec3(0, 1, 0);
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], normals);
        }
        else if (attr == MeshVertexAttribute::Tangent)
        {
          std::vector<glm::vec3> tangents(positions.size());
          for (auto & t : tangents)
            t = glm::vec3(1, 0, 0);
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], tangents);
        }
        else if (attr == MeshVertexAttribute::UV0)
        {
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], uv);
        }
        else
        {
          failed = true;
          Logger::ToLog(Logger::Error,
            "Can't generate sphere, components mask contains unsupported attributes.");
          return false;
        }

        return true;
      });

  if (!failed)
  {
    meshGroup.m_groupIndex = 0;
    meshGroup.m_verticesCount = static_cast<uint32_t>(positions.size());
    meshGroup.m_indexBuffer = std::move(indices);
    meshGroup.m_indicesCount = static_cast<uint32_t>(meshGroup.m_indexBuffer.size());
  }

  return !failed;
}

bool MeshGenerator::GenerateTerrain(std::vector<uint8_t> const & heightmap,
                                    uint32_t heightmapWidth, uint32_t heightmapHeight,
                                    uint32_t componentsMask, float minAltitude, float maxAltitude,
                                    float width, float height, BaseMesh::MeshGroup & meshGroup)
{
  std::vector<glm::vec3> positions;
  positions.reserve(heightmapWidth * heightmapHeight);
  float const tileSizeX = width / heightmapWidth;
  float const tileSizeY = height / heightmapHeight;

  auto isPivot = [heightmapHeight, heightmapWidth](int i, int j)
  {
    if (i == 0 && j == 0)
      return true;
    if (i == 0 && j + 1 == heightmapWidth)
      return true;
    if (i + 1 == heightmapHeight && j == 0)
      return true;
    if (i + 1 == heightmapHeight && j + 1 == heightmapWidth)
      return true;

    auto const di = heightmapHeight / 4;
    auto const dj = heightmapWidth / 4;
    if (i % di == 0 && j % dj == 0)
      return true;

    if (i % di == 0 && j + 1 == heightmapWidth)
      return true;
    if (i + 1 == heightmapHeight && j % dj == 0)
      return true;

    return false;
  };

  auto isValidOffset = [heightmapHeight, heightmapWidth](int i, int j, int xoff, int yoff)
  {
    i += yoff;
    j += xoff;
    return i >= 0 && j >= 0 && i < heightmapHeight && j < heightmapWidth;
  };

  uint32_t const kTolerance = 0;
  for (int i = 0; i < static_cast<int>(heightmapHeight); ++i)
  {
    auto const y = tileSizeY * (i - static_cast<int>(heightmapHeight) / 2);
    for (int j = 0; j < static_cast<int>(heightmapWidth); ++j)
    {
      // Skip points around which the similar values.
      if (!isPivot(i, j))
      {
        static std::vector<std::pair<int, int>> const kOffsets = {{-1, -1}, {0, -1}, {1, -1},
                                                                  {-1, 0}, {1, 0},
                                                                  {-1, 1}, {0, 1}, {1, 1}};
        bool diff = false;
        auto const val = heightmap[i * heightmapWidth + j];
        for (auto const & [xoff, yoff] : kOffsets)
        {
          if (!isValidOffset(i, j, xoff, yoff))
            continue;
          auto const valNeighbour = heightmap[(i + yoff) * heightmapWidth + (j + xoff)];
          if (abs(val - valNeighbour) > kTolerance)
          {
            diff = true;
            break;
          }
        }
        if (!diff)
          continue;
      }

      auto const x = tileSizeX * (j - static_cast<int>(heightmapWidth) / 2);
      auto const z = glm::mix(minAltitude, maxAltitude,
                           static_cast<float>(heightmap[i * heightmapWidth + j]) / 255.0f);
      positions.emplace_back(x, z, y);
    }
  }

  return GenerateTerrain(positions, {}, componentsMask, meshGroup);
}

bool MeshGenerator::GenerateTerrain(std::vector<glm::vec3> const & inputPositions,
                                    std::vector<glm::vec2> const & borders, uint32_t componentsMask,
                                    BaseMesh::MeshGroup & meshGroup)
{
  auto mergeNormals = [](glm::vec3 const & n1, glm::vec3 const & n2, glm::vec3 const & pos)
  {
    float constexpr kEps = 1e-7f;
    float constexpr kThreshold = 0.9999f;
    if (fabs(n1.y) >= kThreshold || fabs(n2.y) >= kThreshold || fabs(pos.y) < kEps)
      return glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::normalize(n1 + n2);
  };

  std::vector<double> coords;
  coords.reserve(inputPositions.size() * 2);
  for (auto const & p : inputPositions)
  {
    coords.emplace_back(static_cast<double>(p.x));
    coords.emplace_back(static_cast<double>(p.z));
  }
  delaunator::Delaunator d(coords);

  std::vector<uint32_t> initialIndices;
  initialIndices.reserve(d.triangles.size());
  for (size_t i = 0; i < d.triangles.size(); i += 3)
  {
    auto const p1 = glm::vec2(inputPositions[d.triangles[i]].x,
                              inputPositions[d.triangles[i]].z);
    auto const p2 = glm::vec2(inputPositions[d.triangles[i + 1]].x,
                              inputPositions[d.triangles[i + 1]].z);
    auto const p3 = glm::vec2(inputPositions[d.triangles[i + 2]].x,
                              inputPositions[d.triangles[i + 2]].z);
    auto const center = (p1 + p2 + p3) / 3.0f;
    if (IsPointInside(borders, center))
    {
      initialIndices.push_back(d.triangles[i]);
      initialIndices.push_back(d.triangles[i + 1]);
      initialIndices.push_back(d.triangles[i + 2]);
    }
  }

  MeshSimplifier::MeshData simplifierData;
  simplifierData.m_positions = inputPositions;
  simplifierData.m_indices = std::move(initialIndices);
  MeshSimplifier simplifier(simplifierData);
  auto result = simplifier.Simplify(100000, 5.0);
  auto positions = std::move(result.m_positions);
  std::vector<uint32_t> indices = std::move(result.m_indices);

  std::vector<glm::vec2> uv;
  uv.reserve(positions.size());
  AABB box;
  for (auto const & p : positions)
    box.extend(p);
  auto const w = box.getMax().x - box.getMin().x;
  auto const h = box.getMax().z - box.getMin().z;
  for (auto const & p : positions)
    uv.emplace_back((p.x - box.getMin().x) / w, (p.z - box.getMin().z) / h);

  std::vector<glm::vec3> normals(positions.size());
  std::vector<glm::vec3> tangents(positions.size());
  for (size_t i = 0; i < indices.size(); i += 3)
  {
    auto const v1 = glm::normalize(positions[indices[i + 1]] - positions[indices[i]]);
    auto const v2 = glm::normalize(positions[indices[i + 2]] - positions[indices[i]]);
    auto const n = glm::cross(v1, v2);
    normals[indices[i]] = mergeNormals(n, normals[indices[i]], positions[indices[i]]);
    normals[indices[i + 1]] = mergeNormals(n, normals[indices[i + 1]], positions[indices[i + 1]]);
    normals[indices[i + 2]] = mergeNormals(n, normals[indices[i + 2]], positions[indices[i + 2]]);

    auto const t = glm::cross(n, glm::vec3(0.0f, 0.0f, 1.0f));
    tangents[indices[i]] += t;
    tangents[indices[i + 1]] += t;
    tangents[indices[i + 2]] += t;
  }

  for (size_t i = 0; i < normals.size(); ++i)
  {
    normals[i] = glm::normalize(normals[i]);
    tangents[i] = glm::normalize(tangents[i]);
  }

  bool failed = false;
  ForEachAttributeWithCheck(
    componentsMask,
    [&meshGroup, &failed, &positions, &uv, &normals, &tangents](MeshVertexAttribute attr) {
      if (attr == MeshVertexAttribute::Position)
      {
        for (auto const & p : positions)
          meshGroup.m_boundingBox.extend(p);

        CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], positions);
      }
      else if (attr == MeshVertexAttribute::Normal)
      {
        CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], normals);
      }
      else if (attr == MeshVertexAttribute::Tangent)
      {
        CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], tangents);
      }
      else if (attr == MeshVertexAttribute::UV0)
      {
        CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], uv);
      }
      else
      {
        failed = true;
        Logger::ToLog(Logger::Error,
                      "Can't generate landscape, components mask contains unsupported attributes.");
        return false;
      }

      return true;
    });

  if (!failed)
  {
    meshGroup.m_groupIndex = 0;
    meshGroup.m_verticesCount = static_cast<uint32_t>(positions.size());
    meshGroup.m_indexBuffer = std::move(indices);
    meshGroup.m_indicesCount = static_cast<uint32_t>(meshGroup.m_indexBuffer.size());
  }

  return !failed;
}
}  // namespace rf
