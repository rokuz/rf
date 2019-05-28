#pragma once
#define API_OPENGL

#include "common.hpp"

namespace rf::gl
{
enum class ShaderType : uint8_t
{
  Vertex = 0,
  Geometry,
  Fragment,

  Count
};

//class RenderTarget;
class Texture;
//class UniformBuffer;

class GpuProgram
{
public:
  GpuProgram();
  ~GpuProgram();

  bool Initialize(std::initializer_list<std::string> && sources, bool areFiles = true);
  void Destroy();
  bool IsValid() const;
	bool Use() const;

  std::optional<GLint> GetUniformLocation(std::string const & uniformName) const;

  void SetFloat(std::string const & uniform, float v);
  void SetUint(std::string const & uniform, unsigned int v);
  void SetInt(std::string const & uniform, int v);
  void SetIntArray(std::string const & uniform, int * v, int count);

  void SetVector(std::string const & uniform, glm::vec2 const & vec);
  void SetVector(std::string const & uniform, glm::vec3 const & vec);
  void SetVector(std::string const & uniform, glm::vec4 const & vec);
  void SetVector(std::string const & uniform, glm::quat const & quat);

  void SetMatrix(std::string const & uniform, glm::mat4x4 const & mat);
  void SetMatrixArray(std::string const & uniform, glm::mat4x4 * mat, int count);

  void SetTexture(std::string const & uniform, Texture * texture, int slot);

  bool ValidateProgram() const;

//	void SetUniformBuffer(std::string const & uniform, UniformBuffer * buffer,
//												int index);
//
//  void SetTexture(std::string const & uniform, RenderTarget * renderTarget,
//                  int index, int slot);
//  void SetDepth(std::string const & uniform, RenderTarget * renderTarget,
//                int slot);
//  void SetImage(std::string const & uniform, RenderTarget * renderTarget, int index,
//								int slot, bool readFlag = true, bool writeFlag = true);

private:
  GLuint m_program = 0;
  std::unordered_map<std::string, GLint> m_uniforms;
  std::vector<std::string> m_shaders;

	bool SetShaderFromFile(std::string && fileName);
  bool CompileShader(ShaderType type, GLuint * shader);
  bool LinkProgram(GLuint prog);

  GLint GetUniformLocationInternal(std::string const & uniformName);

  bool BindUniform(std::string const & uniform);

//  void SetTextureInternal(int uniformIndex, RenderTarget * renderTarget, int index,
//                          int slot);
//  void SetDepthInternal(int uniformIndex, RenderTarget * renderTarget, int slot);
//  void SetImageInternal(int uniformIndex, RenderTarget * renderTarget,
//                        int index, int slot, bool readFlag, bool writeFlag);
};
}  // namespace rf::gl
