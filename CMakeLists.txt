cmake_minimum_required(VERSION 3.0)

project(
  CCoscope
  VERSION 1.0
  LANGUAGES C CXX
  )

# Global C++ flags
add_definitions(
  -Wall
  -Wextra
  -Werror
  -std=c++14
  -Wno-unused-function # Stupid flex.
  -Wno-unused-parameter # Maphoon's fault
  -Wno-sign-compare     # Maphoon's fault
  -g
 # -fno-rtti
  )

find_package(FLEX REQUIRED)
find_package(LLVM REQUIRED)

add_subdirectory(src)
