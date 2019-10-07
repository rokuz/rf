// The code is based on https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification
#pragma once

#include "common.hpp"

namespace rf
{
class MeshSimplifier
{
  class SymmetricMatrix
  {
  public:
    SymmetricMatrix() = default;

    explicit SymmetricMatrix(double val)
    {
      m_data.fill(val);
    }

    SymmetricMatrix(double m11, double m12, double m13, double m14,
                    double m22, double m23, double m24,
                    double m33, double m34,
                    double m44) {
      m_data[0] = m11; m_data[1] = m12; m_data[2] = m13;  m_data[3] = m14;
      m_data[4] = m22; m_data[5] = m23; m_data[6] = m24;
      m_data[7] = m33; m_data[8] = m34;
      m_data[9] = m44;
    }

    // Construction from a plane.
    SymmetricMatrix(double a, double b, double c, double d)
    {
      m_data[0] = a * a; m_data[1] = a * b; m_data[2] = a * c; m_data[3] = a * d;
      m_data[4] = b * b; m_data[5] = b * c; m_data[6] = b * d;
      m_data[7] = c * c; m_data[8] = c * d;
      m_data[9] = d * d;
    }

    double operator[](int index) const { return m_data[index]; }

    double det(int a11, int a12, int a13, int a21, int a22, int a23, int a31, int a32, int a33) const
    {
      auto const det =
          m_data[a11] * m_data[a22] * m_data[a33] + m_data[a13] * m_data[a21] * m_data[a32] +
          m_data[a12] * m_data[a23] * m_data[a31] - m_data[a13] * m_data[a22] * m_data[a31] -
          m_data[a11] * m_data[a23] * m_data[a32] - m_data[a12] * m_data[a21] * m_data[a33];
      return det;
    }

    SymmetricMatrix operator+(SymmetricMatrix const & n) const
    {
      return SymmetricMatrix(m_data[0] + n.m_data[0], m_data[1] + n.m_data[1],
                             m_data[2] + n.m_data[2], m_data[3] + n.m_data[3],
                             m_data[4] + n.m_data[4], m_data[5] + n.m_data[5],
                             m_data[6] + n.m_data[6], m_data[7] + n.m_data[7],
                             m_data[8] + n.m_data[8], m_data[9] + n.m_data[9]);
    }

    SymmetricMatrix & operator+=(SymmetricMatrix const & n)
    {
      for (size_t i = 0; i < m_data.size(); ++i)
        m_data[i] += n.m_data[i];
      return *this;
    }

  private:
    std::array<double, 10> m_data = {};
  };

  struct Triangle
  {
    std::array<uint32_t, 3> m_indices;
    std::array<double, 4> m_errors;
    bool m_isDeleted = false;
    bool m_isDirty = false;
    glm::vec3 m_normal;
  };

  struct Vertex
  {
    glm::vec3 m_position;
    uint32_t m_startTriangleRef = 0;
    uint32_t m_triangleRefsCount = 0;
    SymmetricMatrix m_quadrics;
    bool m_isBorder = false;
  };

  struct Ref
  {
    uint32_t m_triangleIndex = 0;
    uint32_t m_triangleVertex = 0;
  };

public:
  struct MeshData
  {
    std::vector<glm::vec3> m_positions;
    std::vector<uint32_t> m_indices;
  };

  explicit MeshSimplifier(MeshData const & meshData)
  {
    m_vertices.resize(meshData.m_positions.size());
    for (uint32_t i = 0; i < m_vertices.size(); ++i)
      m_vertices[i].m_position = meshData.m_positions[i];

    m_triangles.resize(meshData.m_indices.size() / 3);
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_triangles.size()); ++i)
    {
      auto & t = m_triangles[i];
      glm::vec3 p[3];
      for (uint32_t j = 0; j < 3; ++j)
      {
        t.m_indices[j] = meshData.m_indices[i * 3 + j];
        p[j] = m_vertices[t.m_indices[j]].m_position;
      }

      auto const n = glm::cross(p[1] - p[0], p[2] - p[0]);
      t.m_normal = glm::normalize(n);
      for (uint32_t j = 0; j < 3; ++j)
        m_vertices[t.m_indices[j]].m_quadrics += SymmetricMatrix(n.x, n.y, n.z, glm::dot(-n, p[0]));
    }

    for (auto & t : m_triangles)
    {
      glm::vec3 p;
      for (uint32_t j = 0; j < 3; ++j)
        t.m_errors[j] = CalculateEdgeError(t.m_indices[j], t.m_indices[(j + 1) % 3], p);
      t.m_errors[3] = std::min(t.m_errors[0], std::min(t.m_errors[1], t.m_errors[2]));
    }

    UpdateMesh();

    for (auto & v : m_vertices)
      v.m_isBorder = false;

    std::vector<int> vcount, vids;
    for (auto & v : m_vertices)
    {
      vcount.clear();
      vids.clear();
      for (uint32_t j = 0; j < v.m_triangleRefsCount; ++j)
      {
        auto & t = m_triangles[m_refs[v.m_startTriangleRef + j].m_triangleIndex];
        for (uint32_t k = 0; k < 3; ++k)
        {
          uint32_t ofs = 0;
          auto const id = t.m_indices[k];
          while (ofs < vcount.size())
          {
            if (vids[ofs] == id)
              break;
            ofs++;
          }
          if (ofs == vcount.size())
          {
            vcount.emplace_back(1);
            vids.emplace_back(id);
          }
          else
          {
            vcount[ofs]++;
          }
        }
      }
      for (uint32_t j = 0; j < static_cast<uint32_t>(vcount.size()); ++j)
      {
        if (vcount[j] == 1)
          m_vertices[vids[j]].m_isBorder = true;
      }
    }
  }

  MeshData Simplify(int targetCount, double aggressiveness = 7.0, uint32_t maxIterationsCount = 1000)
  {
    for (auto & t : m_triangles)
      t.m_isDeleted = false;

    uint32_t deletedTriangles = 0;
    auto const triangleCount = static_cast<uint32_t>(m_triangles.size());
    for (uint32_t iteration = 0; iteration < maxIterationsCount; ++iteration)
    {
      if (triangleCount <= targetCount + deletedTriangles)
        break;

      double const threshold = 0.000000001 * pow(double(iteration + 3), aggressiveness);
      CollapseEdges(threshold, deletedTriangles);
    }
    CompactMesh();
    return BuildMeshData();
  }

  MeshData Simplify(double threshold, uint32_t maxIterationsCount = 1000)
  {
    for (auto & t : m_triangles)
      t.m_isDeleted = false;

    for (uint32_t iteration = 0; iteration < maxIterationsCount; ++iteration)
    {
      uint32_t deletedTriangles = 0;
      CollapseEdges(threshold, deletedTriangles);
      if (deletedTriangles == 0)
        break;
    }
    CompactMesh();
    return BuildMeshData();
  }

private:
  void CollapseEdges(double threshold, uint32_t & deletedTriangles)
  {
    std::vector<bool> deleted0, deleted1;

    UpdateMesh();

    for (auto & t : m_triangles)
      t.m_isDirty = false;

    for (auto & t : m_triangles)
    {
      if (t.m_errors[3] > threshold || t.m_isDeleted || t.m_isDirty)
        continue;

      for (uint32_t j = 0; j < 3; ++j)
      {
        if (t.m_errors[j] > threshold)
          continue;

        auto const i0 = t.m_indices[j];
        auto & v0 = m_vertices[i0];
        auto const i1 = t.m_indices[(j + 1) % 3];
        auto & v1 = m_vertices[i1];

        // Border check.
        if (v0.m_isBorder != v1.m_isBorder)
          continue;

        // Compute vertex to collapse to.
        glm::vec3 p;
        CalculateEdgeError(i0, i1, p);
        deleted0.resize(v0.m_triangleRefsCount);
        deleted1.resize(v1.m_triangleRefsCount);
        if (IsFlipped(p, i0, i1, v0, v1, deleted0) || IsFlipped(p, i1, i0, v1, v0, deleted1))
          continue;

        v0.m_position = p;
        v0.m_quadrics += v1.m_quadrics;
        auto const startTriangleRef = static_cast<uint32_t>(m_refs.size());

        UpdateTriangles(i0, v0, deleted0, deletedTriangles);
        UpdateTriangles(i0, v1, deleted1, deletedTriangles);

        auto const triangleRefsCount = static_cast<uint32_t>(m_refs.size()) - startTriangleRef;
        if (triangleRefsCount <= v0.m_triangleRefsCount)
        {
          if (triangleRefsCount != 0)
            memcpy(&m_refs[v0.m_startTriangleRef], &m_refs[startTriangleRef], triangleRefsCount * sizeof(Ref));
        }
        else
        {
          v0.m_startTriangleRef = startTriangleRef;
        }

        v0.m_triangleRefsCount = triangleRefsCount;
        break;
      }
    }
  }

  MeshData BuildMeshData() const
  {
    MeshData meshData;
    meshData.m_positions.resize(m_vertices.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(meshData.m_positions.size()); ++i)
      meshData.m_positions[i] = m_vertices[i].m_position;

    meshData.m_indices.resize(m_triangles.size() * 3);
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_triangles.size()); ++i)
    {
      for (uint32_t j = 0; j < 3; ++j)
        meshData.m_indices[i * 3 + j] = m_triangles[i].m_indices[j];
    }
    return meshData;
  }

  // Check if a triangle flips when this edge is removed.
  bool IsFlipped(glm::vec3 const & p, int i0, int i1, Vertex & v0, Vertex & v1,
                 std::vector<bool> & deleted) const
  {
    for (uint32_t k = 0; k < v0.m_triangleRefsCount; ++k)
    {
      auto & t = m_triangles[m_refs[v0.m_startTriangleRef + k].m_triangleIndex];
      if (t.m_isDeleted)
        continue;

      auto const s = m_refs[v0.m_startTriangleRef + k].m_triangleVertex;
      auto const id1 = t.m_indices[(s + 1) % 3];
      auto const id2 = t.m_indices[(s + 2) % 3];

      if (id1 == i1 || id2 == i1)
      {
        deleted[k] = true;
        continue;
      }

      auto const d1 = glm::normalize(m_vertices[id1].m_position - p);
      auto const d2 = glm::normalize(m_vertices[id2].m_position - p);
      if (fabs(glm::dot(d1, d2)) > 0.999)
        return true;

      auto const n = glm::cross(d1, d2);
      deleted[k] = false;
      if (glm::dot(n, t.m_normal) < 0.05)
        return true;
    }
    return false;
  }

  // Update triangle connections and edge error after an edge is collapsed.
  void UpdateTriangles(uint32_t i0, Vertex & v, std::vector<bool> & deleted,
                       uint32_t & deletedTriangles)
  {
    glm::vec3 p;
    for (uint32_t k = 0; k < v.m_triangleRefsCount; ++k)
    {
      auto const & r = m_refs[v.m_startTriangleRef + k];
      Triangle & t = m_triangles[r.m_triangleIndex];
      if (t.m_isDeleted)
        continue;

      if (deleted[k])
      {
        t.m_isDeleted = true;
        deletedTriangles++;
        continue;
      }
      t.m_indices[r.m_triangleVertex] = i0;
      t.m_isDirty = true;
      t.m_errors[0] = CalculateEdgeError(t.m_indices[0], t.m_indices[1], p);
      t.m_errors[1] = CalculateEdgeError(t.m_indices[1], t.m_indices[2], p);
      t.m_errors[2] = CalculateEdgeError(t.m_indices[2], t.m_indices[0], p);
      t.m_errors[3] = std::min(t.m_errors[0], std::min(t.m_errors[1], t.m_errors[2]));
      m_refs.push_back(r);
    }
  }

  // Compact triangles, compute edge error and build reference list.
  void UpdateMesh()
  {
    int dst = 0;
    for (auto & t : m_triangles)
    {
      if (!t.m_isDeleted)
        m_triangles[dst++] = t;
    }
    m_triangles.resize(dst);

    // Init reference ids list.
    for (auto & v : m_vertices)
    {
      v.m_startTriangleRef = 0;
      v.m_triangleRefsCount = 0;
    }
    for (auto & t : m_triangles)
    {
      for (uint32_t j = 0; j < 3; ++j)
        m_vertices[t.m_indices[j]].m_triangleRefsCount++;
    }
    uint32_t startTriangleRef = 0;
    for (auto & v : m_vertices)
    {
      v.m_startTriangleRef = startTriangleRef;
      startTriangleRef += v.m_triangleRefsCount;
      v.m_triangleRefsCount = 0;
    }

    // Write references.
    m_refs.resize(m_triangles.size() * 3);
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_triangles.size()); ++i)
    {
      auto & t = m_triangles[i];
      for (uint32_t j = 0; j < 3; ++j)
      {
        auto & v = m_vertices[t.m_indices[j]];
        m_refs[v.m_startTriangleRef + v.m_triangleRefsCount].m_triangleIndex = i;
        m_refs[v.m_startTriangleRef + v.m_triangleRefsCount].m_triangleVertex = j;
        v.m_triangleRefsCount++;
      }
    }
  }

  void CompactMesh()
  {
    uint32_t dst = 0;

    for (auto & v : m_vertices)
      v.m_triangleRefsCount = 0;

    for (auto & t : m_triangles)
    {
      if (t.m_isDeleted)
        continue;

      m_triangles[dst++] = t;
      for (auto & index : t.m_indices)
        m_vertices[index].m_triangleRefsCount = 1;
    }
    m_triangles.resize(dst);

    dst = 0;
    for (auto & v : m_vertices)
    {
      if (v.m_triangleRefsCount == 0)
        continue;

      v.m_startTriangleRef = dst;
      m_vertices[dst].m_position = v.m_position;
      dst++;
    }

    for (auto & t : m_triangles)
    {
      for (auto & index : t.m_indices)
        index = m_vertices[index].m_startTriangleRef;
    }
    m_vertices.resize(dst);
  }

  // Calculate error between vertex and quadric.
  double CalculateVertexError(SymmetricMatrix const & q, glm::vec3 const & p) const
  {
    return q[0] * p.x * p.x + 2 * q[1] * p.x * p.y + 2 * q[2] * p.x * p.z + 2 * q[3] * p.x +
           q[4] * p.y * p.y + 2 * q[5] * p.y * p.z + 2 * q[6] * p.y + q[7] * p.z * p.z +
           2 * q[8] * p.z + q[9];
  }

  // Error for an edge.
  double CalculateEdgeError(uint32_t idv1, uint32_t idv2, glm::vec3 & result) const
  {
    // Compute interpolated vertex.
    auto const q = m_vertices[idv1].m_quadrics + m_vertices[idv2].m_quadrics;
    bool const border = m_vertices[idv1].m_isBorder && m_vertices[idv2].m_isBorder;
    auto const det = q.det(0, 1, 2, 1, 4, 5, 2, 5, 7);
    if (det != 0 && !border)
    {
      // q_delta is invertible.
      result.x = static_cast<float>(-1.0 / det * (q.det(1, 2, 3, 4, 5, 6, 5, 7, 8)));  // A41 / det(q_delta).
      result.y = static_cast<float>(1.0 / det * (q.det(0, 2, 3, 1, 5, 6, 2, 7, 8)));  // A42 / det(q_delta).
      result.z = static_cast<float>(-1.0 / det * (q.det(0, 1, 3, 1, 4, 6, 2, 5, 8)));  // A43 / det(q_delta).
      return CalculateVertexError(q, result);
    }

    // det = 0 -> try to find best result.
    auto const p1 = m_vertices[idv1].m_position;
    auto const p2 = m_vertices[idv2].m_position;
    auto const p3 = (p1 + p2) * 0.5f;
    auto const error1 = CalculateVertexError(q, p1);
    auto const error2 = CalculateVertexError(q, p2);
    auto const error3 = CalculateVertexError(q, p3);
    auto const error = std::min(error1, std::min(error2, error3));
    if (error1 == error)
      result = p1;
    if (error2 == error)
      result = p2;
    if (error3 == error)
      result = p3;
    return error;
  }

  std::vector<Triangle> m_triangles;
  std::vector<Vertex> m_vertices;
  std::vector<Ref> m_refs;
};
}  // namespace rf
