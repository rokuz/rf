#include "rf.hpp"

#include <sys/stat.h>

namespace rf
{
void Utils::RandomizeSeed()
{
  srand(static_cast<unsigned int>(time(0)));
}

bool Utils::IsPathExisted(std::string const & fileName)
{
  struct stat buffer;
  return stat(fileName.c_str(), &buffer) == 0;
}

bool Utils::ReadFileToString(std::string const & fileName, std::string & out)
{
  FILE * fp = fopen(fileName.c_str(), "rb");
  if (!fp)
    return false;

  fseek(fp, 0, SEEK_END);
  auto const filesize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  out.resize(filesize + 1);
  fread(&out[0], 1, filesize, fp);
  out[filesize] = 0;

  fclose(fp);

  return true;
}

std::string Utils::GetExtension(std::string const & fileName)
{
  auto const tokens = Tokenize<std::string>(fileName, '.');
  if (tokens.empty())
    return {};
  auto it = tokens.rbegin();
  std::string ext = fileName.substr(it->first, it->second - it->first + 1);
  std::locale loc;
  for (auto i = 0; i < ext.length(); ++i)
    ext[i] = std::tolower(ext[i], loc);

  return ext;
}

std::vector<std::string> Utils::GetExtensions(std::string const & fileName)
{
  std::vector<std::string> result;
  auto const tokens = Tokenize<std::string>(fileName, '.');
  if (tokens.empty())
    return result;

  result.reserve(tokens.size());
  auto it = tokens.begin();
  it++;
  for (; it != tokens.end(); ++it)
  {
    std::string ext = fileName.substr(it->first, it->second - it->first + 1);
    std::locale loc;
    for (auto i = 0; i < ext.length(); ++i)
      ext[i] = std::tolower(ext[i], loc);

    result.push_back(std::move(ext));
  }

  return result;
}

std::string Utils::GetPath(std::string const & fileName)
{
  size_t p_slash = fileName.find_last_of('/');
  size_t p_backslash = fileName.find_last_of('\\');
  if (p_slash == std::string::npos && p_backslash == std::string::npos)
    return {};

  size_t p = 0;
  if (p_slash != std::string::npos && p_backslash != std::string::npos)
    p = std::max(p_slash, p_backslash);
  else if (p_slash != std::string::npos)
    p = p_slash;
  else
    p = p_backslash;

  if (p == 0)
    return "";

  return fileName.substr(0, p + 1);
}

std::string Utils::GetFilename(std::string const & path)
{
  std::string p = path;
  std::replace(p.begin(), p.end(), '\\', '/');
  auto const tokens = Tokenize<std::string>(p, '/');
  if (tokens.empty())
    return path;
  auto it = tokens.rbegin();
  return p.substr(it->first, it->second - it->first + 1);
}

void Utils::RemoveFile(std::string const & path)
{
  remove(path.c_str());
}

std::string Utils::TrimExtension(std::string const & fileName)
{
  size_t dot_sign = fileName.find_last_of('.');
  if (dot_sign != std::string::npos)
  {
    if (dot_sign == 0)
      return {};
    return fileName.substr(0, dot_sign);
  }
  return fileName;
}

std::string Utils::CurrentTimeDate(bool withoutSpaces)
{
  auto const tp = std::chrono::system_clock::now();
  auto const t = std::chrono::system_clock::to_time_t(tp);
  std::string str = std::ctime(&t);
  if (withoutSpaces)
  {
    std::replace(str.begin(), str.end(), ' ', '_');
    std::replace(str.begin(), str.end(), ':', '_');
  }
  str.pop_back();  // remove \n
  return str;
}

bool Utils::CheckForOpenGLError(const char * file, const char * function, int line)
{
  GLenum err(glGetError());

  bool result = false;
  while (err != GL_NO_ERROR)
  {
    result = true;
    std::string error;

    switch (err)
    {
      case GL_INVALID_OPERATION:
        error = "GL_INVALID_OPERATION";
        break;
      case GL_INVALID_ENUM:
        error = "GL_INVALID_ENUM";
        break;
      case GL_INVALID_VALUE:
        error = "GL_INVALID_VALUE";
        break;
      case GL_OUT_OF_MEMORY:
        error = "GL_OUT_OF_MEMORY";
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        error = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
      default:
        error = "Unknown";
    }

    Logger::ToLogWithFormat(Logger::Error, "OpenGL: %s (%s > %s, line: %d).",
                            error.c_str(), file, function, line);
    err = glGetError();
  }

  return result;
}
}  // namespace rf
