cmake_minimum_required(VERSION 3.16)

add_executable(${EDITOR_NAME} "src/main.cpp")

target_include_directories(${EDITOR_NAME} PRIVATE include)

#==== link the engine ====
target_link_libraries(${EDITOR_NAME} ${LIB_NAME})
