cmake_minimum_required(VERSION 3.26...4.4)
project(freecomputepi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python 3.14 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(_core FREE_THREADED freecomputepi/_core.cpp)
install(TARGETS _core DESTINATION freecomputepi)
