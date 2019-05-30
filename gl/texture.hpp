#pragma once
#define API_OPENGL

#include "base_texture.hpp"

namespace rf::gl
{
class Texture : public BaseTexture
{
public:
  Texture() = default;
  explicit Texture(std::string const & id) : BaseTexture(id) {}
  ~Texture() override;

  bool Initialize(std::string && fileName);
  bool InitializeWithData(GLint format, uint8_t const * buffer,
                          uint32_t width, uint32_t height,
                          bool mipmaps = false, int pixelFormat = -1);
  bool InitializeAsCubemap(std::string && rightFileName,
                           std::string && leftFileName,
                           std::string && topFileName,
                           std::string && bottomFileName,
                           std::string && frontFileName,
                           std::string && backFileName,
                           bool mipmaps = false);
  bool InitializeAsArray(std::vector<std::string> && filenames, bool mipmaps = true);

  void Bind();

  int GetInnerFormat() const { return m_innerFormat; }
  int GetPixelFormat() const { return m_pixelFormat; }

  void Save(std::string && filename) const;

private:
  GLuint m_texture = 0;
  GLenum m_target = 0;
  int m_innerFormat = -1;
  int m_pixelFormat = -1;

  void SetSampling();
  void GenerateMipmaps();

  void Destroy();
};
}  // namespace rf::gl
