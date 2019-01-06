#pragma once

#include "common.hpp"

namespace rf
{
class Texture
{
public:
  Texture() = default;
  ~Texture();

  bool Initialize(std::string const & fileName);
  bool InitializeWithData(GLint format, unsigned char const * buffer, size_t width, size_t height,
                          bool mipmaps = false, int pixelFormat = -1);
  bool InitializeAsCubemap(std::string const & frontFilename, std::string const & backFilename,
                           std::string const & leftFilename, std::string const & rightFilename,
                           std::string const & topFilename, std::string const & bottomFilename,
                           bool mipmaps = false);
  bool InitializeAsArray(std::vector<std::string> const & filenames, bool mipmaps = true);

  bool IsLoaded() const { return m_isLoaded; }

  void Bind();

  size_t GetWidth() const
  {
    return m_width;
  }
  size_t GetHeight() const
  {
    return m_height;
  }
  int GetFormat() const
  {
    return m_format;
  }
  int GetPixelFormat() const
  {
    return m_pixelFormat;
  }

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

std::vector<unsigned char> LoadHeightmapData(std::string const & fileName,
		                                         unsigned int & width,
                                             unsigned int & height);
void SaveTextureToPng(std::string const & filename, Texture const & texture);
}  // namespace rf
