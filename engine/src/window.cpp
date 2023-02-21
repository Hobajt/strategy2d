#include "engine/core/window.h"

#include <memory>
#include <exception>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "engine/utils/setup.h"
#include "engine/utils/gl_error.h"

#define WINDOW_HANDLE_CHECK() ASSERT_MSG(window != nullptr, "Attempting to use uninitialized window.")

namespace eng {

    //Handles window creation along with GLFW & glad initialization.
    bool WindowInit(int width, int height, int samples, const char* name, GLFWwindow*& out_window);

    //Converts GLFW error code into a name string.
    const char* GLFW_errorString(int code);

    //Local resize callback, used to call user defined resize callback.
    void onResizeCallback(GLFWwindow* window, int width, int height);

    //===== Window =====

    Window& Window::Get() {
        static Window instance = Window();
        return instance;
    }

    void Window::Initialize(int width, int height, const char* name, int samples) {
        if(IsInitialized()) {
            Release();
        }

        UpdateSize(width, height);
        if(!WindowInit(width, height, samples, name, window)) {
            throw std::exception();
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, onResizeCallback);
        // Input::Get().InitCallbacks(*this);

        ENG_LOG_TRACE("[C] Window '{}'", name);
    }

    Window::~Window() {
        Release();
    }

    void Window::Close() {
        WINDOW_HANDLE_CHECK();
        glfwSetWindowShouldClose(window, true);
    }

    bool Window::ShouldClose() {
        WINDOW_HANDLE_CHECK();
        return glfwWindowShouldClose(window);
    }

    void Window::SwapAndPoll() {
        WINDOW_HANDLE_CHECK();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void Window::Resize(int width, int height) {
        UpdateSize(width, height);
        UpdateViewport();
        if(resizeHandler != nullptr) {
            resizeHandler->OnResize(width, height);
        }
    }

    void Window::UpdateSize(int width, int height) {
        size = glm::ivec2(width, height);
        aspect = (size.x > size.y) ? glm::vec2(size.y / (float)size.x, 1.f) : glm::vec2(1.f, size.x / (float)size.y);
    }

    glm::ivec2 Window::CursorPos() const {
        double x,y;
        glfwGetCursorPos(window, &x, &y);
        return glm::ivec2((int)x, (int)y);
    }

    glm::ivec2 Window::CursorPos_VFlip() const {
        glm::vec2 v = CursorPos();
        v.y = size.y - v.y - 1;
        return v;
    }

    void Window::UpdateViewport() {
        glViewport(0, 0, size.x, size.y);
    }

    void Window::UpdateViewport(const glm::ivec2& sz) {
        glViewport(0, 0, sz.x, sz.y);
    }

    void Window::SetClearColor(const glm::vec4& clr) {
        glClearColor(clr.x, clr.y, clr.z, clr.w);
    }

    void Window::Clear() {
        //TODO: maybe make clear flags modifiable
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    bool Window::GetKeyState(int keycode) const {
        return glfwGetKey(window, keycode) != GLFW_RELEASE;
    }

    bool Window::GetMouseKeyState(int keycode) const {
        return glfwGetMouseButton(window, keycode) != GLFW_RELEASE;
    }

    glm::vec2 Window::GetMousePos() const {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        return glm::vec2(xpos, ypos);
    }

    void Window::Release() noexcept {
        if(window != nullptr) {
            ENG_LOG_TRACE("[D] Window");
            glfwDestroyWindow(window);
            glfwTerminate();
            window = nullptr;
        }
    }

    //=============================================================

    bool WindowInit(int width, int height, int samples, const char* name, GLFWwindow*& window) {
        if(!glfwInit()) {
            ENG_LOG_CRITICAL("Failed to initialize GLFW.");
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_SAMPLES, samples);
        //glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
        //glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

#ifdef ENGINE_GL_DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

        window = glfwCreateWindow(width, height, name, NULL, NULL);
        if (!window) {
            ENG_LOG_CRITICAL("Failed to create GLFW window.");
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);

        //initialize glad
        if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            ENG_LOG_CRITICAL("Failed to initialize Glad.");
            glfwTerminate();
            return false;
        }

        //setup gl error callback
#ifdef ENGINE_GL_DEBUG
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if(flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugCallback, nullptr);
            // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        }
#endif

        //print general gl info
        ENG_LOG_INFO("OpenGL {}, {} ({})", (const char*)(glGetString(GL_VERSION)), (const char*)(glGetString(GL_RENDERER)), (const char*)(glGetString(GL_VENDOR)));
        ENG_LOG_INFO("GLSL {}", (const char*)(glGetString(GL_SHADING_LANGUAGE_VERSION)));

        // //list available extensions
        // GLint n=0; 
        // glGetIntegerv(GL_NUM_EXTENSIONS, &n); 
        // for (GLint i=0; i<n; i++) { 
        //     const char* extension =  (const char*)glGetStringi(GL_EXTENSIONS, i);
        //     LOG(LOG_INFO, "Ext[%d]: %s\n", i, extension);
        // } 

        // GLint maxUnits;
        // glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
        // LOG(LOG_INFO, "combined texture units: %d\n", maxUnits);
        // glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxUnits);
        // LOG(LOG_INFO, "  vertex texture units: %d\n", maxUnits);
        // glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
        // LOG(LOG_INFO, "fragment texture units: %d\n", maxUnits);


        //update viewport size
        glViewport(0, 0, width, height);

        //TODO: maybe move somewhere else or make it part of interface?
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        return true;
    }

#define ERR_MAKE_CASE(err_name) case err_name: return #err_name;
    const char* GLFW_errorString(int code) {
        switch(code) {
            ERR_MAKE_CASE(GLFW_NO_ERROR)
            ERR_MAKE_CASE(GLFW_NOT_INITIALIZED)
            ERR_MAKE_CASE(GLFW_NO_CURRENT_CONTEXT)
            ERR_MAKE_CASE(GLFW_INVALID_ENUM)
            ERR_MAKE_CASE(GLFW_INVALID_VALUE)
            ERR_MAKE_CASE(GLFW_OUT_OF_MEMORY)
            ERR_MAKE_CASE(GLFW_API_UNAVAILABLE)
            ERR_MAKE_CASE(GLFW_VERSION_UNAVAILABLE)
            ERR_MAKE_CASE(GLFW_PLATFORM_ERROR)
            ERR_MAKE_CASE(GLFW_FORMAT_UNAVAILABLE)
            ERR_MAKE_CASE(GLFW_NO_WINDOW_CONTEXT)
        }
        return "UNKNOWN_ERROR_CODE";
    }
#undef ERR_MAKE_CASE
    
    void onResizeCallback(GLFWwindow* window, int width, int height) {
        Window& w = *static_cast<Window*>(glfwGetWindowUserPointer(window));
        w.Resize(width, height);
    }

}//namespace eng
