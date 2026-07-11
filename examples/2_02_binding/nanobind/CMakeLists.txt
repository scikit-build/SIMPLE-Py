cmake_minimum_required(VERSION 3.26...4.4)
project(pyminuit2 LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  Minuit2
  GIT_REPOSITORY https://github.com/GooFit/Minuit2.git
  GIT_TAG        v6-40-02
  GIT_SHALLOW    TRUE
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Minuit2)

find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(nanobind CONFIG REQUIRED)

file(GLOB OUTPUT pyminuit2/*.cpp)
nanobind_add_module(minuit2 ${OUTPUT})
target_link_libraries(minuit2 PUBLIC Minuit2::Minuit2)
install(TARGETS minuit2 DESTINATION .)
