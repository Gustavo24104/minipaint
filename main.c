#include <stdio.h>

// #include "glad/include/glad/gl.h"
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>
#include <time.h>


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

#define PRETO {0, 0, 0}
#define BRANCO {255, 255, 255}
#define VERMELHO {255, 0, 0}
#define VERDE {0, 255, 0}
#define AZUL {0, 0, 255}
#define AMARELO {255, 255, 0}
#define CIANO {0, 255, 255}
#define MAGENTA {255, 0, 255}
#define ALTURA 800
#define LARGURA 600

// variaveis globais D:
unsigned char canvas[ALTURA * LARGURA * 3], tmpBuffer[ALTURA * LARGURA * 3]; // buffer pra fazer preview de linha, retangulo etc
enum Tool estadoAtual = pincel;
enum BrushSize tamanhoAtual = curto;
GLdouble xMouse, yMouse; // global pra poder interpolar qdo necessário
Color corAtual = PRETO; // começa no preto
const Color corBG = {120, 114, 118}; // bg fossil
bool previewingDraw = false;
double startX, startY;
void (*drawFun)(int, int, int, int, Color); // acabei percebendo q toda funcao de desenho das ferramentas tinha essa...
// ...cara, entao pensei q fosse melhor fazer com um ponteiro, ai da pra chamar "uma coisa so" ao inves de...
// ...ficar fazendo vários if/else pra tudo


void setPixel(int x, int y, Color cor) {
    if (x < 0 || x >= ALTURA || y < 0 || y >= LARGURA) return;
    int index = (y * ALTURA + x) * 3;
    unsigned char *buff = !previewingDraw ? canvas : tmpBuffer;
    buff[index] = cor.r;
    buff[index + 1] = cor.g;
    buff[index + 2] = cor.b;
}

void putPixel(int x, int y, Color cor) {
    int area = (int) tamanhoAtual - 1;
    for (int dy = -area; dy <= area; dy++)
        for (int dx = -area; dx <= area; dx++)
            setPixel(x + dx, y + dy, cor);
}

Color getPixel(int x, int y) {
    unsigned char *buff = previewingDraw ? tmpBuffer : canvas;
    Color ret;
    int index = (y * ALTURA + x) * 3;
    ret.r = buff[index];
    ret.g = buff[index + 1];
    ret.b = buff[index + 2];
    return ret;
}

void DrawLineDDA(int x1, int y1, int x2, int y2, Color cor) {
    int dx = x2 - x1, dy = y2 - y1;
    int steps = abs(dx) >= abs(dy) ? abs(dx) : abs(dy);
    float xInc = (float) dx / (float) steps, yInc = (float) dy / (float) steps;
    float xPos = (float) x1, yPos = (float) y1;

    for (int i = 0; i < steps; ++i) {
        putPixel((int) xPos, (int) yPos, cor);
        xPos += xInc;
        yPos += yInc;
    }
}


void DrawLineBresenham(int x1, int y1, int x2, int y2, Color c) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1: -1;
    int sy = y1 < y2 ? 1: -1;
    int dist = dx - dy;

    while (true) {
        putPixel(x1, y1, c);
        if (x1 == x2 && y1 == y2) { // Caso tenha chegado no destino para
            break;
        }
        int e2 = 2 * dist; // Multiplica por 2 pra não ter q tratar com valores quebrados(0.5), dai fica otimizada a func
        if (e2 > -dy) { // avanca X
            dist -= dy;
            x1 += sx;
        }
        if (e2 < dx) { // avanca y
            dist += dx;
            y1 += sy;
        }
    }
}

void DrawRectangleVazio(int x1, int y1, int x2, int y2, Color c) {
    DrawLineBresenham(x1, y1, x2, y1, c);
    DrawLineBresenham(x2, y1, x2, y2, c);
    DrawLineBresenham(x1, y2, x2, y2, c);
    DrawLineBresenham(x1, y1, x1, y2, c);
}

void DrawRectangleCheio(int x1, int y1, int x2, int y2, Color c) {
    DrawLineBresenham(x1, y1, x2, y1, c);
    DrawLineBresenham(x2, y1, x2, y2, c);
    DrawLineBresenham(x1, y2, x2, y2, c);
    DrawLineBresenham(x1, y1, x1, y2, c);
    // faz algo pra preencher... (flood fill?)
}

void DrawCircleVazio(int x1, int y1, int x2, int y2, Color c) {
    int r = (int) round(sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))); //Pitagoras Sqrt((x2 − x1)^2 + (y2 − y1)^2)
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y) {
        putPixel(x1 + x, y1 + y, c);
        putPixel(x1 - x, y1 + y, c);
        putPixel(x1 + x, y1 - y, c);
        putPixel(x1 - x, y1 - y, c);
        putPixel(x1 + y, y1 + x, c);
        putPixel(x1 - y, y1 + x, c);
        putPixel(x1 + y, y1 - x, c);
        putPixel(x1 - y, y1 - x, c);

        if (d < 0) { // continua na mesma linha
            d += 2 * x + 3;
        } else { // desce uma linha
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

//TODO
void DrawCircleCheio(int x1, int y1, int x2, int y2, Color c) {

}


// inside se baseia na cor
bool inside(int x, int y, Color c)  {
    if (x < 0 || x >= ALTURA || y < 0 || y >= LARGURA) return false;
    Color o = getPixel(x, y);
    return c.r == o.r && c.g == o.g && c.b == o.b;
}

// auxiliar do flood fill
void scan(int lx, int rx, int y, Color cor, int *stack, int *stackIdx) {
    bool added = false;
    for(int x = lx; x <= rx; ++x) {
        if(!inside(x, y, cor)) {
            added = false;
        } else if (!added) {
            stack[*stackIdx] = x;
            stack[(*stackIdx) + 1] = y;
            *stackIdx += 2;
            added = true;
        }
    }
}



// https://en.wikipedia.org/wiki/Flood_fill#Span_filling
void FloodFill(int xStart, int yStart, Color newColor) {
    Color startColor = getPixel(xStart, yStart);
    // if (compareColor(startColor, newColor)) return;
    if(inside(xStart, yStart, newColor)) return;
    int *stack = malloc(ALTURA * LARGURA * 2 * sizeof(int)); //x, y
    int stackIdx = 0;
    stack[stackIdx++] = xStart;
    stack[stackIdx++] = yStart;

    while(stackIdx > 0) {
        int y = stack[--stackIdx], x = stack[--stackIdx];
        printf("%d\n", stackIdx);
        int lx = x;
        while(inside(lx - 1, y, startColor)) {
            setPixel(lx - 1, y, newColor); // set pixel pq nao importa o tamanho do pincel
            lx--;
        }
        while(inside(x, y, startColor)) {
            setPixel(x, y, newColor);
            x++;
        }
        scan(lx, x - 1, y + 1, startColor, stack, &stackIdx);
        scan(lx, x - 1, y - 1, startColor, stack, &stackIdx);
    }
    free(stack);
}


void clearCanvas() {
    // memset(buff, 125, ALTURA * LARGURA * 3);
    // for (int i = 0; i < ALTURA * LARGURA * 3; ++i) {
    //     buff[i] = 255;
    // }
    for (int i = 0; i < ALTURA * LARGURA * 3; i += 3) {
        canvas[i] = corBG.r;
        canvas[i + 1] = corBG.g;
        canvas[i + 2] = corBG.b;
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
            drawFun = DrawLineBresenham;
        } else if(key == GLFW_KEY_R) {
            estadoAtual = retanguloVazio;
            drawFun = DrawRectangleVazio;
        } else if(key == GLFW_KEY_O) {
             estadoAtual = circuloVazio;
             drawFun = DrawCircleVazio;
        } else if(key == GLFW_KEY_F) {
            estadoAtual = baldeDeTinta;
        } else if(key == GLFW_KEY_M) {
            corAtual = (Color) MAGENTA;
        } else if(key == GLFW_KEY_1) {
            tamanhoAtual = curto;
        } else if(key == GLFW_KEY_2) {
            tamanhoAtual = medio;
        } else if(key == GLFW_KEY_3) {
            tamanhoAtual = grosso;
        }
    }
}


// mouse mexendo
void mousePosCallback(GLFWwindow *janela, double x, double y) {
    switch(estadoAtual) {
        case pincel: {
            if(glfwGetMouseButton(janela, 0)) { // "arrastar"
                putPixel((int) xMouse, (int) yMouse, corAtual);
                // interpolação...
                if(x != xMouse || y != yMouse) {
                    double dist = sqrt(pow(x - xMouse, 2) + pow(y - yMouse, 2));
                    int amostras = (int) (dist / tamanhoAtual);

                    // parametrização!! :D
                    for (int i = 0; i <= amostras; ++i) {
                        double t = (double) i / (double) amostras;
                        double vx = x * (1 - t) + xMouse * t;
                        double vy = y * (1 - t) + yMouse * t;
                        putPixel((int) vx, (int) vy, corAtual);
                    }
                }
            }
            break;
        }
        case borracha: {
            if(glfwGetMouseButton(janela, 0)) {
                putPixel((int) xMouse, (int) yMouse, corBG);
                // interpolação...
                if(x != xMouse || y != yMouse) {
                    double dist = sqrt(pow(x - xMouse, 2) + pow(y - yMouse, 2));
                    int amostras = (int) (dist / tamanhoAtual);

                    // parametrização!! :D
                    for (int i = 0; i <= amostras; ++i) {
                        double t = (double) i / (double) amostras;
                        double vx = x * (1 - t) + xMouse * t;
                        double vy = y * (1 - t) + yMouse * t;
                        putPixel((int) vx, (int) vy, corBG);
                    }
                }
            }
            break;
        }
        default: ;
    }
}


// clique único
void mouseButtonCallback(GLFWwindow *janela, int button, int action, int mods) {
    switch(estadoAtual) {
        case pincel: {
            if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
                putPixel((int) xMouse, (int) yMouse, corAtual);
                // como eh so um clique (o mouse nao ta mexendo) nao precisa interpolar...
            }
            break;
        }
        case borracha: {
            if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
                putPixel((int) xMouse, (int) yMouse, corBG);
            }
            break;
        } case baldeDeTinta: {
            FloodFill((int) xMouse, (int) yMouse, corAtual);
            break;
        }
        default: { // Eu acabei percebendo que todo código pra essas ferramentas tava ficando igual, ...
            // ...entao decidi usar ponteiro de funcao o.O
            if(action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT && previewingDraw) {
                previewingDraw = false;
                drawFun( (int) startX, (int) startY, (int) xMouse, (int) yMouse, corAtual);
            } else if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && !previewingDraw) {
                glfwGetCursorPos(janela, &startX, &startY);
                previewingDraw = true;
            }
        }
        //
        // case linha: {
        //     if(action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT && previewingDraw) {
        //         previewingDraw = false;
        //         DrawLineDDA( (int) startX, (int) startY, (int) xMouse, (int) yMouse, corAtual);
        //     } else if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && !previewingDraw) {
        //         glfwGetCursorPos(janela, &startX, &startY);
        //         previewingDraw = true;
        //     }
        //     break;
        // }
        // case retanguloVazio: {
        //     if(action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT && previewingDraw) {
        //         previewingDraw = false;
        //         DrawRectangleVazio( (int) startX, (int) startY, (int) xMouse, (int) yMouse, corAtual);
        //     } else if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && !previewingDraw) {
        //         glfwGetCursorPos(janela, &startX, &startY);
        //         previewingDraw = true;
        //     }
        //     break;
        // }
        // case retanguloCheio: {
        //
        //     break;
        // }
        // case circuloVazio: {
        //
        //     break;
        // }
        // case circuloCheio: {
        //
        //     break;
        // }
        // case baldeDeTinta: { // floodfill (nao recursivo - como otimizar?)
        //
        //     break;
        // }
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

    janela = glfwCreateWindow(ALTURA, LARGURA, "miniPaint!", NULL, NULL);

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
    // glOrtho(0.0f, ALTURA, LARGURA, 0.0f, 0.0f, 1.0f);


    // A ideia aqui eh usar uma textura, assim nao tem que salvar todos os pixels que tamo colocando
    // todas as funções aqui sao pra configurar a textura
    GLuint canvasTextures, tmpTxt;
    glGenTextures(1, &canvasTextures);
    glBindTexture(GL_TEXTURE_2D, canvasTextures);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ALTURA, LARGURA, 0,
    GL_RGB, GL_UNSIGNED_BYTE, canvas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // borda da tela
    DrawRectangleVazio(0, 0, 799, 599, (Color) {255, 0, 0});

    // eh melhor usar essas funções de callback do que ficar fazendo polling no laço principal
    glfwSetKeyCallback(janela, HandleKeyboard);
    glfwSetCursorPosCallback(janela, mousePosCallback);
    glfwSetMouseButtonCallback(janela, mouseButtonCallback);

    // main loop
    while(!glfwWindowShouldClose(janela)) {

        glfwPollEvents();
        glfwGetCursorPos(janela, &xMouse, &yMouse); // preciso disso pra fazer interpolacao qdo desenhar

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

        // renderizcao daquilo de previews
        // uso um outro buffer temporario pra nao ficar um efeito de desenahr tudo
        unsigned char *render = canvas;
        if(previewingDraw) {
            memcpy(tmpBuffer, canvas, ALTURA * LARGURA * 3);
            render = tmpBuffer;
            // ponteiro de funcao, pq tava tudo ficando igual
            drawFun((int) startX, (int) startY, (int) xMouse, (int) yMouse, corAtual);
            // if(estadoAtual == linha) { // eu acho q tem como tacar isso aqui pro callback de posicao do mouse mas meh
            //     DrawLineDDA((int) startX, (int) startY, (int) xMouse, (int) yMouse, corAtual);
            // } else if (estadoAtual == retanguloVazio) {
            //     DrawRectangleVazio((int) startX, (int) startY, (int) xMouse, (int) yMouse, corAtual);
            // } else if(estadoAtual == retanguloCheio) {
            //
            // }
            render = tmpBuffer;
        }
        // mais coisa da textura, atualizando ela no buffer e enviando pro oepnGL dnv
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, canvasTextures);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ALTURA, LARGURA,
            GL_RGB, GL_UNSIGNED_BYTE, render);


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
