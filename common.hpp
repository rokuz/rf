#pragma once

#include <algorithm>
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
#include "gl3w.h"
#undef min
#undef max
#else
#include <OpenGL/gl3.h>
#include <OpenGl/gl3ext.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>

#include "glm-aabb/AABB.hpp"

#ifndef WINDOWS_PLATFORM
#define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>

float constexpr kPi = static_cast<float>(3.14159265358979323846);
float constexpr kEps = 1e-5f;

using ByteArray = std::vector<uint8_t>;

#define ENABLE_SHADERS_VALIDATION 1

#ifdef DEBUG
#define ASSERT()
#else
#define ASSERT(condition, s) assert((condition) && s)
#endif
#define CHECK(condition, s) if ((condition) && s) std::abort()
