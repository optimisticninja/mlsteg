cmake_minimum_required(VERSION 3.1)
project(mlsteg VERSION 1.0.0 LANGUAGES C CXX)

add_subdirectory(src)
#add_subdirectory(tests)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
