#include "engine/utils/gl_error.h"
#include "engine/utils/setup.h"

namespace eng {

    void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
        //if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        const char* sourceStr;
        switch (source) {
            case GL_DEBUG_SOURCE_API:                sourceStr = "API"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:      sourceStr = "Window System"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:    sourceStr = "Shader Compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:        sourceStr = "Third Party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:        sourceStr = "Application"; break;
            case GL_DEBUG_SOURCE_OTHER:              sourceStr = "Other"; break;
            default:                                 sourceStr = "---"; break;
        }

        const char* typeStr;
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:                   typeStr = "Error"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:     typeStr = "Deprecated Behaviour"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:      typeStr = "Undefined Behaviour"; break;
            case GL_DEBUG_TYPE_PORTABILITY:             typeStr = "Portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:             typeStr = "Performance"; break;
            case GL_DEBUG_TYPE_MARKER:                  typeStr = "Marker"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:              typeStr = "Push Group"; break;
            case GL_DEBUG_TYPE_POP_GROUP:               typeStr = "Pop Group"; break;
            case GL_DEBUG_TYPE_OTHER:                   typeStr = "Other"; break;
            default:                                    typeStr = "---"; break;
        }

        const char* severityStr;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:            severityStr = "high"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:          severityStr = "medium"; break;
            case GL_DEBUG_SEVERITY_LOW:             severityStr = "low"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:    severityStr = "notification"; break;
            default:                                severityStr = "---"; break;
        }

        ENG_LOG_DEBUG("GL - [{}], severity: {} (source: {}); {}: {}\n", typeStr, severityStr, sourceStr, id, message);
    }

    void GLAPIENTRY glDebugCallback_simple(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
        //if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
        ENG_LOG_DEBUG("GL - [{}] type = 0x{:x}, severity = 0x{:x}, message = {}\n", (type == GL_DEBUG_TYPE_ERROR ? "Error" : "Message"), type, severity, message);
    }

    void glErrorCheck_(const char* file, int line) {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            const char* msg;

            switch (err) {
                case GL_INVALID_ENUM:                       msg = "INVALID ENUM"; break;
                case GL_INVALID_VALUE:                      msg = "INVALID VALUE"; break;
                case GL_INVALID_OPERATION:                  msg = "INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW:                     msg = "STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW:                    msg = "STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY:                      msg = "OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:      msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
                default:                                    msg = "UNKNOWN ERROR"; break;
            }

            ENG_LOG_ERROR("GL Error - {} ({}) | {} ({})\n", msg, err, file, line);
        }
    }

}//namespace eng
