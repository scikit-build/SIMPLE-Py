cmake_minimum_required(VERSION 3.26...4.4)
project(Minuit2SimpleExamle LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  Minuit2
  GIT_REPOSITORY https://github.com/GooFit/Minuit2.git
  GIT_TAG        v6-40-02
  GIT_SHALLOW    TRUE
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Minuit2)

add_executable(simpleminuit simpleminuit.cpp SimpleFCN.hpp)
target_link_libraries(simpleminuit PRIVATE Minuit2::Minuit2)
