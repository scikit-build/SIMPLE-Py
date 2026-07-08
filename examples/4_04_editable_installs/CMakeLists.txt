cmake_minimum_required(VERSION 3.15...4.3)
project(${SKBUILD_PROJECT_NAME} LANGUAGES NONE)

add_custom_target(copy_file ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/source.txt ${CMAKE_CURRENT_BINARY_DIR}/file.txt
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/file.txt DESTINATION ${SKBUILD_PROJECT_NAME})
