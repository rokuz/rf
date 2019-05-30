#pragma once

#include "common.hpp"

namespace rf
{
using MaterialColor = std::optional<glm::vec3>;
MaterialColor const kInvalidColor = MaterialColor();

struct MeshMaterial
{
  std::string m_diffuseTexture;
  std::string m_normalsTexture;
  std::string m_specularTexture;
  MaterialColor m_diffuseColor = kInvalidColor;
  MaterialColor m_specularColor = kInvalidColor;
  MaterialColor m_ambientColor = kInvalidColor;

  bool IsValid() const
  {
    return !m_diffuseTexture.empty() ||
           !m_normalsTexture.empty() ||
           !m_specularTexture.empty() ||
           m_diffuseColor || m_specularColor || m_ambientColor;
  }
};

using MaterialCollection = std::unordered_map<uint32_t, std::shared_ptr<MeshMaterial>>;

enum MeshVertexAttribute : uint32_t
{
  Position = 1 << 0,
  Normal = 1 << 1,
  UV0 = 1 << 2,
  UV1 = 1 << 3,
  UV2 = 1 << 4,
  UV3 = 1 << 5,
  Tangent = 1 << 6,
  Color = 1 << 7,
  BoneIndices = 1 << 8,
  BoneWeights = 1 << 9
};

enum class MeshAttributeUnderlyingType : uint8_t
{
  Float,
  UnsignedInteger
};

std::string const kAttributesNames[] = {"aPosition",    "aNormal",     "aUV0",     "aUV1",
                                        "aUV2",         "aUV3",        "aTangent", "aColor",
                                        "aBoneIndices", "aBoneWeights"};

MeshVertexAttribute const kAllAttributes[] = {Position, Normal,  UV0,   UV1,         UV2,
                                              UV3,      Tangent, Color, BoneIndices, BoneWeights};
uint32_t constexpr kAttributesCount = sizeof(kAllAttributes) / sizeof(kAllAttributes[0]);

uint32_t constexpr kMaxBonesNumber = 64;
uint32_t constexpr kMaxBonesPerVertex = 4;

using IndexBuffer32 = std::vector<uint32_t>;
using VertexBufferCollection = std::unordered_map<MeshVertexAttribute, ByteArray>;

struct BoneAnimation
{
  uint32_t m_boneIndex = 0;
  std::vector<std::pair<double, glm::vec3>> m_translationKeys;
  std::vector<std::pair<double, glm::vec3>> m_scaleKeys;
  std::vector<std::pair<double, glm::quat>> m_rotationKeys;
};

struct MeshAnimation
{
  std::string m_name;
  double m_durationInTicks = 0.0;
  double m_ticksPerSecond = 0.0;
  std::vector<BoneAnimation> m_boneAnimations;
};

using MeshAnimations = std::vector<std::unique_ptr<MeshAnimation>>;

using BoneIndicesCollection = std::unordered_map<std::string, uint32_t>;

class BaseMesh
{
public:
  BaseMesh();
  virtual ~BaseMesh();

  int GetGroupsCount() const { return m_groupsCount; }
  glm::mat4x4 GetGroupTransform(int index, glm::mat4x4 const & transform) const;
  AABB const & GetGroupBoundingBox(int index) const;
  std::shared_ptr<MeshMaterial> GetGroupMaterial(int index) const;
  AABB GetBoundingBox() const;
  size_t GetAnimationsCount() const;
  void GetBonesTransforms(int groupIndex, size_t animIndex, double timeSinceStart, bool cycled,
                          std::vector<glm::mat4x4> & bonesTransforms);
  uint32_t GetAttributesMask() const { return m_attributesMask; }

  struct MeshGroup
  {
    VertexBufferCollection m_vertexBuffers;
    IndexBuffer32 m_indexBuffer;
    AABB m_boundingBox;
    int m_groupIndex = -1;
    uint32_t m_verticesCount = 0;
    uint32_t m_indicesCount = 0;
    uint32_t m_startIndex = 0;
    int m_materialIndex = -1;
    std::unordered_map<uint32_t, glm::mat4x4> m_boneOffsets;
  };

  struct MeshNode
  {
    std::string m_name;
    std::vector<MeshGroup> m_groups;
    glm::mat4x4 m_transform;
    std::vector<std::unique_ptr<MeshNode>> m_children;
  };

protected:
  bool LoadMesh(std::string && filename, uint32_t desiredAttributesMask);
  bool GenerateSphere(float radius, uint32_t attributesMask = Position | Normal | UV0 | Tangent);
  bool GeneratePlane(float width, float height, uint32_t widthSegments = 1,
                     uint32_t heightSegments = 1, uint32_t uSegments = 1, uint32_t vSegments = 1,
                     uint32_t attributesMask = Position | Normal | UV0 | Tangent);
  void DestroyMesh();

  glm::mat4x4 FindBoneAnimation(uint32_t boneIndex, size_t animIndex, double animTime, bool & found);
  void CalculateBonesTransform(size_t animIndex, double animTime, const BaseMesh::MeshGroup & group,
                               std::unique_ptr<BaseMesh::MeshNode> const & meshNode,
                               glm::mat4x4 const & parentTransform,
                               std::vector<glm::mat4x4> & bonesTransforms);

  void FillGpuBuffers(std::unique_ptr<BaseMesh::MeshNode> const & meshNode,
                      uint8_t * vbPtr, uint32_t * ibPtr,
                      uint32_t & vbOffset, uint32_t & ibOffset,
                      bool fillIndexBuffer, uint32_t attributesMask);

  MeshGroup const & FindMeshGroup(std::unique_ptr<BaseMesh::MeshNode> const & meshNode,
                                  int index) const;
  MeshGroup const & FindCachedMeshGroup(int index) const;

  uint32_t m_verticesCount = 0;
  uint32_t m_indicesCount = 0;
  uint32_t m_attributesMask = 0;
  int m_groupsCount = 0;
  
  MaterialCollection m_materials;
  MeshAnimations m_animations;
  BoneIndicesCollection m_bonesIndices;
  
  std::unique_ptr<MeshNode> m_rootNode;
  std::unique_ptr<MeshNode> m_bonesRootNode;
  
  mutable std::vector<MeshGroup const *> m_groupsCache;
};

extern void ForEachAttribute(uint32_t attributesMask,
                             std::function<void(MeshVertexAttribute)> const & func);
extern void ForEachAttributeWithCheck(uint32_t attributesMask,
                                      std::function<bool(MeshVertexAttribute)> const & func);

extern uint32_t GetAttributeElementsCount(MeshVertexAttribute attr);
extern uint32_t GetAttributeSizeInBytes(MeshVertexAttribute attr);
extern uint32_t GetAttributeOffsetInBytes(uint32_t attributesMask, MeshVertexAttribute attr);
extern MeshAttributeUnderlyingType GetAttributeUnderlyingType(MeshVertexAttribute attr);

extern uint32_t GetVertexSizeInBytes(uint32_t attributesMask);
}  // namespace rf
