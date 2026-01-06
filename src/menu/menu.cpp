#include "menu/menu.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "assets/model.h"
#include "gl_utils.h"

bool InitMenuUi(MenuUi &menu, const MenuBounds &bounds,
                const std::string &startImage, const std::string &loseImage,
                const std::string &winImage) {
  menu.bounds = bounds;
  menu.program =
      CreateProgram("shaders/menu_vertex.vs", "shaders/menu_fragment.fs");
  if (!menu.program) {
    return false;
  }

  menu.locTexture = glGetUniformLocation(menu.program, "uTexture");

  float quad[] = {
      -1.0f, -1.0f, 0.0f, 0.0f, //
      1.0f,  -1.0f, 1.0f, 0.0f, //
      -1.0f, 1.0f,  0.0f, 1.0f, //
      1.0f,  1.0f,  1.0f, 1.0f  //
  };
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

  menu.startTexture = LoadTexture2D(startImage);
  menu.loseTexture = LoadTexture2D(loseImage);
  menu.winTexture = LoadTexture2D(winImage);
  return menu.startTexture != 0;
}

bool RunStartMenu(MenuUi &menu, GLFWwindow *window) {
  if (!menu.startTexture) {
    return false;
  }

  glDisable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(menu.program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, menu.startTexture);
    glUniform1i(menu.locTexture, 0);
    glBindVertexArray(menu.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glfwSwapBuffers(window);
    glfwPollEvents();

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    int winW = 0;
    int winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    bool mouseDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
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
  LoseMenuResult result;
  if (!menu.loseTexture) {
    return result;
  }

  glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glUseProgram(menu.program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, menu.loseTexture);
  glUniform1i(menu.locTexture, 0);
  glBindVertexArray(menu.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glEnable(GL_DEPTH_TEST);

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

bool ShowWinScreen(MenuUi &menu, GLFWwindow *window, int width, int height) {
  if (!menu.winTexture) {
    return false;
  }

  glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glUseProgram(menu.program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, menu.winTexture);
  glUniform1i(menu.locTexture, 0);
  glBindVertexArray(menu.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glEnable(GL_DEPTH_TEST);

  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  int winW = 0;
  int winH = 0;
  glfwGetWindowSize(window, &winW, &winH);
  bool mouseDown =
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  bool closeRequested = false;
  if (mouseDown && !menu.wasMouseDownWin && winW > 0 && winH > 0) {
    closeRequested = true; // qualquer clique fecha na vitória
  }
  menu.wasMouseDownWin = mouseDown;
  return closeRequested;
}

void CleanupMenuUi(MenuUi &menu) {
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
