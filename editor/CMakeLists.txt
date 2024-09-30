cmake_minimum_required(VERSION 3.16)

if(BUILD_EDITOR)
    add_executable(${EDITOR_NAME} "src/main.cpp" "include/editor.h" "src/editor.cpp" "include/input.h" "src/input.cpp" 
    "include/menus.h" "src/menus.cpp" "include/tools.h" "src/tools.cpp" "include/context.h" "src/context.cpp")

    target_include_directories(${EDITOR_NAME} PRIVATE include)

    #==== link the engine ====
    target_link_libraries(${EDITOR_NAME} ${LIB_NAME})

    #==== link file dialog library ====
    target_include_directories(${EDITOR_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/vendor/nativefiledialog/src/include")
    target_link_libraries(${EDITOR_NAME} nfd)
else()
    message(STATUS "===Skipping Editor build===")
endif()
