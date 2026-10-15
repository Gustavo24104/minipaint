#include <stdio.h>

// #include "glad/include/glad/gl.h"
#include <stdlib.h>
#include <GLFW/glfw3.h>



void DrawLineDDA(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1, dy = y2 - y1;
    int steps = abs(dx) >= abs(dy) ? dx : dy;
    float xInc = (float) dx / (float) steps, yInc = (float) dy / (float) steps;
    float xPos = (float) x1, yPos = (float) y1;

    for (int i = 0; i < steps; ++i) {
        glVertex2i( (int) xPos, (int) yPos);
        xPos += xInc;
        yPos += yInc;
    }
}


void DrawRectangle(int x1, int y1, int x2, int y2) {
    DrawLineDDA(x1, y1, x2, y1);
    DrawLineDDA(x2, y1, x2, y2);
    DrawLineDDA(x1, y2, x2, y2);
    DrawLineDDA(x1, y1, x1, y2);
}


int main(void) {
    GLFWwindow *janela;
    double delta = 0;
    double lastTime = 0;
    long int frameCount = 0;

    if(!glfwInit()) return -1;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    janela = glfwCreateWindow(800, 600, "Teste!", NULL, NULL);

    if(!janela) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(janela);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, 800, 600, 0.0f, 0.0f, 1.0f);

    glPointSize(1);
    while(!glfwWindowShouldClose(janela)) {

        glfwPollEvents();
        // printf("Mouse: %d\n", glfwGetMouseButton(janela, 0));
        double curr = glfwGetTime();
        frameCount++;
        delta = curr - lastTime;
        if(delta >= 1) {
            printf("frames: %ld | draw time: %lf ms \n", frameCount, 1000.0f / (float) frameCount);
            frameCount = 0;
            lastTime = curr;
        }

        glClear(GL_COLOR_BUFFER_BIT);


        // render
        glBegin(GL_POINTS);
        if(glfwGetMouseButton(janela, 0)) {
            GLdouble x, y;
            glfwGetCursorPos(janela, &x, &y);
            glVertex2d(x, y);
        }
        glEnd();
        glfwSwapBuffers(janela);

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
