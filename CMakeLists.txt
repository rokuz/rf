cmake_minimum_required(VERSION 3.13)

project(rf)

OPTION(RF_BUILD_TESTS "If the test suite is built in addition to the framework." ON)

if (APPLE)
  add_definitions(-Wno-deprecated)
endif()

set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_CXX_STANDARD 17)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
add_subdirectory(3party/assimp)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
if (WIN32)
  set(GLFW_INSTALL ON CACHE BOOL "")
else()
  set(GLFW_INSTALL OFF CACHE BOOL "")
endif()
add_subdirectory(3party/glfw)

set(GLM_STATIC_LIBRARY_ENABLE OFF CACHE BOOL "")
if (WIN32)
  set(GLM_INSTALL_ENABLE ON CACHE BOOL "")
else()
  set(GLM_INSTALL_ENABLE OFF CACHE BOOL "")
endif()
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
include_directories(3party/delaunator-cpp)

if (WIN32)
  include_directories(3party/gl3w)
endif()

if (APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

set(SRC_LIST
  base_mesh.cpp
  base_mesh.hpp
  base_texture.cpp
  base_texture.hpp
  camera.cpp
  camera.hpp
  common.hpp
  free_camera.cpp
  free_camera.hpp
  gl/gpu_program.cpp
  gl/gpu_program.hpp
  gl/mesh.cpp
  gl/mesh.hpp
  gl/texture.cpp
  gl/texture.hpp
  logger.cpp
  logger.hpp
  mesh_generator.cpp
  mesh_generator.hpp
  rf.cpp
  rf.hpp
  window.cpp
  window.hpp)

add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

if (RF_BUILD_TESTS)
  add_subdirectory(3party/googletest)
  add_subdirectory(tests)
  add_subdirectory(gl_tests)
endif()
