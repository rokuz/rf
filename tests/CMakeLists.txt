cmake_minimum_required(VERSION 3.13)

project(rf_tests)

set(CMAKE_MACOSX_RPATH OFF)

enable_testing()

set(CMAKE_CXX_STANDARD 17)

include_directories(..)
include_directories(../3party/googletest/googletest/include)
include_directories(../3party/assimp/include)
include_directories(../3party/glm)
include_directories(../3party/glm-aabb)
include_directories(../3party/stb)

find_package(OpenGL REQUIRED)

if (WIN32)
  include_directories(../3party/gl3w)
endif()

if (UNIX)
  if (NOT APPLE)
    set(PTHREADS "-pthread")
  endif()
endif()

set(SOURCE_ROOT .)
aux_source_directory(${SOURCE_ROOT} TESTS_SRC_LIST)

add_executable(${PROJECT_NAME} ${TESTS_SRC_LIST})
target_link_libraries(${PROJECT_NAME} rf ${OPENGL_LIBRARY} assimp gtest ${PTHREADS})
if (WIN32)
  target_link_libraries(${PROJECT_NAME} gl3w)
endif()

add_test(unit_tests ${PROJECT_NAME})
