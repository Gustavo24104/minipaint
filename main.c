#include <stdio.h>

// #include "glad/include/glad/gl.h"
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <math.h>


enum Tool {
    pincel,
    borracha,
    linha,
    retanguloVazio,
    retanguloCheio,
    circuloVazio,
    circuloCheio,
    baldeDeTinta,
};

enum BrushSize {
    curto = 1,
    medio = 2,
    grosso = 3,
};



// variaveis globais D:
unsigned char canvas[800 * 600 * 3];
enum Tool estadoAtual = pincel;
enum BrushSize tamahnoAtual = curto;
GLdouble xMouse, yMouse, lastX, lastY;


//TODO: Alterar cores
void putPixel(int x, int y, int r, int g, int b) {
    int index = (y * 800 + x) * 3;
    switch(tamahnoAtual) {
        case curto: {
            canvas[index] = r;
            canvas[index + 1] = g;
            canvas[index + 2] = b;
            break;
        } case medio: { //TODO!
          break;
        } case grosso: { //TODO!
            break;
        }
    }
}

// r, g, b: out params
void getPixel(int x, int y, int *r, int *g, int *b) {
    int index = (y * 800 + x) * 3;
    *r = canvas[index];
    *g = canvas[index + 1];
    *b = canvas[index + 2];
}



void mouseClick() {
    switch(estadoAtual) {
        case pincel: {
            putPixel((int) xMouse, (int) yMouse, 0, 0, 0);
            // interpolação...
            if(lastX != xMouse || lastY != yMouse) {
                double dist = sqrt(pow(lastX - xMouse, 2) + pow(lastY - yMouse, 2));
                int amostras = (int) (dist / tamahnoAtual);

                // parametrização!! :D
                for (int i = 0; i <= amostras; ++i) {
                    double t = (double) i / (double) amostras;
                    double vx = lastX * (1 - t) + xMouse * t;
                    double vy = lastY * (1 - t) + yMouse * t;
                    putPixel((int) vx, (int) vy, 0, 0, 0);
                }
            }
            break;
        }
        case borracha: {
            putPixel((int) xMouse, (int) yMouse, 255, 255, 255);
            // interpolação...
            if(lastX != xMouse || lastY != yMouse) {
                double dist = sqrt(pow(lastX - xMouse, 2) + pow(lastY - yMouse, 2));
                int amostras = (int) (dist / tamahnoAtual);

                // parametrização!! :D
                for (int i = 0; i <= amostras; ++i) {
                    double t = (double) i / (double) amostras;
                    double vx = lastX * (1 - t) + xMouse * t;
                    double vy = lastY * (1 - t) + yMouse * t;
                    putPixel((int) vx, (int) vy, 255, 255, 255);
                }
            }
            break;
        }
        case linha: {
            // pra fazer o preview: possivelmente usar tipo um "canvas temporario" e desenhar nele
            // ou entao, desenhar e limpar a linha qdo o mouse se mexer (talvez essa seja melhor)

            break;
        }
        case retanguloVazio: {

            break;
        }
        case retanguloCheio: {

            break;
        }
        case circuloVazio: {

            break;
        }
        case circuloCheio: {

            break;
        }
        case baldeDeTinta: { // floodfill (nao recursivo - como otimizar?)

            break;
        }
        default: ;
    }
}



void DrawLineDDA(int x1, int y1, int x2, int y2, int r, int g, int b) {
    int dx = x2 - x1, dy = y2 - y1;
    int steps = abs(dx) >= abs(dy) ? dx : dy;
    float xInc = (float) dx / (float) steps, yInc = (float) dy / (float) steps;
    float xPos = (float) x1, yPos = (float) y1;

    for (int i = 0; i < steps; ++i) {
        putPixel((int) xPos, (int) yPos, r, g, b);
        xPos += xInc;
        yPos += yInc;
    }
}

void DrawLineBresenham(int x1, int y1, int x2, int y2, int r, int g, int b) {

}


void DrawRectangle(int x1, int y1, int x2, int y2, int r, int g, int b) {
    DrawLineDDA(x1, y1, x2, y1, r, g, b);
    DrawLineDDA(x2, y1, x2, y2, r, g, b);
    DrawLineDDA(x1, y2, x2, y2, r, g, b);
    DrawLineDDA(x1, y1, x1, y2, r, g, b);
}


void clearCanvas() {
    for (int i = 0; i < 800 * 600 * 3; ++i) {
        canvas[i] = 255;
    }
}

void HandleKeyboard(GLFWwindow *janela) {
    if(glfwGetKey(janela, GLFW_KEY_C)) {
        clearCanvas();
    } else if(glfwGetKey(janela, GLFW_KEY_B)) {
        estadoAtual = borracha;
    } else if(glfwGetKey(janela, GLFW_KEY_P)) {
        estadoAtual = pincel;
    }
}


int main(void) {
    clearCanvas();
    GLFWwindow *janela;
    double delta = 0;
    double lastTime = 0;
    long int frameCount = 0;

    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    janela = glfwCreateWindow(800, 600, "miniPaint!", NULL, NULL);

    if(!janela) {
        glfwTerminate();
        return -1;
    }


    // cursor
    unsigned char pixels[8 * 8 * 4];
    for (int i = 0; i < 8 * 8 * 4; i += 4) {
        pixels[i] = 0; // red
        pixels[i + 1] = 0;  // green
        pixels[i + 2] = 0;  // blue
        pixels[i + 3] = 255; // alpha
    }
    GLFWimage image;
    image.width = 8;
    image.height = 8;
    image.pixels = pixels;
    GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);
    glfwSetCursor(janela, cursor);

    glfwMakeContextCurrent(janela);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glOrtho(0.0f, 800, 600, 0.0f, 0.0f, 1.0f);

    // A ideia aqui eh usar uma textura, assim nao tem que salvar todos os pixels que tamo colocando
    // todas as funções aqui sao pra configurar a textura
    GLuint canvasTextures;
    glGenTextures(1, &canvasTextures);
    glBindTexture(GL_TEXTURE_2D, canvasTextures);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0,
    GL_RGB, GL_UNSIGNED_BYTE, canvas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    DrawRectangle(0, 0, 799, 599, 255, 0, 0);
    while(!glfwWindowShouldClose(janela)) {

        glfwPollEvents();
        glfwGetCursorPos(janela, &xMouse, &yMouse);

        double curr = glfwGetTime();
        frameCount++;
        delta = curr - lastTime;
        if(delta >= 1) {
            printf("frames: %ld | draw time: %lf ms \n", frameCount, 1000.0f / (float) frameCount);
            frameCount = 0;
            lastTime = curr;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, canvasTextures);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 800, 600,
            GL_RGB, GL_UNSIGNED_BYTE, canvas);


        // render
        // ai cria um quadrado q cobre a tela inteira e "cola" a textura do canvas em cima dele
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 1.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, -1.0f);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, -1.0f);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);

        glEnd();
        glfwSwapBuffers(janela);
        glfwPollEvents();


        if(glfwGetMouseButton(janela, 0)) {
            mouseClick();
        }

        HandleKeyboard(janela);

        // DrawRectangle(0, 0, 799, 599);

        lastX = xMouse;
        lastY = yMouse;

    }

    glDisable(GL_TEXTURE_2D);

    glfwTerminate();
    return 0;
}
