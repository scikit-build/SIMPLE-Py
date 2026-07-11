cmake_minimum_required(VERSION 3.26...4.4)
project(simpleclass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(pybind11)

pybind11_add_module(simpleclass simpleclass.cpp)
install(TARGETS simpleclass DESTINATION .)
