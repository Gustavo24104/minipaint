#include <stdio.h>

// #include "glad/include/glad/gl.h"
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum Tool {
    pincel,
    borracha,
    linha,
    retanguloVazio,
    retanguloCheio,
    circuloVazio,
    circuloCheio,
    baldeDeTinta,
    selecionar,
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
#define UI_ALTURA 60

typedef struct {
    int x1, y1, x2, y2; // canto de cima/esquerda e canto de baixo/direita
} Botao;

// variaveis globais D:
unsigned char canvas[ALTURA * LARGURA * 3], tmpBuffer[ALTURA * LARGURA * 3]; // buffer pra fazer preview de linha, retangulo etc

unsigned char uiBuffer[ALTURA * UI_ALTURA * 3]; // buffer para a interface de cima
bool desenhandoUI = false; // qdo true o setPixel escreve no uiBuffer
bool cliqueNaUI = false; // botao do mouse foi apertado em cima da barra

// posicao dos botoes na barra: {x1, y1, x2, y2} guarda os cantos (PODIA TER FEITO ISSO COM UM FOR)
Botao botoesFerramenta[8] = { // + 44 no x1 e x2
    {8, 10, 47, 49}, // pincel
    {52, 10, 91, 49}, // borracha
    {96, 10, 135, 49}, // linha
    {140, 10, 179, 49}, // retangulo vazio
    {184, 10, 223, 49}, // retangulo cheio
    {228, 10, 267, 49}, // circulo vazio
    {272, 10, 311, 49}, // circulo cheio
    {316, 10, 355, 49}, // balde
};
Botao botoesTamanho[3] = { // + 44 no x1 e x2
    {372, 10, 411, 49}, // fino
    {416, 10, 455, 49}, // medio
    {460, 10, 499, 49}, // grosso
};
Botao botoesCor[8] = {
    {516, 7, 537, 28}, // preto
    {542, 7, 563, 28}, // branco
    {568, 7, 589, 28}, // vermelho
    {594, 7, 615, 28}, // verde
    {516, 31, 537, 52}, // azul
    {542, 31, 563, 52}, // amarelo
    {568, 31, 589, 52}, // ciano
    {594, 31, 615, 52}, // magenta
};
Botao botaoNovo = {640, 10, 679, 49};
Botao botaoSalvar = {692, 10, 731, 49};

const Color paleta[8] = {PRETO, BRANCO, VERMELHO, VERDE, AZUL, AMARELO, CIANO, MAGENTA};
const Color corUIFundo = {67, 67, 67};
const Color corBotao = {190, 190, 190};
const Color corBotaoPressionado = {255, 255, 255};
const Color corDestaque = {0, 100, 255};
const Color corIcone = {0, 0, 0};

// icones da barra: indices 0-7 sao as ferramentas(mesma ordem do enum); 8 da o clear, 9 salva e 10-12 sao as espessuras
#ifndef ASSETS
#define ASSETS "assets/"
#endif
#define TAM_ICONE 32
const char *icons[13] = {
    ASSETS "Pincel_icon.png",               // 0 - Pincel
    ASSETS "Borracha_icon.png",             // 1 - Borracha
    ASSETS "Linha_icon.png",                // 2 - Linha
    ASSETS "Retangulo_icon.png",            // 3 - Retangulo
    ASSETS "Retangulo_Preenchido_icon.png", // 4 - Ret. Cheio
    ASSETS "Circulo_icon.png",              // 5 - Circulo
    ASSETS "Circulo_Preenchido_icon.png",   // 6 - Circulo Cheio
    ASSETS "Balde_icon.png",                // 7 - Balde
    ASSETS "Resetar_icon.png",              // 8 - Resetar
    ASSETS "Salvar_icon.png",               // 9 - Salvar
    ASSETS "Linha_Fina_icon.png",           // 10 - Fino
    ASSETS "Linha_Media_icon.png",          // 11 - Medio
    ASSETS "Linha_Grossa_icon.png",         // 12 - Grosso
};
unsigned char *icones[13]; // pixels RGBA de cada icone (32x32), lidos pelo stbi_load

enum Tool estadoAtual = pincel;
enum BrushSize tamanhoAtual = curto;
const enum BrushSize tamanhos[3] = {curto, medio, grosso}; // o que cada botao de espessura escolhe
GLdouble xMouse, yMouse; // global pra poder interpolar qdo necessário
int indiceCor = 0; // posicao da cor atual na paleta, começa no preto
const Color corBG = {120, 114, 118}; // bg fossil
bool previewingDraw = false;
double startX, startY;
void (*drawFun)(int, int, int, int, Color); // acabei percebendo q toda funcao de desenho das ferramentas tinha essa...
// ...cara, entao pensei q fosse melhor fazer com um ponteiro, ai da pra chamar "uma coisa so" ao inves de...
// ...ficar fazendo vários if/else pra tudo

typedef struct {
    float m[3][3];
} Matriz;

bool selecionado = false;
unsigned char selecao[ALTURA * LARGURA * 3]; // copia dos pixels selecionados
int selX, selY, selW, selH;                  // retangulo de selecao
Matriz selMatriz;                            // translada, rotaciona e escala

void FloodFill(int xStart, int yStart, Color newColor);
void applySelecao();

void SaveImage(char *nomeArquivo) {
    if (selecionado) {
        applySelecao();
    }
    stbi_write_png(nomeArquivo, ALTURA, LARGURA, 3, canvas, ALTURA * 3);
    printf("salvo!\n");
}

void setPixel(int x, int y, Color cor) {
    unsigned char *buff;
    int alturaBuff;

    if (desenhandoUI) {
        buff = uiBuffer;
        alturaBuff = UI_ALTURA;
    } else if (previewingDraw) {
        buff = tmpBuffer;
        alturaBuff = LARGURA;
    } else {
        buff = canvas;
        alturaBuff = LARGURA;
    }
    if (x < 0 || x >= ALTURA || y < 0 || y >= alturaBuff) return;

    int index = (y * ALTURA + x) * 3;
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

    if(x1 > x2) {
        int tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if(y1 > y2) {
        int tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    for(int x = x1; x < x2; ++x) {
        for(int y = y1; y < y2; ++y) {
            setPixel(x, y, c);
        }
    }
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

void DrawCircleCheio(int x1, int y1, int x2, int y2, Color c) {
    int r = (int) round(sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))); //Pitagoras Sqrt((x2 − x1)^2 + (y2 − y1)^2)

    int x = 0;
    int y = r;
    int d = 1 - r;
    while (x <= y) {
        for (int i = x1 - x; i <= x1 + x; i++) {
            putPixel(i, y1 + y, c);
            putPixel(i, y1 - y, c);
        }

        for (int i = x1 - y; i <= x1 + y; i++) {
            putPixel(i, y1 + x, c);
            putPixel(i, y1 - x, c);
        }


        if (d < 0) { // continua na mesma linha
            d += 2 * x + 3;
        } else { // desce uma linha
            d += 2 * (x - y) + 5;
            y--;
        }

        x++;
    }

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
    if (xStart < 0 || xStart >= ALTURA || yStart < 0 || yStart >= LARGURA) return;
    Color startColor = getPixel(xStart, yStart);
    // if (compareColor(startColor, newColor)) return;
    if(inside(xStart, yStart, newColor)) return;
    int *stack = malloc(ALTURA * LARGURA * 2 * sizeof(int)); //x, y
    int stackIdx = 0;
    stack[stackIdx++] = xStart;
    stack[stackIdx++] = yStart;

    while(stackIdx > 0) {
        int y = stack[--stackIdx], x = stack[--stackIdx];
        // printf("%d\n", stackIdx);
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

// ---------------- Transformacoes ----------------

Matriz Identidade() {
    return (Matriz) {{{1, 0, 0},
                      {0, 1, 0},
                      {0, 0, 1}}};
}

Matriz Multiplica(Matriz A, Matriz B) {
    Matriz C = {0};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                C.m[i][j] += A.m[i][k] * B.m[k][j];
    return C;
}

Matriz Translacao(float tx, float ty) {
    return (Matriz) {{{1, 0, tx},
                      {0, 1, ty},
                      {0, 0, 1}}};
}

Matriz Escala(float sx, float sy) {
    return (Matriz) {{{sx, 0, 0},
                      {0, sy, 0},
                      {0, 0, 1}}};
}

Matriz Rotacao(float graus) {
    float a = graus * 3.14159265f / 180.0f;
    return (Matriz) {{{cosf(a), -sinf(a), 0},
                      {sinf(a), cosf(a), 0},
                      {0, 0, 1}}};
}

Matriz gira(Matriz M, float cx, float cy) {
    return Multiplica(Translacao(cx, cy), Multiplica(M, Translacao(-cx, -cy)));
}

void Aplica(Matriz M, float x, float y, float *nx, float *ny) {
    *nx = M.m[0][0] * x + M.m[0][1] * y + M.m[0][2] * 1;
    *ny = M.m[1][0] * x + M.m[1][1] * y + M.m[1][2] * 1;
}

Matriz Inversa(Matriz M) { //tem q usar isso aqui se nao fica cheio de buraco o resultado
    float a = M.m[0][0], b = M.m[0][1], tx = M.m[0][2];
    float c = M.m[1][0], d = M.m[1][1], ty = M.m[1][2];
    float det = a * d - b * c;
    Matriz R = Identidade();
    R.m[0][0] = d / det;
    R.m[0][1] = -b / det;
    R.m[1][0] = -c / det;
    R.m[1][1] = a / det;
    R.m[0][2] = -(R.m[0][0] * tx + R.m[0][1] * ty);
    R.m[1][2] = -(R.m[1][0] * tx + R.m[1][1] * ty);
    return R;
}

// ---------------- Selecao ----------------

// copia a regiao pro buffer da selecao e apaga ela do canvas
void IniciaSelecao(int x1, int y1, int x2, int y2) {
    if (x1 > x2) {
        int tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y1 > y2) {
        int tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= ALTURA) { //pra nao sair da tela
        x2 = ALTURA - 1;
    }
    if (y2 >= LARGURA) { //pra nao sair da tela
        y2 = LARGURA - 1;
    }
    if (x2 - x1 < 1 || y2 - y1 < 1) return; // selecao muito pequena

    selX = x1;
    selY = y1;
    selW = x2 - x1 + 1;
    selH = y2 - y1 + 1;
    for (int y = 0; y < selH; ++y)
        for (int x = 0; x < selW; ++x) {
            unsigned char *origem = &canvas[((selY + y) * ALTURA + selX + x) * 3];
            unsigned char *destino = &selecao[(y * selW + x) * 3];
            destino[0] = origem[0];
            destino[1] = origem[1];
            destino[2] = origem[2];
            setPixel(selX + x, selY + y, corBG);
        }

    selMatriz = Identidade();
    selecionado = true;
}

// os 4 cantos da selecao ja transformados pela selMatriz
void CantosSelecao(float px[4], float py[4]) {
    float cantoX[4] = {selX, selX + selW, selX + selW, selX};
    float cantoY[4] = {selY, selY, selY + selH, selY + selH};
    for (int i = 0; i < 4; ++i)
        Aplica(selMatriz, cantoX[i], cantoY[i], &px[i], &py[i]);
}

// desenha a selecao transformada
// pra cada pixel de destino aplica a matriz INVERSA pra achar de qual pixel da selecao ele veio
void DrawSelecao() {
    Matriz inversa = Inversa(selMatriz);

    // so precisa percorrer o retangulo que envolve os 4 cantos transformados
    float px[4], py[4];
    CantosSelecao(px, py);
    float minX = px[0], maxX = px[0], minY = py[0], maxY = py[0];
    for (int i = 1; i < 4; ++i) {
        if (px[i] < minX) minX = px[i];
        if (px[i] > maxX) maxX = px[i];
        if (py[i] < minY) minY = py[i];
        if (py[i] > maxY) maxY = py[i];
    }

    for (int y = (int) floorf(minY); y <= (int) ceilf(maxY); ++y)
        for (int x = (int) floorf(minX); x <= (int) ceilf(maxX); ++x) {
            float origemX, origemY;
            Aplica(inversa, x + 0.5f, y + 0.5f, &origemX, &origemY); // + 0.5 = centro do pixel
            int sx = (int) floorf(origemX) - selX;
            int sy = (int) floorf(origemY) - selY;
            if (sx < 0 || sx >= selW || sy < 0 || sy >= selH) continue; // fora da selecao

            unsigned char *p = &selecao[(sy * selW + sx) * 3];
            setPixel(x, y, (Color) {p[0], p[1], p[2]});
        }
}

// contorno da selecao: liga os 4 cantos transformados com Bresenham
void DrawBordaSelecao() {
    float px[4], py[4];
    CantosSelecao(px, py);

    enum BrushSize tamanhoSalvo = tamanhoAtual;
    tamanhoAtual = curto;
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        DrawLineBresenham((int) px[i], (int) py[i], (int) px[j], (int) py[j], corDestaque);
    }
    tamanhoAtual = tamanhoSalvo;
}

// cola a selecao no canvas
void applySelecao() {
    selecionado = false;
    previewingDraw = false;
    DrawSelecao();
}

// junta mais uma transformacao M nas que a selecao ja tem
void TransformaSelecao(Matriz M) {
    selMatriz = Multiplica(M, selMatriz);
}

// teclas da selecao; devolve true se a tecla foi usada
bool TeclaSelecao(int key) {
    float cx, cy;
    Aplica(selMatriz, selX + selW / 2.0f, selY + selH / 2.0f, &cx, &cy);

    if (key == GLFW_KEY_LEFT) TransformaSelecao(Translacao(-5, 0));
    else if (key == GLFW_KEY_RIGHT) TransformaSelecao(Translacao(5, 0));
    else if (key == GLFW_KEY_UP) TransformaSelecao(Translacao(0, -5));
    else if (key == GLFW_KEY_DOWN) TransformaSelecao(Translacao(0, 5));
    else if (key == GLFW_KEY_E) TransformaSelecao(gira(Rotacao(15), cx, cy));
    else if (key == GLFW_KEY_Q) TransformaSelecao(gira(Rotacao(-15), cx, cy));
    else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) TransformaSelecao(gira(Escala(1.1f, 1.1f), cx, cy));
    else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) TransformaSelecao(gira(Escala(1 / 1.1f, 1 / 1.1f), cx, cy));
    else if (key == GLFW_KEY_ENTER) applySelecao();
    else if (key == GLFW_KEY_ESCAPE) { // desfaz tudo e devolve a regiao pro lugar
        selMatriz = Identidade();
        applySelecao();
    }
    else return false;
    return true;
}

void clearCanvas() {
    selecionado = false;
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

// ---------------- Interface ----------------
void selecionaFerramenta(enum Tool t) {
    if (selecionado) applySelecao();
    estadoAtual = t;
    if (t == linha) {
        drawFun = DrawLineBresenham;
    }
    else if (t == retanguloVazio) {
        drawFun = DrawRectangleVazio;
    }
    else if (t == retanguloCheio) {
        drawFun = DrawRectangleCheio;
    }
    else if (t == circuloVazio) {
        drawFun = DrawCircleVazio;
    }
    else if (t == circuloCheio) {
        drawFun = DrawCircleCheio;
    }
    else if (t == selecionar) {
        drawFun = DrawRectangleVazio;
    }
    // else NULL;
}

// botao fica branco com bordinha azul
void DrawBotao(Botao b, bool selecionado) {
    if (selecionado) {
        DrawRectangleCheio(b.x1, b.y1, b.x2, b.y2, corBotaoPressionado);
        DrawRectangleVazio(b.x1, b.y1, b.x2, b.y2, corDestaque);
        DrawRectangleVazio(b.x1 + 1, b.y1 + 1, b.x2 - 1, b.y2 - 1, corDestaque);
    } else {
        DrawRectangleCheio(b.x1, b.y1, b.x2, b.y2, corBotao);
    }
}

// quadradinho das cores
void DrawBotaoCor(int i) {
    Botao b = botoesCor[i];
    if (i == indiceCor) {
        DrawRectangleVazio(b.x1 - 2, b.y1 - 2, b.x2 + 2, b.y2 + 2, (Color) BRANCO);
        DrawRectangleVazio(b.x1 - 1, b.y1 - 1, b.x2 + 1, b.y2 + 1, (Color) BRANCO);
    }
    DrawRectangleCheio(b.x1, b.y1, b.x2, b.y2, paleta[i]);
    DrawRectangleVazio(b.x1, b.y1, b.x2, b.y2, (Color) {120, 120, 120}); // contorno cinza
}

void DrawIcone(int id, Botao b) {
    if (icones[id] == NULL) return; // icone nao carregou

    // o botao tem 40 e o icone 32 (sobra 8, entao 4 eh de cada lado)
    int inicioX = b.x1 + 4;
    int inicioY = b.y1 + 4;

    for (int y = 0; y < TAM_ICONE; ++y) {
        for (int x = 0; x < TAM_ICONE; ++x) {
            unsigned char *icone = &icones[id][(y * TAM_ICONE + x) * 4];
            unsigned char *fundo = &uiBuffer[((inicioY + y) * ALTURA + inicioX + x) * 3];
            int a = icone[3];

            Color cor;
            cor.r = (icone[0] * a + fundo[0] * (255 - a)) / 255;
            cor.g = (icone[1] * a + fundo[1] * (255 - a)) / 255;
            cor.b = (icone[2] * a + fundo[2] * (255 - a)) / 255;
            setPixel(inicioX + x, inicioY + y, cor);
        }
    }
}

void DrawUI() {
    enum BrushSize tamanhoSalvo = tamanhoAtual;
    tamanhoAtual = curto;
    desenhandoUI = true;

    // fundo da barra inteira
    DrawRectangleCheio(0, 0, ALTURA - 1, UI_ALTURA - 1, corUIFundo);

    // desenha as linhas separando os tipos de ferramentas
    DrawLineBresenham(362, 8, 362, 51, corBotao); // ferramentas | espessuras
    DrawLineBresenham(506, 8, 506, 51, corBotao); // espessuras | cores
    DrawLineBresenham(628, 8, 628, 51, corBotao); // cores | ações (novo e salvar)

    // ferramentas
    for (int i = 0; i < 8; ++i) {
        bool selecionada = ((int) estadoAtual == i);
        DrawBotao(botoesFerramenta[i], selecionada);
        DrawIcone(i, botoesFerramenta[i]);
    }

    // espessuras
    for (int i = 0; i < 3; ++i) {
        bool selecionada = (tamanhoSalvo == tamanhos[i]);
        DrawBotao(botoesTamanho[i], selecionada);
        DrawIcone(10 + i, botoesTamanho[i]);
    }

    for (int i = 0; i < 8; ++i)
        DrawBotaoCor(i);

    DrawBotao(botaoNovo, false);
    DrawIcone(8, botaoNovo);
    DrawBotao(botaoSalvar, false);
    DrawIcone(9, botaoSalvar);

    desenhandoUI = false;
    tamanhoAtual = tamanhoSalvo;
}

bool dentroBotao(Botao b, int x, int y) {
    return x >= b.x1 && x <= b.x2 && y >= b.y1 && y <= b.y2;
}

void cliqueUI(int x, int y) {
    for (int i = 0; i < 8; ++i)
        if (dentroBotao(botoesFerramenta[i], x, y)) {
            selecionaFerramenta((enum Tool) i);
        }
    for (int i = 0; i < 3; ++i)
        if (dentroBotao(botoesTamanho[i], x, y)) {
            tamanhoAtual = tamanhos[i];
        }
    for (int i = 0; i < 8; ++i)
        if (dentroBotao(botoesCor[i], x, y)) {
            indiceCor = i;
        }
    if (dentroBotao(botaoNovo, x, y)) {
        clearCanvas();
    }
    if (dentroBotao(botaoSalvar, x, y)) {
        SaveImage("Imagem_Salva.png");
    }
}

// atalhos de teclado (os mesmos da barra)
void HandleKeyboard(GLFWwindow *janela, int key, int scancode, int action, int mods) {
    if(selecionado && (action == GLFW_PRESS || action == GLFW_REPEAT) && TeclaSelecao(key)) {
        return;
    }
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_C) {
            clearCanvas();
        } else if(key == GLFW_KEY_B) {
            selecionaFerramenta(borracha);
        } else if(key == GLFW_KEY_P) {
            selecionaFerramenta(pincel);
        } else if(key == GLFW_KEY_L) {
            selecionaFerramenta(linha);
        } else if(key == GLFW_KEY_R) {
            if(mods == GLFW_MOD_SHIFT) {
                selecionaFerramenta(retanguloCheio);
            } else {
                selecionaFerramenta(retanguloVazio);
            }
        } else if(key == GLFW_KEY_O) {
            if (mods == GLFW_MOD_SHIFT) {
                selecionaFerramenta(circuloCheio);
            } else {
                selecionaFerramenta(circuloVazio);
            }
        } else if(key == GLFW_KEY_F) {
            selecionaFerramenta(baldeDeTinta);
        } else if(key == GLFW_KEY_T) {
            selecionaFerramenta(selecionar);
        } else if(key == GLFW_KEY_S) {
            if(mods == GLFW_MOD_CONTROL) {
                SaveImage("teste.png");
            }
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
    y -= UI_ALTURA; // passa pra coordenada do canvas
    if(cliqueNaUI) return; // clicou num botao e arrastou pro canvas, nao pinta
    switch(estadoAtual) {
        case pincel: {
            if(glfwGetMouseButton(janela, 0)) { // "arrastar"
                putPixel((int) xMouse, (int) yMouse, paleta[indiceCor]);
                // interpolação...
                if(x != xMouse || y != yMouse) {
                    double dist = sqrt(pow(x - xMouse, 2) + pow(y - yMouse, 2));
                    int amostras = (int) (dist / tamanhoAtual);

                    // parametrização!! :D
                    for (int i = 0; i <= amostras; ++i) {
                        double t = (double) i / (double) amostras;
                        double vx = x * (1 - t) + xMouse * t;
                        double vy = y * (1 - t) + yMouse * t;
                        putPixel((int) vx, (int) vy, paleta[indiceCor]);
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
    double xJanela, yJanela;
    glfwGetCursorPos(janela, &xJanela, &yJanela);
    double xCanvas = xJanela, yCanvas = yJanela - UI_ALTURA;

    if(action == GLFW_RELEASE) cliqueNaUI = false;
    // clique em cima da barra (se nao tiver no meio de um preview, ai deixa terminar o desenho)
    if(yJanela < UI_ALTURA && !previewingDraw) {
        if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
            cliqueNaUI = true;
            cliqueUI((int) xJanela, (int) yJanela);
        }
        return;
    }

    switch(estadoAtual) {
        case pincel: {
            if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
                putPixel((int) xCanvas, (int) yCanvas, paleta[indiceCor]);
                // como eh so um clique (o mouse nao ta mexendo) nao precisa interpolar...
            }
            break;
        }
        case borracha: {
            if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
                putPixel((int) xCanvas, (int) yCanvas, corBG);
            }
            break;
        } case baldeDeTinta: {
            if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
                FloodFill((int) xCanvas, (int) yCanvas, paleta[indiceCor]);
            }
            break;
        }
        case selecionar: {
            if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
                if(selecionado) {
                    applySelecao();
                }
                startX = xCanvas;
                startY = yCanvas;
                previewingDraw = true;
            } else if(action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT && previewingDraw) {
                previewingDraw = false;
                IniciaSelecao((int) startX, (int) startY, (int) xCanvas, (int) yCanvas);
            }
            break;
        }
        default: { // Eu acabei percebendo que todo código pra essas ferramentas tava ficando igual, ...
            // ...entao decidi usar ponteiro de funcao o.O
            if(action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT && previewingDraw) {
                previewingDraw = false;
                drawFun( (int) startX, (int) startY, (int) xCanvas, (int) yCanvas, paleta[indiceCor]);
            } else if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && !previewingDraw) {
                startX = xCanvas;
                startY = yCanvas;
                previewingDraw = true;
            }
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // deixei o canvas com tamanho fixo

    janela = glfwCreateWindow(ALTURA, LARGURA + UI_ALTURA, "miniPaint!", NULL, NULL);

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

    // mesma coisa pra barra de ferramentas
    for (int i = 0; i < 13; ++i) {
        int w, h;
        icones[i] = stbi_load(icons[i], &w, &h, NULL, 4);
        if (icones[i] == NULL || w != TAM_ICONE || h != TAM_ICONE) {
            printf("Erro: icone %s nao abriu ou nao tem 32x32\n", icons[i]);
            icones[i] = NULL; // o botao fica sem icone nenhum
        }
    }
    GLuint uiTexture;
    glGenTextures(1, &uiTexture);
    glBindTexture(GL_TEXTURE_2D, uiTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ALTURA, UI_ALTURA, 0,
    GL_RGB, GL_UNSIGNED_BYTE, uiBuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // borda da tela
    DrawRectangleVazio(0, 0, 799, 599, (Color) {255, 0, 0}); // TODO: vai tirar isso?

    // eh melhor usar essas funções de callback do que ficar fazendo polling no laço principal
    glfwSetKeyCallback(janela, HandleKeyboard);
    glfwSetCursorPosCallback(janela, mousePosCallback);
    glfwSetMouseButtonCallback(janela, mouseButtonCallback);

    // main loop
    while(!glfwWindowShouldClose(janela)) {

        glfwPollEvents();
        glfwGetCursorPos(janela, &xMouse, &yMouse); // preciso disso pra fazer interpolacao qdo desenhar
        yMouse -= UI_ALTURA; // coordenada do canvas

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
        // uso um outro buffer temporario pra nao ficar um efeito de desenhar tudo
        unsigned char *render = canvas;
        if(previewingDraw) {
            memcpy(tmpBuffer, canvas, ALTURA * LARGURA * 3);
            render = tmpBuffer;
            // ponteiro de funcao, pq tava tudo ficando igual
            Color corPreview = paleta[indiceCor];
            if(estadoAtual == selecionar) {
                corPreview = corDestaque; // a selecao ja aparece azul enquanto arrasta
            }
            drawFun((int) startX, (int) startY, (int) xMouse, (int) yMouse, corPreview);
            render = tmpBuffer;
        }
        if(selecionado) { // mesma ideia: desenha a selecao no buffer temporario ate confirmar
            memcpy(tmpBuffer, canvas, ALTURA * LARGURA * 3);
            previewingDraw = true; // pro setPixel escrever no tmpBuffer
            DrawSelecao();
            DrawBordaSelecao();
            previewingDraw = false;
            render = tmpBuffer;
        }
        // mais coisa da textura, atualizando ela no buffer e enviando pro oepnGL dnv
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, canvasTextures);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ALTURA, LARGURA,
            GL_RGB, GL_UNSIGNED_BYTE, render);
        // render
        // o glViewport diz em que pedaco da janela desenhar, em pixels (o y conta de baixo pra cima)
        // canvas: os 600 px de baixo da janela
        glViewport(0, 0, ALTURA, LARGURA);
        // ai cria um quadrado q cobre esse pedaco inteiro e "cola" a textura do canvas em cima dele
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

        DrawUI();
        glBindTexture(GL_TEXTURE_2D, uiTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ALTURA, UI_ALTURA,
            GL_RGB, GL_UNSIGNED_BYTE, uiBuffer);
        glViewport(0, LARGURA, ALTURA, UI_ALTURA);
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
