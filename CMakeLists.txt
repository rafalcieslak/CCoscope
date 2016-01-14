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
  )

find_package(FLEX REQUIRED)

add_subdirectory(src)