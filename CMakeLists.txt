cmake_minimum_required(VERSION 3.12)

project(rf)

OPTION(RF_AABB_BUILD_TESTS "If the test suite is built in addition to the framework." ON)

if (APPLE)
  add_definitions(-Wno-deprecated)
endif()

set(CMAKE_MACOSX_RPATH OFF)
set(BUILD_SHARED_LIBS OFF)
set(gtest_force_shared_crt ON)
set(CMAKE_CXX_STANDARD 17)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
add_subdirectory(3party/assimp)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
add_subdirectory(3party/glfw)

set(GLM_STATIC_LIBRARY_ENABLE OFF CACHE BOOL "")
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "")
set(GLM_TEST_ENABLE OFF CACHE BOOL "")
add_subdirectory(3party/glm)

add_subdirectory(3party/glm-aabb)

find_package(OpenGL REQUIRED)

if (WIN32)
  add_subdirectory(3party/gl3w)
endif()

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3party/assimp/include)
include_directories(3party/assimp/include)
include_directories(3party/cxxopts/include)
include_directories(3party/glfw/include)
include_directories(3party/glm)
include_directories(3party/glm-aabb)
include_directories(3party/stb)

if (WIN32)
  include_directories(3party/gl3w)
endif()

if (APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

set(SOURCE_ROOT .)
aux_source_directory(${SOURCE_ROOT} SRC_LIST)
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

if (RF_AABB_BUILD_TESTS)
  add_subdirectory(3party/googletest)
  add_subdirectory(tests)
endif()
