#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
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

#include <glm/glm.hpp>

#include "glm-aabb/AABB.hpp"

#ifndef WINDOWS_PLATFORM
#define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>

#define ENABLE_SHADERS_VALIDATION 1