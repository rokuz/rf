#pragma once
#define API_OPENGL

#include "common.hpp"

namespace rf::gl
{
class Texture
{
public:
  Texture() = default;
  ~Texture();

  bool Initialize(std::string && fileName);
  bool InitializeWithData(GLint format, unsigned char const * buffer, size_t width, size_t height,
                          bool mipmaps = false, int pixelFormat = -1);
  bool InitializeAsCubemap(std::string && frontFileName, std::string && backFileName,
                           std::string && leftFileName, std::string && rightFileName,
                           std::string && topFileName, std::string && bottomFileName,
                           bool mipmaps = false);
  bool InitializeAsArray(std::vector<std::string> && filenames, bool mipmaps = true);

  bool IsLoaded() const { return m_isLoaded; }

  void Bind();

  size_t GetWidth() const { return m_width; }
  size_t GetHeight() const { return m_height; }
  int GetFormat() const { return m_format; }
  int GetPixelFormat() const { return m_pixelFormat; }

private:
  GLuint m_texture = 0;
  GLenum m_target = 0;
  size_t m_width = 0;
  size_t m_height = 0;
  int m_format = 0;
  int m_pixelFormat = 0;
  bool m_isLoaded = false;
  size_t m_arraySize = 0;

  void SetSampling();
  void GenerateMipmaps();

  void Destroy();

  friend void SaveTextureToPng(std::string const &, Texture const &);
};

extern std::vector<uint8_t> LoadHeightmapData(std::string const & fileName,
                                              uint32_t & width, uint32_t & height);
extern void SaveTextureToPng(std::string const & filename, Texture const & texture);
}  // namespace rf::gl
