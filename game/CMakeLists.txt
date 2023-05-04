cmake_minimum_required(VERSION 3.16)

#######################

option(GAME_DEBUG_GUI "Enables debugging GUI." OFF)

#######################

add_executable(${TARGET_NAME} "src/main.cpp" "include/game.h" "src/game.cpp" "include/stage.h" "src/stage.cpp" "include/menu.h" "src/menu.cpp"
"include/intro.h" "src/intro.cpp" "include/ingame.h" "src/ingame.cpp" "include/recap.h" "src/recap.cpp")

target_include_directories(${TARGET_NAME} PRIVATE include)

#==== link the engine ====
target_link_libraries(${TARGET_NAME} ${LIB_NAME})
