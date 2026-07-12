cmake_minimum_required(VERSION 4.4)
project(freecomputepi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python 3.15 REQUIRED COMPONENTS Interpreter Development.Module Development.SABIModule)

python_add_library(_core MODULE freecomputepi/_core.cpp WITH_SOABI USE_SABI 3.15)
install(TARGETS _core DESTINATION freecomputepi)
