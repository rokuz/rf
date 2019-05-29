#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "rf.hpp"

namespace rf::gl
{
namespace
{
int FindPixelFormat(int textureFormat)
{
  switch (textureFormat)
  {
    case GL_R8:
      return GL_RED;
    case GL_R8_SNORM:
      return GL_RED;
    case GL_R16:
      return GL_RED;
    case GL_R16_SNORM:
      return GL_RED;
    case GL_RG8:
      return GL_RG;
    case GL_RG8_SNORM:
      return GL_RG;
    case GL_RG16:
      return GL_RG;
    case GL_RG16_SNORM:
      return GL_RG;
    case GL_R3_G3_B2:
      return GL_RGB;
    case GL_RGB4:
      return GL_RGB;
    case GL_RGB5:
      return GL_RGB;
    case GL_RGB8:
      return GL_RGB;
    case GL_RGB8_SNORM:
      return GL_RGB;
    case GL_RGB10:
      return GL_RGB;
    case GL_RGB12:
      return GL_RGB;
    case GL_RGB16_SNORM:
      return GL_RGB;
    case GL_RGBA2:
      return GL_RGB;
    case GL_RGBA4:
      return GL_RGB;
    case GL_RGB5_A1:
      return GL_RGBA;
    case GL_RGBA8:
      return GL_RGBA;
    case GL_RGBA8_SNORM:
      return GL_RGBA;
    case GL_RGB10_A2:
      return GL_RGBA;
    case GL_RGB10_A2UI:
      return GL_RGBA;
    case GL_RGBA12:
      return GL_RGBA;
    case GL_RGBA16:
      return GL_RGBA;
    case GL_SRGB8:
      return GL_RGB;
    case GL_SRGB8_ALPHA8:
      return GL_RGBA;
    case GL_R16F:
      return GL_RED;
    case GL_RG16F:
      return GL_RG;
    case GL_RGB16F:
      return GL_RGB;
    case GL_RGBA16F:
      return GL_RGBA;
    case GL_R32F:
      return GL_RED;
    case GL_RG32F:
      return GL_RG;
    case GL_RGB32F:
      return GL_RGB;
    case GL_RGBA32F:
      return GL_RGBA;
    case GL_R11F_G11F_B10F:
      return GL_RGB;
    case GL_RGB9_E5:
      return GL_RGB;
    case GL_R8I:
      return GL_RED;
    case GL_R8UI:
      return GL_RED;
    case GL_R16I:
      return GL_RED;
    case GL_R16UI:
      return GL_RED;
    case GL_R32I:
      return GL_RED;
    case GL_R32UI:
      return GL_RED;
    case GL_RG8I:
      return GL_RG;
    case GL_RG8UI:
      return GL_RG;
    case GL_RG16I:
      return GL_RG;
    case GL_RG16UI:
      return GL_RG;
    case GL_RG32I:
      return GL_RG;
    case GL_RG32UI:
      return GL_RG;
    case GL_RGB8I:
      return GL_RGB;
    case GL_RGB8UI:
      return GL_RGB;
    case GL_RGB16I:
      return GL_RGB;
    case GL_RGB16UI:
      return GL_RGB;
    case GL_RGB32I:
      return GL_RGB;
    case GL_RGB32UI:
      return GL_RGB;
    case GL_RGBA8I:
      return GL_RGBA;
    case GL_RGBA8UI:
      return GL_RGBA;
    case GL_RGBA16I:
      return GL_RGBA;
    case GL_RGBA16UI:
      return GL_RGBA;
    case GL_RGBA32I:
      return GL_RGBA;
    case GL_RGBA32UI:
      return GL_RGBA;
  }
  return -1;
}

int FindFormat(int components)
{
  int format;
  switch (components)
  {
    case 1:
      format = GL_R8;
      break;
    case 3:
      format = GL_RGB8;
      break;
    case 4:
      format = GL_RGBA8;
      break;
    default:
      format = -1;
  }
  return format;
}

int GetMipLevelsCount(size_t width, size_t height)
{
  auto const sz = std::min(width, height);
  return static_cast<int>(log2(static_cast<float>(sz) + 1));
}

struct ImageInfo
{
  int width = 0;
  int height = 0;
  int components = 0;
  uint8_t * imageData = nullptr;
};

bool AreEqual(ImageInfo const & info1, ImageInfo const & info2)
{
  return (info1.width == info2.width) && (info1.height == info2.height) &&
         (info1.components == info2.components);
}
}  // namespace

Texture::~Texture()
{
  Destroy();
}

bool Texture::Initialize(std::string && fileName)
{
  // Workaround for some CMake generated projects.
  if (!Utils::IsPathExisted(fileName))
  {
    fileName = "../" + fileName;
    if (!Utils::IsPathExisted(fileName))
    {
      Logger::ToLogWithFormat(Logger::Error, "File '%s' is not found.", fileName.c_str());
      return false;
    }
  }

  Destroy();

  stbi_set_flip_vertically_on_load(true);
  ImageInfo info;

  if (!stbi_info(fileName.c_str(), &info.width, &info.height, &info.components))
  {
    Logger::ToLogWithFormat(Logger::Error, "Could not get info from the file '%s'.", fileName.c_str());
    return false;
  }

  info.imageData =
    stbi_load(fileName.c_str(), &info.width, &info.height, &info.components, info.components);
  if (info.imageData == nullptr)
  {
    Logger::ToLogWithFormat(Logger::Error, "Could not load file '%s'.", fileName.c_str());
    return false;
  }

  m_format = FindFormat(info.components);
  if (m_format == -1)
  {
    Logger::ToLogWithFormat(Logger::Error, "Format of file '%s' is unknown.", fileName.c_str());
    stbi_image_free(info.imageData);
    return false;
  }

  bool result = InitializeWithData(m_format, info.imageData, static_cast<size_t>(info.width),
                                   static_cast<size_t>(info.height), false);
  stbi_image_free(info.imageData);

  return result;
}

bool Texture::InitializeWithData(GLint format, unsigned char const * buffer, size_t width,
                                 size_t height, bool mipmaps, int pixelFormat)
{
  Destroy();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  m_target = GL_TEXTURE_2D;
  m_format = format;
  m_pixelFormat = pixelFormat < 0 ? FindPixelFormat(format) : pixelFormat;
  m_width = width;
  m_height = height;
  glGenTextures(1, &m_texture);
  glBindTexture(m_target, m_texture);
  int mipLevels = mipmaps ? GetMipLevelsCount(m_width, m_height) : 1;
  glTexStorage2D(m_target, mipLevels, m_format, m_width, m_height);
  glTexSubImage2D(m_target, 0, 0, 0, m_width, m_height, m_pixelFormat, GL_UNSIGNED_BYTE, buffer);

  SetSampling();
  if (mipmaps)
    GenerateMipmaps();
  glBindTexture(m_target, 0);

  if (glCheckError())
  {
    Destroy();
    return false;
  }

  m_isLoaded = true;
  return m_isLoaded;
}

bool Texture::InitializeAsCubemap(std::string && frontFileName, std::string && backFileName,
                                  std::string && leftFileName, std::string && rightFileName,
                                  std::string && topFileName, std::string && bottomFileName,
                                  bool mipmaps)
{
  // Workaround for some CMake generated projects.
  if (!Utils::IsPathExisted(frontFileName))
  {
    frontFileName = "../" + frontFileName;
    if (!Utils::IsPathExisted(frontFileName))
    {
      Logger::ToLogWithFormat(Logger::Error, "File '%s' is not found.", frontFileName.c_str());
      return false;
    }
    else
    {
      backFileName = "../" + backFileName;
      leftFileName = "../" + leftFileName;
      rightFileName = "../" + rightFileName;
      topFileName = "../" + topFileName;
      bottomFileName = "../" + bottomFileName;
    }
  }

  Destroy();

  stbi_set_flip_vertically_on_load(true);

  size_t constexpr kSidesCount = 6;
  std::string filenames[kSidesCount] = {std::move(rightFileName), std::move(leftFileName),
                                        std::move(topFileName), std::move(bottomFileName),
                                        std::move(frontFileName), std::move(backFileName)};
  ImageInfo info[kSidesCount];
  auto cleanFunc = [&info, kSidesCount]() {
    for (size_t i = 0; i < kSidesCount; i++)
    {
      if (info[i].imageData != nullptr)
        stbi_image_free(info[i].imageData);
    }
  };

  for (size_t i = 0; i < kSidesCount; i++)
  {
    info[i].imageData = stbi_load(filenames[i].c_str(), &info[i].width, &info[i].height,
                                  &info[i].components, STBI_rgb_alpha);
    if (info[i].imageData == nullptr)
    {
      cleanFunc();
      return false;
    }

    if (i > 0 && !AreEqual(info[i], info[i - 1]))
    {
      Logger::ToLogWithFormat(Logger::Error,
        "Could not create a cubemap, files have different properties (width, height, components).");
      cleanFunc();
      return false;
    }
  }

  m_format = FindFormat(info[0].components);
  if (m_format < 0)
  {
    Logger::ToLogWithFormat(Logger::Error, "Cubemap format is unsupported.");
    cleanFunc();
    return false;
  }
  m_pixelFormat = FindPixelFormat(m_format);
  if (m_pixelFormat < 0)
  {
    Logger::ToLogWithFormat(Logger::Error,
      "Could not create a cubemap, pixel format is unsupported.");
    cleanFunc();
    return false;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  m_target = GL_TEXTURE_CUBE_MAP;
  m_width = static_cast<size_t>(info[0].width);
  m_height = static_cast<size_t>(info[0].height);
  glGenTextures(1, &m_texture);
  glBindTexture(m_target, m_texture);
  int mipLevels = mipmaps ? GetMipLevelsCount(m_width, m_height) : 1;
  glTexStorage2D(m_target, mipLevels, m_format, m_width, m_height);
  for (size_t i = 0; i < 6; i++)
  {
    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, m_width, m_height,
                    m_pixelFormat, GL_UNSIGNED_BYTE, info[i].imageData);
  }
  SetSampling();
  if (mipmaps)
    GenerateMipmaps();
  glBindTexture(m_target, 0);

  cleanFunc();

  if (glCheckError())
  {
    Destroy();
    return false;
  }

  m_isLoaded = true;
  return m_isLoaded;
}

bool Texture::InitializeAsArray(std::vector<std::string> && filenames, bool mipmaps)
{
  ASSERT(filenames.size() > 0, "");

  // Workaround for some CMake generated projects.
  if (!Utils::IsPathExisted(filenames.front()))
  {
    filenames.front() = "../" + filenames.front();
    if (!Utils::IsPathExisted(filenames.front()))
    {
      Logger::ToLogWithFormat(Logger::Error, "File '%s' is not found.",
                              filenames.front().c_str());
      return false;
    }
    else
    {
      for (size_t i = 1; i < filenames.size(); ++i)
        filenames[i] = "../" + filenames[i];
    }
  }

  Destroy();

  stbi_set_flip_vertically_on_load(true);

  m_arraySize = filenames.size();
  std::vector<ImageInfo> info;
  info.resize(m_arraySize);

  auto cleanFunc = [this, &info]() {
    for (size_t i = 0; i < m_arraySize; i++)
    {
      if (info[i].imageData != nullptr)
        stbi_image_free(info[i].imageData);
    }
  };

  for (size_t i = 0; i < m_arraySize; i++)
  {
    info[i].imageData = stbi_load(filenames[i].c_str(), &info[i].width, &info[i].height,
                                  &info[i].components, STBI_rgb_alpha);
    if (info[i].imageData == nullptr)
    {
      cleanFunc();
      return false;
    }

    if (i > 0 && !AreEqual(info[i], info[i - 1]))
    {
      Logger::ToLogWithFormat(Logger::Error,
        "Could not create an array, files have different properties (width, height, components).");
      cleanFunc();
      return false;
    }
  }

  m_format = FindFormat(info[0].components);
  if (m_format < 0)
  {
    Logger::ToLogWithFormat(Logger::Error, "Format of an array is unsupported.");
    cleanFunc();
    return false;
  }
  m_pixelFormat = FindPixelFormat(m_format);
  if (m_pixelFormat < 0)
  {
    Logger::ToLogWithFormat(Logger::Error,
      "Could not create an array, pixel format is unsupported.");
    cleanFunc();
    return false;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  m_target = GL_TEXTURE_2D_ARRAY;
  m_width = static_cast<size_t>(info[0].width);
  m_height = static_cast<size_t>(info[0].height);
  glGenTextures(1, &m_texture);
  glBindTexture(m_target, m_texture);
  int mipLevels = mipmaps ? GetMipLevelsCount(m_width, m_height) : 1;
  glTexStorage3D(m_target, mipLevels, m_format, m_width, m_height, m_arraySize);
  for (size_t i = 0; i < m_arraySize; i++)
  {
    glTexSubImage3D(m_target, 0, 0, 0, i, m_width, m_height, 1, m_pixelFormat,
                    GL_UNSIGNED_BYTE, info[i].imageData);
  }
  SetSampling();
  if (mipmaps)
    GenerateMipmaps();
  glBindTexture(m_target, 0);

  cleanFunc();

  if (glCheckError())
  {
    Destroy();
    return false;
  }

  m_isLoaded = true;
  return m_isLoaded;
}

void Texture::SetSampling()
{
  if (m_target != 0 && m_texture != 0)
  {
    if (m_target == GL_TEXTURE_CUBE_MAP)
    {
      glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    else if (m_target == GL_TEXTURE_2D_ARRAY)
    {
      glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }
    else
    {
      glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }
}
void Texture::GenerateMipmaps()
{
  if (m_target != 0)
  {
    glGenerateMipmap(m_target);
    glCheckError();
  }
}

void Texture::Destroy()
{
  if (m_texture != 0)
    glDeleteTextures(1, &m_texture);

  m_texture = 0;
  m_target = 0;
  m_width = 0;
  m_height = 0;
  m_format = 0;
  m_pixelFormat = 0;
  m_arraySize = 0;
  m_isLoaded = false;
}

void Texture::Bind()
{
  glBindTexture(m_target, m_texture);
}

std::vector<uint8_t> LoadHeightmapData(std::string const & fileName, uint32_t & width,
                                       uint32_t & height)
{
  stbi_set_flip_vertically_on_load(true);
  ImageInfo info;
  info.imageData = stbi_load(fileName.c_str(), &info.width, &info.height, &info.components, 0);
  if (info.imageData == nullptr)
  {
    Logger::ToLogWithFormat(Logger::Error, "Could not load file '%s'.", fileName.c_str());
    return std::vector<uint8_t>();
  }

  auto const bufSize = static_cast<size_t>(width * height);
  std::vector<uint8_t> buffer;
  buffer.resize(bufSize);

  for (int i = 0; i < bufSize; i++)
    buffer[i] = info.imageData[i * info.components];

  stbi_image_free(info.imageData);
  return buffer;
}

void SaveTextureToPng(std::string const & filename, Texture const & texture)
{
  glBindTexture(texture.m_target, texture.m_texture);

  GLint sz = 0;
  GLint componentSize = 0;
  glGetTexLevelParameteriv(texture.m_target, 0, GL_TEXTURE_RED_SIZE, &componentSize);
  sz += componentSize;
  glGetTexLevelParameteriv(texture.m_target, 0, GL_TEXTURE_GREEN_SIZE, &componentSize);
  sz += componentSize;
  glGetTexLevelParameteriv(texture.m_target, 0, GL_TEXTURE_BLUE_SIZE, &componentSize);
  sz += componentSize;
  glGetTexLevelParameteriv(texture.m_target, 0, GL_TEXTURE_ALPHA_SIZE, &componentSize);
  sz += componentSize;
  sz /= 8;

  auto const bufSize = texture.GetWidth() * texture.GetHeight() * sz;
  std::vector<unsigned char> pixels(bufSize);

  int pixelFormat = texture.GetPixelFormat();
  if (pixelFormat < 0)
  {
    Logger::ToLogWithFormat(Logger::Error,
      "Failed to save file '%s', pixel format is unsupported.", filename.c_str());
    return;
  }

  glGetTexImage(texture.m_target, 0, pixelFormat, GL_UNSIGNED_BYTE, pixels.data());
  if (glCheckError())
    return;

  stbi_write_png(filename.c_str(), texture.GetWidth(), texture.GetHeight(), sz, pixels.data(), 0);
}
}  // namespace rf::gl
