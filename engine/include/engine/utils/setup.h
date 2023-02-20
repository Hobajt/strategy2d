#pragma once

//==== Preparations & setup in general, general-purpose macros and stuff, logging & assert macros ====

#include "engine/utils/engine_config.h"

#include <stdio.h>
#include <exception>

#define GLSL_VERSION_STRING "#version 450 core"

//======= Various utility macros =======
#define BIT(n) (1<<(n))
#define GET_BIT(var, n) ((var) & (1<<(n)))
#define HAS_BIT(var, n) (((var) & (1<<(n))) != 0)
#define GET_FLAG(var, nvar) ((var) & (nvar))
#define HAS_FLAG(var, nvar) (((var) & (nvar)) != 0)

//======= debug setup =======
#ifndef ENGINE_DEBUG
    #define DBGBREAK()
    #define DBGONLY(...)
#else
    //debug break macro
    #ifdef _WIN32
    //#if defined(WIN32) || defined(_WIN32) || defined (__WIN32__) || defined(__NT__)
        #define DBGBREAK() __debugbreak()
    #elif defined(__linux__)
        #include <csignal>
        #define DBGBREAK() std::raise(SIGTRAP)
    #endif

    //debug-only code macro
    #define DBGONLY(...) __VA_ARGS__

    //force-enable assertions & graphics debugging in debug mode
    #define ENGINE_ENABLE_ASSERTS
    #define ENGINE_GL_DEBUG
#endif

#include "utils/log.h"

//===== assertion macros =====
#ifdef ENGINE_ENABLE_ASSERTS
    #define ASSERT(x) if(!(x)) { LOG_ERROR("Assertion '{0}' failed; file {1} (line {2})", #x, __FILE__, __LINE__); DBGBREAK(); }
    #define ASSERT_MSG(x, ...) if(!(x)) { LOG_ERROR("Assertion '{0}' failed; file {1} (line {2})", #x, __FILE__, __LINE__); LOG_ERROR(__VA_ARGS__); DBGBREAK(); }
#else
    #define ASSERT(x)
    #define ASSERT_MSG(x, ...)
#endif
