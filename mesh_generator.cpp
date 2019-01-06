#include "mesh_generator.hpp"

#include "logger.hpp"

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
void CopyToVertexBuffer(TVertexBuffer & vb, std::vector<T> const & v)
{
  size_t const sz = v.size() * sizeof(T);
  vb.resize(sz);
  memcpy(vb.data(), v.data(), sz);
}
}  // namespace

bool MeshGenerator::GenerateSphere(float radius, uint32_t componentsMask,
                                   Mesh::MeshGroup & meshGroup)
{
  if (componentsMask == 0)
  {
    Logger::ToLog("Error: Can't generate sphere, components mask is invalid.\n");
    return false;
  }

  if (radius <= 0)
  {
    Logger::ToLog("Error: Can't generate sphere, radius must be more than 0.\n");
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
      [&meshGroup, &failed, &positions, &indices, &uv](MeshVertexAttribute attr) {
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
            if (fabs(n.y + 1.0) < 1e-5)
              tangents[i] = glm::vec3(-1, 0, 0);
            else if (fabs(n.y - 1.0) < 1e-5)
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
          Logger::ToLog(
              "Error: Can't generate sphere, components mask contains unsupported attributes.\n");
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
                                  Mesh::MeshGroup & meshGroup, uint32_t widthSegments,
                                  uint32_t heightSegments, uint32_t uSegments, uint32_t vSegments)
{
  if (componentsMask == 0)
  {
    Logger::ToLog("Error: Can't generate plane, components mask is invalid.\n");
    return false;
  }

  if (width <= 0 || height <= 0 || widthSegments <= 0 || heightSegments <= 0 || uSegments <= 0 ||
      vSegments <= 0)
  {
    Logger::ToLog("Error: Can't generate plane, plane parameters are invalid.\n");
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
      [&meshGroup, &failed, &positions, &indices, &uv](MeshVertexAttribute attr) {
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
            normals[i] = glm::vec3(0, 1, 0);
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], normals);
        }
        else if (attr == MeshVertexAttribute::Tangent)
        {
          std::vector<glm::vec3> tangents(positions.size());
          for (size_t i = 0; i < tangents.size(); i++)
            tangents[i] = glm::vec3(1, 0, 0);
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], tangents);
        }
        else if (attr == MeshVertexAttribute::UV0)
        {
          CopyToVertexBuffer(meshGroup.m_vertexBuffers[attr], uv);
        }
        else
        {
          failed = true;
          Logger::ToLog(
              "Error: Can't generate sphere, components mask contains unsupported attributes.\n");
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
