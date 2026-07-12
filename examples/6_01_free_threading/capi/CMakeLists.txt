cmake_minimum_required(VERSION 3.26...4.4)
project(freecomputepi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python 3.14 REQUIRED COMPONENTS Interpreter Development.Module)

python_add_library(_core MODULE freecomputepi/_core.cpp WITH_SOABI)
install(TARGETS _core DESTINATION freecomputepi)
