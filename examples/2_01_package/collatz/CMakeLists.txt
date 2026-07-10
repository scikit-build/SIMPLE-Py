cmake_minimum_required(VERSION 3.15...4.3)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(collatz collatz.cpp)

install(TARGETS collatz DESTINATION .)
