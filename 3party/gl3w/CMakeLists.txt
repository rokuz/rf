cmake_minimum_required(VERSION 3.12)
project(gl3w)

set(SOURCE_LIB
  gl3w.cpp
  gl3w.h 
  glcorearb.h 
  glext.h 
  glxext.h 
  wglext.h		
)

add_library(gl3w STATIC ${SOURCE_LIB})
