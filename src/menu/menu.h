#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

struct MenuBounds {
  float playMinU = 0.0f;
  float playMaxU = 1.0f;
  float playMinV = 0.0f;
  float playMaxV = 1.0f;

  float tryMinU = 0.0f;
  float tryMaxU = 1.0f;
  float exitMinU = 0.0f;
  float exitMaxU = 1.0f;
  float buttonMinV = 0.0f;
  float buttonMaxV = 1.0f;
};

struct MenuUi {
  GLuint program = 0;
  GLint locTexture = -1;
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint startTexture = 0;
  GLuint loseTexture = 0;
  GLuint winTexture = 0;
  bool wasMouseDownStart = false;
  bool wasMouseDownLose = false;
  bool wasMouseDownWin = false;
  MenuBounds bounds;
};

struct LoseMenuResult {
  bool retry = false;
  bool quit = false;
};

bool InitMenuUi(MenuUi &menu, const MenuBounds &bounds,
                const std::string &startImage, const std::string &loseImage,
                const std::string &winImage);
bool RunStartMenu(MenuUi &menu, GLFWwindow *window);
LoseMenuResult ShowLoseMenu(MenuUi &menu, GLFWwindow *window, int width,
                            int height);
bool ShowWinScreen(MenuUi &menu, GLFWwindow *window, int width, int height);
void CleanupMenuUi(MenuUi &menu);
