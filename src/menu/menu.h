#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

struct MenuBounds {
  // Area clicavel do botao "Play" no menu inicial (UV)
  float playMinU = 0.0f;
  float playMaxU = 1.0f;
  float playMinV = 0.0f;
  float playMaxV = 1.0f;

  // Area clicavel do botao "Try Again" no menu de derrota
  float tryMinU = 0.0f;
  float tryMaxU = 1.0f;
  // Area clicavel do botao "Exit" no menu de derrota
  float exitMinU = 0.0f;
  float exitMaxU = 1.0f;
  float buttonMinV = 0.0f;
  float buttonMaxV = 1.0f;

  // Areas clicaveis no menu de vitoria
  float winMenuMinU = 0.0f;
  float winMenuMaxU = 1.0f;
  float winExitMinU = 0.0f;
  float winExitMaxU = 1.0f;
  float winButtonMinV = 0.0f;
  float winButtonMaxV = 1.0f;
};

struct MenuUi {
  // Programa e recursos de desenho do menu
  GLuint program = 0;
  GLint locTexture = -1;
  GLuint vao = 0;
  GLuint vbo = 0;
  // Texturas dos menus
  GLuint startTexture = 0;
  GLuint loseTexture = 0;
  GLuint winTexture = 0;
  // Debounce de clique por menu
  bool wasMouseDownStart = false;
  bool wasMouseDownLose = false;
  bool wasMouseDownWin = false;
  MenuBounds bounds;
};

struct LoseMenuResult {
  // Acao escolhida no menu de derrota
  bool retry = false;
  bool quit = false;
};

struct WinMenuResult {
  // Acao escolhida no menu de vitoria
  bool goToMenu = false;
  bool quit = false;
};

// Inicializa texturas e quad do menu
bool InitMenuUi(MenuUi &menu, const MenuBounds &bounds,
                const std::string &startImage, const std::string &loseImage,
                const std::string &winImage);
// Executa o menu inicial ate clicar em jogar
bool RunStartMenu(MenuUi &menu, GLFWwindow *window);
// Desenha o menu de derrota e devolve acao
LoseMenuResult ShowLoseMenu(MenuUi &menu, GLFWwindow *window, int width,
                            int height);
// Desenha o menu de vitoria e devolve acao
WinMenuResult ShowWinScreen(MenuUi &menu, GLFWwindow *window, int width,
                            int height);
// Liberta recursos do menu
void CleanupMenuUi(MenuUi &menu);
