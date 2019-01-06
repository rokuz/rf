#include "gpu_program.hpp"

#include "rf.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace rf
{
namespace
{
ShaderType GetTypeByExt(std::vector<std::string> const & exts)
{
#define FIND_EXT(str) std::find(exts.cbegin(), exts.cend(), str) != exts.cend()
  if (FIND_EXT("vsh") || FIND_EXT("vs"))
    return ShaderType::Vertex;
  if (FIND_EXT("gsh") || FIND_EXT("gs"))
    return ShaderType::Geometry;
  if (FIND_EXT("fsh") || FIND_EXT("fs"))
    return ShaderType::Fragment;
#undef FIND_EXT
  return ShaderType::Count;
}

GLenum GetOGLShaderType(ShaderType type)
{
  switch (type)
  {
    case ShaderType::Vertex:
      return GL_VERTEX_SHADER;

    case ShaderType::Geometry:
      return GL_GEOMETRY_SHADER;

    case ShaderType::Fragment:
      return GL_FRAGMENT_SHADER;

    case ShaderType::Count:
      assert(false && "Invalid shader type.");
  }
  return -1;
}
}  // namespace

GpuProgram::GpuProgram()
{
  m_shaders.resize(static_cast<size_t>(ShaderType::Count));
}

GpuProgram::~GpuProgram()
{
  Destroy();
}

bool GpuProgram::SetShaderFromFile(std::string const & fileName)
{
  auto const shaderType = GetTypeByExt(Utils::GetExtensions(fileName));
  if (shaderType == ShaderType::Count)
  {
    Logger::ToLogWithFormat(
        "Error: Could not add shader '%s'. Shader type is undefined.\n",
        fileName.c_str());
    return false;
  }

  std::string sourceStr;
  Utils::ReadFileToString(fileName, sourceStr);
  if (sourceStr.empty())
  {
    Logger::ToLogWithFormat("Error: Failed to load shader '%s'.\n", fileName.c_str());
    return false;
  }

  m_shaders[static_cast<size_t>(shaderType)] = sourceStr;
  return true;
}

bool GpuProgram::Initialize(std::initializer_list<std::string> shaders, bool areFiles)
{
  if (!areFiles && shaders.size() != static_cast<size_t>(ShaderType::Count))
  {
    Logger::ToLog("Error: Initialization as sources requires passing sources for all shader types.\n");
    return false;
  }

  Destroy();
  m_program = glCreateProgram();

  std::vector<GLuint> compiledShaders;
  compiledShaders.reserve(shaders.size());
  size_t shaderIndex = 0;
  for (auto const & s : shaders)
  {
    if (s.empty())
    {
      shaderIndex++;
      continue;
    }

    if (areFiles)
    {
      if (!SetShaderFromFile(s))
      {
        Destroy();
        return false;
      }
    }
    else
    {
      m_shaders[static_cast<size_t>(shaderIndex)] = s;
    }

    GLuint shader;
    if (!CompileShader(static_cast<ShaderType>(shaderIndex), &shader))
    {
      Destroy();
      Logger::ToLogWithFormat("Error: Failed to compile shader '%s'.\n",
                              m_shaders[shaderIndex].c_str());
      return false;
    }

    glAttachShader(m_program, shader);
    compiledShaders.push_back(shader);
    shaderIndex++;
  }

  if (compiledShaders.empty())
  {
    Logger::ToLog("Error: No valid shaders are found.\n");
    Destroy();
    return false;
  }

  if (!LinkProgram(m_program))
  {
    Logger::ToLog("Error: Failed to link program.\n");

    for (auto const s : compiledShaders)
      glDeleteShader(s);

    Destroy();
    return false;
  }

#if (ENABLE_SHADERS_VALIDATION)
  ValidateProgram(m_program);
#endif

  for (auto const s : compiledShaders)
  {
    glDetachShader(m_program, s);
    glDeleteShader(s);
  }

  return true;
}

bool GpuProgram::IsValid() const
{
  return m_program != 0;
}

void GpuProgram::Destroy()
{
  if (m_program != 0)
  {
    glDeleteProgram(m_program);
    m_program = 0;
  }
}

bool GpuProgram::CompileShader(ShaderType type, GLuint * shader)
{
  GLint status;
  GLchar const * source = m_shaders[static_cast<size_t>(type)].c_str();

  *shader = glCreateShader(GetOGLShaderType(type));
  glShaderSource(*shader, 1, &source, nullptr);
  glCompileShader(*shader);

#if (ENABLE_SHADERS_VALIDATION)
  GLint logLength;
  glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength > 0)
  {
    std::vector<GLchar> log(logLength);
    glGetShaderInfoLog(*shader, logLength, &logLength, log.data());
    Logger::ToLogWithFormat("Shader compilation log:\n%s", log.data());
  }
#endif

  glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
  if (status == 0)
  {
    glDeleteShader(*shader);
    return false;
  }

  return true;
}

bool GpuProgram::LinkProgram(GLuint prog)
{
  GLint status = 0;
  glLinkProgram(prog);

#if (ENABLE_SHADERS_VALIDATION)
  GLint logLength;
  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength > 0)
  {
    std::vector<GLchar> log(logLength);
    glGetProgramInfoLog(prog, logLength, &logLength, log.data());
    Logger::ToLogWithFormat("Gpu program linkage log:\n%s", log.data());
  }
#endif

  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  return status != 0;
}

void GpuProgram::ValidateProgram(GLuint prog)
{
  GLint logLength;

  glValidateProgram(prog);
  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength > 0)
  {
    std::vector<GLchar> log(logLength);
    glGetProgramInfoLog(prog, logLength, &logLength, log.data());
    if (strcmp(log.data(), "Validation successful.\n") != 0)
      Logger::ToLogWithFormat("Gpu program validation log:\n%s", log.data());
  }
}

bool GpuProgram::Use()
{
  if (!IsValid())
    return false;

  glUseProgram(m_program);
  return true;
}

bool GpuProgram::BindUniform(std::string const & uniform)
{
  if (!IsValid())
    return false;

  m_uniforms[uniform] = glGetUniformLocation(m_program, uniform.c_str());
  if (m_uniforms[uniform] < 0)
  {
    m_uniforms[uniform] = glGetUniformBlockIndex(m_program, uniform.c_str());
    if (m_uniforms[uniform] < 0)
    {
      Logger::ToLogWithFormat("Error: Uniform '%s' has not been found to bind.\n",
                              uniform.c_str());
      return false;
    }
    return true;
  }
  return true;
}

std::optional<GLint> GpuProgram::GetUniformLocation(std::string const & uniform)
{
  auto const it = m_uniforms.find(uniform);
  if (it == m_uniforms.end())
    return {};
  return it->second;
}

GLint GpuProgram::GetUniformLocationInternal(std::string const & uniformName)
{
  auto const loc = GetUniformLocation(uniformName);
  if (!loc)
  {
    if (!BindUniform(uniformName))
      return -1;

    return GetUniformLocation(uniformName).value();
  }
  return loc.value();
}

void GpuProgram::SetFloat(std::string const & uniform, float v)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform1fv(uf, 1, &v);
}

void GpuProgram::SetUint(std::string const & uniform, unsigned int v)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform1uiv(uf, 1, &v);
}

void GpuProgram::SetInt(std::string const & uniform, int v)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform1iv(uf, 1, &v);
}

void GpuProgram::SetIntArray(std::string const & uniform, int * v, int count)
{
  if (v == nullptr)
    return;

  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform1iv(uf, count, v);
}

void GpuProgram::SetVector(std::string const & uniform, glm::vec2 const & vec)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform2fv(uf, 1, glm::value_ptr(vec));
}

void GpuProgram::SetVector(std::string const & uniform, glm::vec3 const & vec)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform3fv(uf, 1, glm::value_ptr(vec));
}

void GpuProgram::SetVector(std::string const & uniform, glm::vec4 const & vec)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform4fv(uf, 1, glm::value_ptr(vec));
}

void GpuProgram::SetVector(std::string const & uniform, glm::quat const & quat)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniform4fv(uf, 1, glm::value_ptr(quat));
}

void GpuProgram::SetMatrix(std::string const & uniform, glm::mat4x4 const & mat)
{
  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniformMatrix4fv(uf, 1, static_cast<GLboolean>(false), glm::value_ptr(mat));
}

void GpuProgram::SetMatrixArray(std::string const & uniform, glm::mat4x4 * mat, int count)
{
  if (mat == nullptr)
    return;

  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glUniformMatrix4fv(uf, count, static_cast<GLboolean>(false), glm::value_ptr(mat[0]));
}

//void GpuProgram::SetUniformBuffer(std::string const & uniform,
//                                  UniformBuffer * buffer, int index)
//{
//  if (buffer == nullptr)
//    return;
//
//  auto const uf = GetUniformLocationInternal(uniform);
//  if (uf < 0)
//    return;
//
//  buffer->Bind(index);
//  glUniformBlockBinding(m_program, uf, index);
//}
//
//void GpuProgram::SetTexture(std::string const & uniform, Texture * texture, int slot)
//{
//  if (texture == nullptr)
//    return;
//
//  auto const uf = GetUniformLocationInternal(uniform);
//  if (uf < 0)
//    return;
//
//  SetTextureInternal(uf, texture, slot);
//}
//
//void GpuProgram::SetTexture(std::string const & uniform, RenderTarget * renderTarget,
//                            int index, int slot)
//{
//  if (renderTarget == nullptr)
//    return;
//
//  auto const uf = GetUniformLocationInternal(uniform);
//  if (uf < 0)
//    return;
//
//  SetTextureInternal(uf, renderTarget, index, slot);
//}
//
//void GpuProgram::SetDepth(std::string const & uniform, RenderTarget * renderTarget,
//                          int slot)
//{
//  if (renderTarget == nullptr)
//    return;
//
//  auto const uf = GetUniformLocationInternal(uniform);
//  if (uf < 0)
//    return;
//
//  SetDepthInternal(uf, renderTarget, slot);
//}
//
//void GpuProgram::SetImage(std::string const & uniform, RenderTarget * renderTarget,
//                          int index, bool readFlag, bool writeFlag, int slot)
//{
//  if (renderTarget == nullptr)
//    return;
//
//  auto const uf = GetUniformLocationInternal(uniform);
//  if (uf < 0)
//    return;
//
//  SetImageInternal(uf, renderTarget, index, readFlag, writeFlag, slot);
//}
//
//void GpuProgram::SetTextureInternal(int uniformIndex, Texture * texture, int slot)
//{
//  glActiveTexture(GL_TEXTURE0 + slot);
//  texture->Bind();
//  glUniform1i(uniformIndex, slot);
//}
//
//void GpuProgram::SetTextureInternal(int uniformIndex, RenderTarget * renderTarget,
//                                    int index, int slot)
//{
//  glActiveTexture(GL_TEXTURE0 + slot);
//  renderTarget->Bind(index);
//  glUniform1i(uniformIndex, slot);
//}
//
//void GpuProgram::SetDepthInternal(int uniformIndex, RenderTarget * renderTarget,
//                                  int slot)
//{
//  glActiveTexture(GL_TEXTURE0 + slot);
//  renderTarget->BindDepth();
//  glUniform1i(uniformIndex, slot);
//}
//
//void GpuProgram::SetImageInternal(int uniformIndex, RenderTarget * renderTarget,
//                                  int index, int slot, bool readFlag, bool writeFlag)
//{
//  glActiveTexture(GL_TEXTURE0 + slot);
//  renderTarget->BindAsImage(index, slot, readFlag, writeFlag);
//  glUniform1i(uniformIndex, slot);
//}
}  // namespace rf