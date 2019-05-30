#include "texture.hpp"

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

TextureFormat ConvertFromOpenGLFormat(int format)
{
  switch (format)
  {
  case GL_R8 : return TextureFormat::R8;
  case GL_RG8 : return TextureFormat::RG8;
  case GL_RGB8 : return TextureFormat::RGB8;
  case GL_RGBA8 : return TextureFormat::RGBA8;
  }
  return TextureFormat::Unspecified;
}

int ConvertToOpenGLFormat(TextureFormat format)
{
  switch (format)
  {
  case TextureFormat::R8 : return GL_R8;
  case TextureFormat::RG8 : return GL_RG8;
  case TextureFormat::RGB8 : return GL_RGB8;
  case TextureFormat::RGBA8 : return GL_RGBA8;
  default:
    Logger::ToLog(Logger::Warning, "Unsupported texture format conversion for OpenGL.");
  }
  return -1;
}
}  // namespace

Texture::~Texture()
{
  Destroy();
}

bool Texture::Initialize(std::string && fileName)
{
  Destroy();

  auto imageData = Load(std::move(fileName));
  if (imageData == nullptr)
    return false;

  m_innerFormat = ConvertToOpenGLFormat(m_format);
  if (m_innerFormat < 0)
    return false;

  auto const result = InitializeWithData(m_innerFormat, imageData, m_width,
                                         m_height, true /* mipmaps */);
  FreeLoadedData(imageData);
  return result;
}

bool Texture::InitializeWithData(GLint format, uint8_t const * buffer,
                                 uint32_t width, uint32_t height,
                                 bool mipmaps, int pixelFormat)
{
  Destroy();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  m_target = GL_TEXTURE_2D;
  m_width = width;
  m_height = height;
  m_format = ConvertFromOpenGLFormat(format);
  m_innerFormat = format;
  m_pixelFormat = pixelFormat < 0 ? FindPixelFormat(format) : pixelFormat;
  glGenTextures(1, &m_texture);
  glBindTexture(m_target, m_texture);
  auto const mipLevels = mipmaps ? CalculateMipLevelsCount() : 1;
  glTexStorage2D(m_target, mipLevels, m_innerFormat, m_width, m_height);
  glTexSubImage2D(m_target, 0, 0, 0, m_width, m_height, m_pixelFormat,
                  GL_UNSIGNED_BYTE, buffer);

  SetSampling();
  if (mipmaps)
    GenerateMipmaps();
  glBindTexture(m_target, 0);

  if (glCheckError())
  {
    Destroy();
    return false;
  }
  return true;
}

bool Texture::InitializeAsCubemap(std::string && rightFileName,
                                  std::string && leftFileName,
                                  std::string && topFileName,
                                  std::string && bottomFileName,
                                  std::string && frontFileName,
                                  std::string && backFileName,
                                  bool mipmaps)
{
  Destroy();

  auto imageData = LoadCubemap(std::move(rightFileName), std::move(leftFileName),
                               std::move(topFileName), std::move(bottomFileName),
                               std::move(frontFileName), std::move(backFileName));
  if (imageData.empty())
    return false;

  auto cleanFunc = [this, &imageData]()
  {
    for (auto & d : imageData)
      FreeLoadedData(d);
  };

  m_innerFormat = ConvertToOpenGLFormat(m_format);
  if (m_innerFormat < 0)
  {
    cleanFunc();
    return false;
  }

  m_pixelFormat = FindPixelFormat(m_innerFormat);
  if (m_pixelFormat < 0)
  {
    Logger::ToLogWithFormat(Logger::Error,
      "Could not create a cubemap, pixel format is unsupported.");
    cleanFunc();
    return false;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  m_target = GL_TEXTURE_CUBE_MAP;
  glGenTextures(1, &m_texture);
  glBindTexture(m_target, m_texture);
  auto const mipLevels = mipmaps ? CalculateMipLevelsCount() : 1;
  glTexStorage2D(m_target, mipLevels, m_innerFormat, m_width, m_height);
  CHECK(imageData.size() == 6, "");
  for (size_t i = 0; i < imageData.size(); ++i)
  {
    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, m_width, m_height,
                    m_pixelFormat, GL_UNSIGNED_BYTE, imageData[i]);
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

  return true;
}

bool Texture::InitializeAsArray(std::vector<std::string> && filenames, bool mipmaps)
{
  CHECK(filenames.size() > 0, "");
  Destroy();

  auto imageData = LoadArray(std::move(filenames));
  if (imageData.empty())
    return false;

  auto cleanFunc = [this, &imageData]()
  {
    for (auto & d : imageData)
      FreeLoadedData(d);
  };

  m_innerFormat = ConvertToOpenGLFormat(m_format);
  if (m_innerFormat < 0)
  {
    cleanFunc();
    return false;
  }

  m_pixelFormat = FindPixelFormat(m_innerFormat);
  if (m_pixelFormat < 0)
  {
    Logger::ToLogWithFormat(Logger::Error,
      "Could not create a textures array, the pixel format is unsupported.");
    cleanFunc();
    return false;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  m_target = GL_TEXTURE_2D_ARRAY;
  glGenTextures(1, &m_texture);
  glBindTexture(m_target, m_texture);
  auto const mipLevels = mipmaps ? CalculateMipLevelsCount() : 1;
  glTexStorage3D(m_target, mipLevels, m_innerFormat, m_width, m_height, m_arraySize);
  for (size_t i = 0; i < m_arraySize; ++i)
  {
    glTexSubImage3D(m_target, 0, 0, 0, i, m_width, m_height, 1, m_pixelFormat,
                    GL_UNSIGNED_BYTE, imageData[i]);
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

  return true;
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
  m_innerFormat = -1;
  m_pixelFormat = -1;
  m_arraySize = 0;
}

void Texture::Bind()
{
  glBindTexture(m_target, m_texture);
}

void Texture::Save(std::string && filename) const
{
  glBindTexture(m_target, m_texture);

  GLint sz = 0;
  GLint componentSize = 0;
  glGetTexLevelParameteriv(m_target, 0, GL_TEXTURE_RED_SIZE, &componentSize);
  sz += componentSize;
  glGetTexLevelParameteriv(m_target, 0, GL_TEXTURE_GREEN_SIZE, &componentSize);
  sz += componentSize;
  glGetTexLevelParameteriv(m_target, 0, GL_TEXTURE_BLUE_SIZE, &componentSize);
  sz += componentSize;
  glGetTexLevelParameteriv(m_target, 0, GL_TEXTURE_ALPHA_SIZE, &componentSize);
  sz += componentSize;
  sz /= 8;

  if (m_pixelFormat < 0)
  {
    Logger::ToLogWithFormat(Logger::Error,
      "Failed to save file '%s', the pixel format is unsupported.", filename.c_str());
    return;
  }

  std::vector<uint8_t> pixels(static_cast<size_t>(m_width) * m_height * sz);
  glGetTexImage(m_target, 0, m_pixelFormat, GL_UNSIGNED_BYTE, pixels.data());
  if (glCheckError())
    return;

  SaveToPng(std::move(filename), m_width, m_height, m_format, pixels.data());
}
}  // namespace rf::gl
