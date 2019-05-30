#include "base_texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "rf.hpp"

namespace rf
{
namespace
{
TextureFormat FindFormat(int components)
{
  switch (components)
  {
  case 1:
    return TextureFormat::R8;
  case 2:
    return TextureFormat::RG8;
  case 3:
    return TextureFormat::RGB8;
  case 4:
    return TextureFormat::RGBA8;
  }
  return TextureFormat::Unspecified;
}

int GetChannelsCount(TextureFormat format)
{
  switch (format)
  {
  case TextureFormat::R8 : return 1;
  case TextureFormat::RG8 : return 2;
  case TextureFormat::RGB8 : return 3;
  case TextureFormat::RGBA8 : return 4;
  default:
    CHECK(false, ("Invalid texture format."));
  }
  return 0;
}
}  // namespace

uint8_t const * BaseTexture::Load(std::string && fileName)
{
  // Workaround for some CMake generated projects.
  if (!Utils::IsPathExisted(fileName))
  {
    fileName = "../" + fileName;
    if (!Utils::IsPathExisted(fileName))
    {
      Logger::ToLogWithFormat(Logger::Error, "File '%s' is not found.", fileName.c_str());
      return nullptr;
    }
  }

  stbi_set_flip_vertically_on_load(true);

  int w, h, components;
  if (!stbi_info(fileName.c_str(), &w, &h, &components))
  {
    Logger::ToLogWithFormat(Logger::Error, "Could not get info from the file '%s'.",
                            fileName.c_str());
    return nullptr;
  }

  m_format = FindFormat(components);
  if (m_format == TextureFormat::Unspecified)
  {
    Logger::ToLogWithFormat(Logger::Error, "Format of file '%s' is unknown.",
                            fileName.c_str());
    return nullptr;
  }

  m_type = TextureType::Texture2D;
  m_width = static_cast<uint32_t>(w);
  m_height = static_cast<uint32_t>(h);

  return stbi_load(fileName.c_str(), &h, &h, &components, components);
}

std::vector<uint8_t const *> BaseTexture::LoadCubemap(std::string && rightFileName,
                                                      std::string && leftFileName,
                                                      std::string && topFileName,
                                                      std::string && bottomFileName,
                                                      std::string && frontFileName,
                                                      std::string && backFileName)
{
  std::vector<std::string> filenames = {std::move(rightFileName), std::move(leftFileName),
                                        std::move(topFileName), std::move(bottomFileName),
                                        std::move(frontFileName), std::move(backFileName)};
  auto result = LoadArray(std::move(filenames));
  m_type = TextureType::Cubemap;
  return result;
}

std::vector<uint8_t const *> BaseTexture::LoadArray(std::vector<std::string> && filenames)
{
  if (filenames.empty())
    return {};

  std::vector<uint8_t const *> result(filenames.size());
  std::optional<uint32_t> w;
  std::optional<uint32_t> h;
  std::optional<TextureFormat> format;
  bool failed = false;
  for (size_t i = 0; i < filenames.size(); ++i)
  {
    result[i] = Load(std::move(filenames[i]));
    if (result[i] == nullptr)
    {
      failed = true;
      break;
    }

    if ((w && w.value() != m_width) || (h && h.value() != m_height) ||
        (format && format.value() != m_format))
    {
      Logger::ToLogWithFormat(Logger::Error,
        "Could not create a texture from multiple files.",
        "Files have different properties (width, height, format).");
      failed = true;
      break;
    }

    w = m_width;
    h = m_height;
    format = m_format;
  }

  if (failed)
  {
    for (auto & r : result)
    {
      if (r != nullptr)
        FreeLoadedData(r);
    }
    return {};
  }

  m_type = TextureType::Array2D;
  m_arraySize = result.size();
  return result;
}

std::vector<uint8_t> BaseTexture::LoadHeightmap(std::string && fileName)
{
  auto imageData = Load(std::move(fileName));
  if (imageData == nullptr)
    return {};

  auto const channelsCount = GetChannelsCount(m_format);
  std::vector<uint8_t> buffer(static_cast<size_t>(m_width) * m_height);
  for (size_t i = 0; i < buffer.size(); ++i)
    buffer[i] = imageData[i * channelsCount];

  FreeLoadedData(imageData);
  m_type = TextureType::Heightmap;
  return buffer;
}

void BaseTexture::FreeLoadedData(uint8_t const * imageData)
{
  CHECK(imageData != nullptr, "");
  stbi_image_free(const_cast<uint8_t *>(imageData));
}

int BaseTexture::CalculateMipLevelsCount() const
{
  auto const sz = std::min(m_width, m_height);
  return static_cast<int>(log2(static_cast<float>(sz) + 1));
}

// static
void BaseTexture::SaveToPng(std::string && filename, uint32_t width, uint32_t height,
                            TextureFormat format, uint8_t const * data)
{
  auto const components = GetChannelsCount(format);
  stbi_write_png(filename.c_str(), static_cast<int>(width), static_cast<int>(height),
                 components, data, 0);
}
}  // namespace rf
