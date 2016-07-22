cmake_minimum_required(VERSION 3.0)

project(
  CCoscope
  VERSION 1.0
  LANGUAGES CXX
  )

# Global C++ flags
add_definitions(
  -Wall
  -Wextra
  -Werror
  -Wno-unused-function # Stupid flex.
  -Wno-unused-parameter # Maphoon's fault
  -Wno-sign-compare     # Maphoon's fault
  -g
  -rdynamic
  )

find_package(FLEX REQUIRED)
find_package(LLVM 3.8 REQUIRED)

add_subdirectory(src)
add_subdirectory(libcco)
