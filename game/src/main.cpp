#include <stdio.h>

#include <engine/engine.h>

void dummyWindow();

int main(int argc, char** argv) {
    printf("Gmae executable.\n");

    eng::Log::Initialize();

    dummyWindow();

    LOG_TRACE("test app log");
    LOG_INFO("test app log");
    LOG_WARN("test app log");
    LOG_ERROR("test app log");
    LOG_CRITICAL("test app log");
    ENG_LOG_INFO("test engine log");

    // ASSERT_MSG(1 == 2, "seems legit to me...");
    
    return 0;
}

void dummyWindow() {
    // int width = 640;
    // int height = 480;
    // const char* name = "test";
    // int samples = 1;


    // if(!glfwInit()) {
    //     printf("Failed to initialize GLFW.\n");
    //     return;
    // }

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // glfwWindowHint(GLFW_SAMPLES, samples);

    // GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
    // if (!window) {
    //     printf("Failed to create GLFW window.\n");
    //     glfwTerminate();
    //     return;
    // }
    // glfwMakeContextCurrent(window);

    // if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    //     printf("Failed to initialize Glad.\n");
    //     glfwTerminate();
    //     return;
    // }

    // printf("OpenGL %s, %s (%s)\n", glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_VENDOR));
    // printf("GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // glViewport(0, 0, width, height);

    // printf("ALL OK\n");

    // while(!glfwWindowShouldClose(window)) {
    //     if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    //         glfwSetWindowShouldClose(window, true);
    //     }

    //     glfwSwapBuffers(window);
    //     glfwPollEvents();
    // }

    // printf("Done, terminating...\n");
}
