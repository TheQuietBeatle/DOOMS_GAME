#include <GLFW/glfw3.h>
#include <stdio.h>
int main() {
    if (!glfwInit()) { printf("init fail\n"); return 2; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(1280, 720, "GLFWTest", 0, 0);
    if (!w) { printf("win fail\n"); glfwTerminate(); return 3; }
    glfwMakeContextCurrent(w);
    glfwSetWindowPos(w, -1820, 100);
    glfwSwapInterval(1);
    printf("window ok\n"); fflush(stdout);
    double start = glfwGetTime();
    int frames = 0;
    while (!glfwWindowShouldClose(w) && glfwGetTime() - start < 10.0) {
        glViewport(0, 0, 1280, 720);
        glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(w);
        glfwPollEvents();
        frames++;
    }
    printf("loop end frames=%d shouldClose=%d\n", frames, glfwWindowShouldClose(w)); fflush(stdout);
    glfwTerminate();
    return 0;
}
