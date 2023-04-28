cmake_minimum_required(VERSION 3.16)

##########################

option(BUILD_GLFW_FROM_SOURCE "Use GLFW library from sources or installed library." ON)
option(BUILD_EDITOR "Builds the game editor." ON)

##########################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project("strategy2d")

set(LIB_NAME "engine")
set(TARGET_NAME "game")
set(EDITOR_NAME "editor")

##########################

add_subdirectory(vendor/nativefiledialog)

add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(editor)

if(EXISTS "${CMAKE_SOURCE_DIR}/sandbox")
    message(STATUS "==== building sandbox ====")
    add_subdirectory(sandbox)
endif()
