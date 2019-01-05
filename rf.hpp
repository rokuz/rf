#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
#include "gl3w.hpp"
#undef min
#undef max
#else
#include <OpenGL/gl3.h>
#include <OpenGl/gl3ext.h>
#endif

#include <glm/glm.hpp>

#include "glm-aabb/AABB.hpp"

#include <GLFW/glfw3.h>

namespace rf
{
class Utils
{
public:
  static void RandomizeSeed();
  static bool IsPathExisted(std::string const & fileName);
  static bool ReadFileToString(std::string const & fileName, std::string & out);
  static std::string GetExtension(std::string const & fileName);
  static std::list<std::string> GetExtensions(std::string const & fileName);
  static std::string TrimExtension(std::string const & fileName);
  static std::string GetPath(std::string const & path);
  static std::string GetFilename(std::string const & path);
  static std::string CurrentTimeDate(bool withoutSpaces = false);

  template <typename StringType>
  static std::list<std::pair<size_t, size_t>> Tokenize(StringType const & str,
                                                       typename StringType::value_type delimiter)
  {
    std::list<std::pair<size_t, size_t>> result;
    if (str.empty())
      return result;

    size_t offset = 0;
    size_t pos = 0;
    while ((pos = str.find_first_of(delimiter, offset)) != StringType::npos)
    {
      if (pos != 0 && offset != pos)
        result.emplace_back(std::make_pair((size_t)offset, (size_t)pos - 1));

      offset = pos + 1;
    }

    if (offset < str.length())
      result.push_back(std::make_pair((size_t)offset, str.length() - 1));

    return result;
  }

  static bool CheckForOpenGLError(const char * file, const char * function, int line);
};
}  // namespace rf

#define glCheckError() common::Utils::CheckForOpenGLError(__FILE__, __FUNCTION__, __LINE__)
