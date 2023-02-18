cmake_minimum_required(VERSION 3.16)

#######################

option(GAME_DEBUG_GUI "Enables debugging GUI." OFF)

#######################

add_executable(${TARGET_NAME} "src/main.cpp")

target_include_directories(${TARGET_NAME} PRIVATE include)

#==== link the engine ====
target_link_libraries(${TARGET_NAME} ${LIB_NAME})
