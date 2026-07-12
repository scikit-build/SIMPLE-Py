cmake_minimum_required(VERSION 3.26...4.4)
project(setuptools_example LANGUAGES C)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

python_add_library(_core MODULE src/setuptools_example/_core.c WITH_SOABI)
install(TARGETS _core DESTINATION setuptools_example)
