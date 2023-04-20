cmake_minimum_required(VERSION 3.16)

#######################

set(APP_NAME "sandbox")

add_executable(${APP_NAME} "src/main.cpp" "include/sandbox.h" "src/sandbox.cpp")

target_include_directories(${APP_NAME} PRIVATE include)

#==== link the engine ====
target_link_libraries(${APP_NAME} ${LIB_NAME})
