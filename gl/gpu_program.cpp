#include "gpu_program.hpp"

#include "rf.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace rf::gl
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

GLenum GetOpenGLShaderType(ShaderType type)
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
      ASSERT(false, "Invalid shader type.");
  }
  CHECK(false, "Unknown shader type");
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

bool GpuProgram::SetShaderFromFile(std::string && fileName)
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

  auto const shaderType = GetTypeByExt(Utils::GetExtensions(fileName));
  if (shaderType == ShaderType::Count)
  {
    Logger::ToLogWithFormat(Logger::Error, "Could not add shader '%s'. Shader type is undefined.",
                            fileName.c_str());
    return false;
  }

  std::string sourceStr;
  Utils::ReadFileToString(fileName, sourceStr);

  if (sourceStr.empty())
  {
    Logger::ToLogWithFormat(Logger::Error, "Failed to load shader '%s'.", fileName.c_str());
    return false;
  }

  m_shaders[static_cast<size_t>(shaderType)] = sourceStr;
  return true;
}

bool GpuProgram::Initialize(std::initializer_list<std::string> && shaders, bool areFiles)
{
  if (!areFiles && shaders.size() != static_cast<size_t>(ShaderType::Count))
  {
    Logger::ToLog(Logger::Error,
                  "Initialization as sources requires passing sources for all shader types.");
    return false;
  }

  Destroy();
  m_program = glCreateProgram();

  std::vector<GLuint> compiledShaders;
  compiledShaders.reserve(shaders.size());
  size_t shaderIndex = 0;
  for (auto s : shaders)
  {
    if (s.empty())
    {
      shaderIndex++;
      continue;
    }

    if (areFiles)
    {
      if (!SetShaderFromFile(std::move(s)))
      {
        Destroy();
        return false;
      }
    }
    else
    {
      m_shaders[static_cast<size_t>(shaderIndex)] = std::move(s);
    }
    shaderIndex++;
  }

  shaderIndex = 0;
  for (auto const & s : m_shaders)
  {
    if (s.empty())
    {
      shaderIndex++;
      continue;
    }

    GLuint shader;
    if (!CompileShader(static_cast<ShaderType>(shaderIndex), &shader))
    {
      Destroy();
      Logger::ToLogWithFormat(Logger::Error, "Failed to compile shader '%s'.",
                              m_shaders[shaderIndex].c_str());
      return false;
    }

    glAttachShader(m_program, shader);
    compiledShaders.push_back(shader);
    shaderIndex++;
  }

  if (compiledShaders.empty())
  {
    Logger::ToLog(Logger::Error, "No valid shaders are found.");
    Destroy();
    return false;
  }

  if (!LinkProgram(m_program))
  {
    Logger::ToLog(Logger::Error, "Failed to link program.");

    for (auto const s : compiledShaders)
      glDeleteShader(s);

    Destroy();
    return false;
  }

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

  *shader = glCreateShader(GetOpenGLShaderType(type));
  glShaderSource(*shader, 1, &source, nullptr);
  glCompileShader(*shader);

  GLint logLength;
  glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength > 0)
  {
    std::vector<GLchar> log(logLength);
    glGetShaderInfoLog(*shader, logLength, &logLength, log.data());
    Logger::ToLogWithFormat(Logger::Info, "Shader compilation log:\n  %s", log.data());
  }

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

  GLint logLength;
  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength > 0)
  {
    std::vector<GLchar> log(logLength);
    glGetProgramInfoLog(prog, logLength, &logLength, log.data());
    Logger::ToLogWithFormat(Logger::Info, "Gpu program linkage log:\n  %s", log.data());
  }

  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  return status != 0;
}

bool GpuProgram::ValidateProgram() const
{
  GLint logLength;
  glValidateProgram(m_program);
  glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength > 0)
  {
    std::vector<GLchar> log(logLength);
    glGetProgramInfoLog(m_program, logLength, &logLength, log.data());
    if (strcmp(log.data(), "Validation successful.\n") != 0)
    {
      Logger::ToLogWithFormat(Logger::Warning, "Gpu program validation log:\n  %s", log.data());
      return false;
    }
  }
  return true;
}

bool GpuProgram::Use() const
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
      Logger::ToLogWithFormat(Logger::Error, "Uniform '%s' has not been found to bind.",
                              uniform.c_str());
      return false;
    }
    return true;
  }
  return true;
}

std::optional<GLint> GpuProgram::GetUniformLocation(std::string const & uniform) const
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

void GpuProgram::SetTexture(std::string const & uniform, Texture * texture, int slot)
{
  CHECK(texture != nullptr, "Texture must exist");

  auto const uf = GetUniformLocationInternal(uniform);
  if (uf < 0)
    return;

  glActiveTexture(GL_TEXTURE0 + slot);
  texture->Bind();
  glUniform1i(uf, slot);
}
}  // namespace rf::gl
