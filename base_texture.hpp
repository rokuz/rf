#pragma once

#include "common.hpp"

namespace rf
{
enum class TextureType : uint8_t
{
  Texture2D,
  Array2D,
  Cubemap,
  Heightmap,
};

enum class TextureFormat : uint8_t
{
  Unspecified,
  R8,
  RG8,
  RGB8,
  RGBA8,
  DepthStencil,
  Depth
};

enum class CubemapSide : uint8_t
{
  Right = 0,
  Left,
  Top,
  Bottom,
  Front,
  Back
};

class BaseTexture
{
public:
  BaseTexture() = default;
  explicit BaseTexture(std::string const & id) : m_id(id) {}
  virtual ~BaseTexture() = default;

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }

  void SetId(std::string const & id) { m_id = id; }
  std::string GetId() const { return m_id; }

  static void SaveToPng(std::string && filename, uint32_t width, uint32_t height,
                        TextureFormat format, uint8_t const * data);

protected:
  uint8_t const * Load(std::string && fileName);

  std::vector<uint8_t const *> LoadCubemap(std::string && rightFileName,
                                           std::string && leftFileName,
                                           std::string && topFileName,
                                           std::string && bottomFileName,
                                           std::string && frontFileName,
                                           std::string && backFileName);

  std::vector<uint8_t const *> LoadArray(std::vector<std::string> && filenames);

  std::vector<uint8_t> LoadHeightmap(std::string && fileName);

  void FreeLoadedData(uint8_t const * imageData);
  int CalculateMipLevelsCount() const;

  std::string m_id;

  TextureType m_type = TextureType::Texture2D;
  TextureFormat m_format = TextureFormat::Unspecified;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  size_t m_arraySize = 0;
};
}  // namespace rf
