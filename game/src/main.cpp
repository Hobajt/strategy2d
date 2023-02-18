#include <stdio.h>

#include <engine/engine.h>

#include <glm/glm.hpp>

void dummyWindow();

int main(int argc, char** argv) {
    printf("Gmae executable.\n");

    glm::vec3 v = glm::vec3(1.f, 2.f, 3.f);
    printf("v = (%.2f, %.2f, %.2f)\n", v.x ,v.y, v.z);

    FT_Library ft;
    FT_Face face;

    dummyWindow();
    
    return 0;
}

void dummyWindow() {
    int width = 640;
    int height = 480;
    const char* name = "test";
    int samples = 1;


    if(!glfwInit()) {
        printf("Failed to initialize GLFW.\n");
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, samples);

    GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window.\n");
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize Glad.\n");
        glfwTerminate();
        return;
    }

    printf("OpenGL %s, %s (%s)\n", glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_VENDOR));
    printf("GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glViewport(0, 0, width, height);

    printf("ALL OK\n");

    while(!glfwWindowShouldClose(window)) {
        if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    printf("Done, terminating...\n");
}
