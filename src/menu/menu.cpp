#include "menu/menu.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "assets/model.h"
#include "gl_utils.h"

bool InitMenuUi(MenuUi &menu, const MenuBounds &bounds,
                const std::string &startImage, const std::string &loseImage,
                const std::string &winImage) {
  // Guarda bounds (retangulos clicaveis) e cria shader do menu
  menu.bounds = bounds;
  menu.program =
      CreateProgram("shaders/menu_vertex.vs", "shaders/menu_fragment.fs");
  if (!menu.program) {
    return false;
  }

  //vai buscar o uniform da textura
  menu.locTexture = glGetUniformLocation(menu.program, "uTexture");

  // Quad fullscreen com UVs
  float quad[] = {
      -1.0f, -1.0f, 0.0f, 0.0f, 
      1.0f,  -1.0f, 1.0f, 0.0f, 
      -1.0f, 1.0f,  0.0f, 1.0f, 
      1.0f,  1.0f,  1.0f, 1.0f  
  };
  // Cria VAO e VBO do quad e define atributos
  glGenVertexArrays(1, &menu.vao);
  glGenBuffers(1, &menu.vbo);
  glBindVertexArray(menu.vao);
  glBindBuffer(GL_ARRAY_BUFFER, menu.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)(sizeof(float) * 2));
  glBindVertexArray(0);

  // Carrega texturas dos menus
  menu.startTexture = LoadTexture2D(startImage);
  menu.loseTexture = LoadTexture2D(loseImage);
  menu.winTexture = LoadTexture2D(winImage);
  return menu.startTexture != 0;
}

bool RunStartMenu(MenuUi &menu, GLFWwindow *window) {
  // Loop do menu inicial ate clicar em jogar
  if (!menu.startTexture) {
    return false;
  }

  glDisable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {

    int fbWidth = 0;
    int fbHeight = 0;
    //ajusta o viewport(area de desenho na janela) ao tamanho da janela
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    // Desenha menu
    glUseProgram(menu.program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, menu.startTexture);
    glUniform1i(menu.locTexture, 0);
    glBindVertexArray(menu.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Evento e input do rato
    glfwSwapBuffers(window);
    glfwPollEvents();

    // Trata clique na area do botao jogar
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    int winW = 0;
    int winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    
    //verifica se o botao esquerdo do rato esta pressionado
    bool mouseDown =  glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    //verifica se o rato esta dentro da area do botao jogar
    if (mouseDown && !menu.wasMouseDownStart && winW > 0 && winH > 0) {
      float u = static_cast<float>(mouseX) / static_cast<float>(winW);
      float v = 1.0f - static_cast<float>(mouseY) / static_cast<float>(winH);
      if (u >= menu.bounds.playMinU && u <= menu.bounds.playMaxU &&
          v >= menu.bounds.playMinV && v <= menu.bounds.playMaxV) {
        menu.wasMouseDownStart = mouseDown;
        glEnable(GL_DEPTH_TEST);
        return true;
      }
    }
    menu.wasMouseDownStart = mouseDown;
  }

  glEnable(GL_DEPTH_TEST);
  return false;
}

LoseMenuResult ShowLoseMenu(MenuUi &menu, GLFWwindow *window, int width,
                            int height) {
  // Desenha menu de derrota e valida clique
  LoseMenuResult result;
  if (!menu.loseTexture) {
    return result;
  }

  // Desenha menu de derrota
  glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glUseProgram(menu.program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, menu.loseTexture);
  glUniform1i(menu.locTexture, 0);
  glBindVertexArray(menu.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glEnable(GL_DEPTH_TEST);

  // Trata clique nas areas configuradas
  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  int winW = 0;
  int winH = 0;
  glfwGetWindowSize(window, &winW, &winH);
  bool mouseDown =
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  if (mouseDown && !menu.wasMouseDownLose && winW > 0 && winH > 0) {
    float u = static_cast<float>(mouseX) / static_cast<float>(winW);
    float v = 1.0f - static_cast<float>(mouseY) / static_cast<float>(winH);
    if (u >= menu.bounds.tryMinU && u <= menu.bounds.tryMaxU &&
        v >= menu.bounds.buttonMinV && v <= menu.bounds.buttonMaxV) {
      result.retry = true;
    } else if (u >= menu.bounds.exitMinU && u <= menu.bounds.exitMaxU &&
               v >= menu.bounds.buttonMinV && v <= menu.bounds.buttonMaxV) {
      result.quit = true;
    }
  }
  menu.wasMouseDownLose = mouseDown;
  return result;
}

WinMenuResult ShowWinScreen(MenuUi &menu, GLFWwindow *window, int width,
                            int height) {
  // Desenha menu de vitoria e valida clique
  WinMenuResult result;
  if (!menu.winTexture) {
    return result;
  }

  // Desenha menu de vitoria
  glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glUseProgram(menu.program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, menu.winTexture);
  glUniform1i(menu.locTexture, 0);
  glBindVertexArray(menu.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Trata clique nas areas configuradas
  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  int winW = 0;
  int winH = 0;
  glfwGetWindowSize(window, &winW, &winH);
  bool mouseDown =
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  if (mouseDown && !menu.wasMouseDownWin && winW > 0 && winH > 0) {
    float u = static_cast<float>(mouseX) / static_cast<float>(winW);
    float v = 1.0f - static_cast<float>(mouseY) / static_cast<float>(winH);
    if (u >= menu.bounds.winMenuMinU && u <= menu.bounds.winMenuMaxU &&
        v >= menu.bounds.winButtonMinV && v <= menu.bounds.winButtonMaxV) {
      result.goToMenu = true;
    } else if (u >= menu.bounds.winExitMinU && u <= menu.bounds.winExitMaxU &&
               v >= menu.bounds.winButtonMinV &&
               v <= menu.bounds.winButtonMaxV) {
      result.quit = true;
    }
  }
  menu.wasMouseDownWin = mouseDown;
  return result;
}

void CleanupMenuUi(MenuUi &menu) {
  // Liberta recursos OpenGL do menu
  if (menu.startTexture) {
    glDeleteTextures(1, &menu.startTexture);
    menu.startTexture = 0;
  }
  if (menu.loseTexture) {
    glDeleteTextures(1, &menu.loseTexture);
    menu.loseTexture = 0;
  }
  if (menu.winTexture) {
    glDeleteTextures(1, &menu.winTexture);
    menu.winTexture = 0;
  }
  if (menu.vbo) {
    glDeleteBuffers(1, &menu.vbo);
    menu.vbo = 0;
  }
  if (menu.vao) {
    glDeleteVertexArrays(1, &menu.vao);
    menu.vao = 0;
  }
  if (menu.program) {
    glDeleteProgram(menu.program);
    menu.program = 0;
  }
}
