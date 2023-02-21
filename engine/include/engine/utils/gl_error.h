#pragma once

//==== Contains set of functions for OpenGL error reports and handling ====

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace eng {

    //Function for manual error checking (query for errors) - use the macro instead.
    void glErrorCheck_(const char* file, int line);
    #define glErrorCheck() glErrorCheck_(__FILE__, __LINE__)

    //Debug callback for OpenGL error handling.
    void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);

    ///Debug callback for OpenGL error handling.
    //Only logs error codes (no string conversion). Codes from here: https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
    void GLAPIENTRY glDebugCallback_simple(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);

}//namespace eng
