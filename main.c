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

typedef struct {
    int r, g, b;
} Color;



// variaveis globais D:
unsigned char canvas[800 * 600 * 3];
enum Tool estadoAtual = pincel;
enum BrushSize tamahnoAtual = curto;
GLdouble xMouse, yMouse;
Color corAtual = {0, 0, 0}; // começa no preto
const Color corBG = {255, 255, 255};


//TODO: Alterar cores
void putPixel(int x, int y, Color cor) {
    int index = (y * 800 + x) * 3;
    switch(tamahnoAtual) {
        case curto: {
            canvas[index] = cor.r;
            canvas[index + 1] = cor.g;
            canvas[index + 2] = cor.b;
            break;
        } case medio: { //TODO!
          break;
        } case grosso: { //TODO!
            break;
        }
    }
}

// r, g, b: out params
void getPixel(int x, int y, Color *color) {
    int index = (y * 800 + x) * 3;
    color->r = canvas[index];
    color->g = canvas[index + 1];
    color->b = canvas[index + 2];
}


void DrawLineDDA(int x1, int y1, int x2, int y2, Color cor) {
    int dx = x2 - x1, dy = y2 - y1;
    int steps = abs(dx) >= abs(dy) ? dx : dy;
    float xInc = (float) dx / (float) steps, yInc = (float) dy / (float) steps;
    float xPos = (float) x1, yPos = (float) y1;

    for (int i = 0; i < steps; ++i) {
        putPixel((int) xPos, (int) yPos, cor);
        xPos += xInc;
        yPos += yInc;
    }
}


//TODO: Implementar
void DrawLineBresenham(int x1, int y1, int x2, int y2, Color c) {

}

void DrawRectangle(int x1, int y1, int x2, int y2, Color c) {
    DrawLineDDA(x1, y1, x2, y1, c);
    DrawLineDDA(x2, y1, x2, y2, c);
    DrawLineDDA(x1, y2, x2, y2, c);
    DrawLineDDA(x1, y1, x1, y2, c);
}


void clearCanvas() {
    for (int i = 0; i < 800 * 600 * 3; ++i) {
        canvas[i] = 255;
    }
}

//TODO: Isso tem q ser UI dps
void HandleKeyboard(GLFWwindow *janela, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_C) {
            clearCanvas();
        } else if(key == GLFW_KEY_B) {
            estadoAtual = borracha;
        } else if(key == GLFW_KEY_P) {
            estadoAtual = pincel;
        } else if(key == GLFW_KEY_L) {
            estadoAtual = linha;
        }
    }
}


// mouse mexendo
void mousePosCallback(GLFWwindow *janela, double x, double y) {
    if(glfwGetMouseButton(janela, 0)) { // "arrastar"
        switch(estadoAtual) {
            case pincel: {
                Color c = {0, 0, 0};
                putPixel((int) xMouse, (int) yMouse, c);
                // interpolação...
                if(x != xMouse || y != yMouse) {
                    double dist = sqrt(pow(x - xMouse, 2) + pow(y - yMouse, 2));
                    int amostras = (int) (dist / tamahnoAtual);

                    // parametrização!! :D
                    for (int i = 0; i <= amostras; ++i) {
                        double t = (double) i / (double) amostras;
                        double vx = x * (1 - t) + xMouse * t;
                        double vy = y * (1 - t) + yMouse * t;
                        putPixel((int) vx, (int) vy, c);
                    }
                }
                break;
            }
            case borracha: {
                putPixel((int) xMouse, (int) yMouse, corBG);
                // interpolação...
                if(x != xMouse || y != yMouse) {
                    double dist = sqrt(pow(x - xMouse, 2) + pow(y - yMouse, 2));
                    int amostras = (int) (dist / tamahnoAtual);

                    // parametrização!! :D
                    for (int i = 0; i <= amostras; ++i) {
                        double t = (double) i / (double) amostras;
                        double vx = x * (1 - t) + xMouse * t;
                        double vy = y * (1 - t) + yMouse * t;
                        putPixel((int) vx, (int) vy, corBG);
                    }
                }
                break;
            }
            case linha: {
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
}

// clique único
void mouseButtonCallback(GLFWwindow *janela, int button, int action, int mods) {
    if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        switch(estadoAtual) {
            case pincel: {
                Color c = {0, 0, 0};
                putPixel((int) xMouse, (int) yMouse, c);
                // como eh so um clique (o mouse nao ta mexendo) nao precisa interpolar...
                break;
            }
            case borracha: {
                putPixel((int) xMouse, (int) yMouse, corBG);
                break;
            }
            case linha: {
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

    // A ideia aqui eh usar uma textura, assim nao tem que salvar todos os pixels que tamo colocando
    // todas as funções aqui sao pra configurar a textura
    GLuint canvasTextures;
    glGenTextures(1, &canvasTextures);
    glBindTexture(GL_TEXTURE_2D, canvasTextures);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0,
    GL_RGB, GL_UNSIGNED_BYTE, canvas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // borda da tela
    DrawRectangle(0, 0, 799, 599, (Color) {255, 0, 0});

    // eh melhor usar essas funções de callback do que ficar fazendo polling no laço principal
    glfwSetCursorPosCallback(janela, mousePosCallback);
    glfwSetKeyCallback(janela, HandleKeyboard);
    glfwSetMouseButtonCallback(janela, mouseButtonCallback);

    // main loop
    while(!glfwWindowShouldClose(janela)) {

        glfwPollEvents();
        glfwGetCursorPos(janela, &xMouse, &yMouse); // preciso disso pra fazer interlopacao qdo desenhar

        // eh interessante ver os frames que a aplicação ta pegando
        // nao serve mto pra medir performance eu acho, mas da pra ter uma ideia
        double curr = glfwGetTime();
        frameCount++;
        delta = curr - lastTime;
        if(delta >= 1) {
            printf("frames: %ld | draw time: %lf ms \n", frameCount, 1000.0f / (float) frameCount);
            frameCount = 0;
            lastTime = curr;
        }

        // mais coisa da textura, atualizando ela no buffer e enviando pro oepnGL dnv
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
    }

    glDisable(GL_TEXTURE_2D);

    glfwTerminate();
    return 0;
}
