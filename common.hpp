#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <initializer_list>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
#define WINDOWS_PLATFORM
#undef min
#undef max
#pragma warning(disable : 4005)
#pragma warning(disable : 4267)
#pragma warning(disable : 4996)
#endif

#ifdef API_OPENGL
#ifdef WINDOWS_PLATFORM
#include "gl3w.h"
#else
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "glm-aabb/AABB.hpp"

#ifndef WINDOWS_PLATFORM
#define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>

#include "logger.hpp"

float constexpr kPi = static_cast<float>(3.14159265358979323846);
float constexpr kEps = 1e-5f;

constexpr float RadToDeg(float r) { return r * 180.0f / kPi; }
constexpr float DegToRad(float d) { return d * kPi / 180.0f; }

using ByteArray = std::vector<uint8_t>;

#define CHECK(condition, s) \
{ \
  if (!(condition)) \
  { \
    rf::Logger::ToLog(rf::Logger::Error, "Assertion failed! [", #condition, "]", s); \
    std::abort(); \
  } \
}

#ifdef DEBUG
#define ASSERT() {}
#else
#define ASSERT(condition, s) CHECK(condition, s)
#endif
