#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "assets/model.h"
#include "game/collision.h"
#include "game/game_state.h"
#include "game/police.h"
#include "game/road.h"
#include "gl_utils.h"
#include "math.h"
#include "menu/menu.h"

static float LerpFloat(float a, float b, float t) { return a + (b - a) * t; }

static Vec3 LerpVec3(const Vec3 &a, const Vec3 &b, float t) {
  float clamped = std::clamp(t, 0.0f, 1.0f);
  return {LerpFloat(a.x, b.x, clamped), LerpFloat(a.y, b.y, clamped),
          LerpFloat(a.z, b.z, clamped)};
}

static float SmoothStep01(float t) {
  float clamped = std::clamp(t, 0.0f, 1.0f);
  return clamped * clamped * (3.0f - 2.0f * clamped);
}

static void CursorPosCallback(GLFWwindow *, double, double) {}

static void MouseButtonCallback(GLFWwindow *, int, int, int) {}

static void ScrollCallback(GLFWwindow *, double, double) {}

static void KeyCallback(GLFWwindow *, int, int, int, int) {}

int main() {
  if (!glfwInit()) {
    std::cerr << "Falha ao iniciar GLFW.\n";
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "Pista", nullptr, nullptr);
  if (!window) {
    std::cerr << "Falha ao criar janela.\n";
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSetCursorPosCallback(window, CursorPosCallback);
  glfwSetMouseButtonCallback(window, MouseButtonCallback);
  glfwSetScrollCallback(window, ScrollCallback);
  glfwSetKeyCallback(window, KeyCallback);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Falha ao iniciar GLEW.\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.18f, 0.19f, 0.21f, 1.0f);

  MenuBounds menuBounds;
  menuBounds.playMinU = 0.55f; // area do botão na textura inicial
  menuBounds.playMaxU = 0.93f;
  menuBounds.playMinV = 0.13f;
  menuBounds.playMaxV = 0.29f;

  menuBounds.tryMinU = 0.18f;
  menuBounds.tryMaxU = 0.50f;
  menuBounds.exitMinU = 0.52f;
  menuBounds.exitMaxU = 0.82f;
  menuBounds.buttonMinV = 0.12f;
  menuBounds.buttonMaxV = 0.25f;
  // Hotspots do menu de vitoria (ajustar se a arte mudar)
  menuBounds.winMenuMinU = 0.18f;
  menuBounds.winMenuMaxU = 0.50f;
  menuBounds.winExitMinU = 0.51f;
  menuBounds.winExitMaxU = 0.82f;
  menuBounds.winButtonMinV = 0.12f;
  menuBounds.winButtonMaxV = 0.25f;
  MenuUi menuUi;
  if (!InitMenuUi(menuUi, menuBounds, "src/menu/images/menu_inicial.png",
                  "src/menu/images/menu_perder.png",
                  "src/menu/images/menu_ganhar.png")) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  Model trackModel;
  if (!LoadObj("assets/textures/pista/pista.obj", trackModel)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  Model carModel;
  if (!LoadObj("assets/textures/carro/MrBeanCarFinal.obj", carModel)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  Model policeCarModel;
  if (!LoadObj("assets/textures/carro_policia/Ford Crown Victoria Police "
               "Interceptor.obj",
               policeCarModel)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  SetupTextures(trackModel);
  SetupTextures(carModel);
  SetupTextures(policeCarModel);

  for (auto &mesh : trackModel.meshes) {
    SetupMesh(mesh);
  }
  for (auto &mesh : carModel.meshes) {
    SetupMesh(mesh);
  }
  for (auto &mesh : policeCarModel.meshes) {
    SetupMesh(mesh);
  }

  GLuint trackProgram =
      CreateProgram("shaders/track_vertex.vs", "shaders/track_fragment.fs");
  GLuint carProgram =
      CreateProgram("shaders/car_vertex.vs", "shaders/car_fragment.fs");
  if (!trackProgram || !carProgram) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  GLint trackLocModel = glGetUniformLocation(trackProgram, "uModel");
  GLint trackLocView = glGetUniformLocation(trackProgram, "uView");
  GLint trackLocProj = glGetUniformLocation(trackProgram, "uProj");
  GLint trackLocColor = glGetUniformLocation(trackProgram, "uColor");
  GLint trackLocLight = glGetUniformLocation(trackProgram, "uLightDir");
  GLint trackLocLight2 = glGetUniformLocation(trackProgram, "uLightDir2");
  GLint trackLocAmbient = glGetUniformLocation(trackProgram, "uAmbient");
  GLint trackLocTexture = glGetUniformLocation(trackProgram, "uTexture");
  GLint trackLocUseTexture = glGetUniformLocation(trackProgram, "uUseTexture");

  GLint carLocModel = glGetUniformLocation(carProgram, "uModel");
  GLint carLocView = glGetUniformLocation(carProgram, "uView");
  GLint carLocProj = glGetUniformLocation(carProgram, "uProj");
  GLint carLocColor = glGetUniformLocation(carProgram, "uColor");
  GLint carLocLight = glGetUniformLocation(carProgram, "uLightDir");
  GLint carLocLight2 = glGetUniformLocation(carProgram, "uLightDir2");
  GLint carLocAmbient = glGetUniformLocation(carProgram, "uAmbient");
  GLint carLocTexture = glGetUniformLocation(carProgram, "uTexture");
  GLint carLocUseTexture = glGetUniformLocation(carProgram, "uUseTexture");

  const float worldScale = 40.0f;
  const float trackHalfExtent = worldScale * 0.5f - 0.5f;
  const float carScale = worldScale * 0.005f;
  const float policeCarScale = worldScale * 0.0075f;
  const float carBaseRotation = 3.1415926f / 2.0f;
  const float policeBaseRotation = 3.1415926f * 2.0f;
  float carLift = 0.02f * carScale;
  float policeLift = 0.02f * policeCarScale;

  GameState gameState;
  MovementConfig movementConfig;
  movementConfig.maxSpeed = 2.8f;      // Slower player
  movementConfig.acceleration = 2.0f;  // Menor sensibilidade no W/S
  movementConfig.turnRate = 1.0f;      // Menor sensibilidade no A/D
  MovementConfig policeMovementConfig; // Default maxSpeed 5.0f
  gameState.player.position = {0.0f, 0.0f, -6.0f};
  gameState.player.heading = 0.0f;
  gameState.police.position = {0.0f, 0.0f, -8.0f};
  gameState.police.heading = 0.0f;
  ExtractRoadPoints(trackModel, worldScale, gameState.roadPoints,
                    gameState.roadTriangles);

  bool gameOver = false;
  bool playerWon = false;
  const float catchDistance = 0.3f;
  const float policeStartDelay = 1.0f;
  const float winTime = 10.0f;

  float startTime = 0.0f;
  float lastFrameTime = 0.0f;

  auto resetGame = [&](float currentTime) {
    gameOver = false;
    playerWon = false;
    gameState.player.position = {0.0f, 0.0f, -6.0f};
    gameState.player.heading = 0.0f;
    gameState.player.velocity = {0.0f, 0.0f, 0.0f};
    gameState.player.speed = 0.0f;
    gameState.police.position = {0.0f, 0.0f, -8.0f};
    gameState.police.heading = 0.0f;
    gameState.police.velocity = {0.0f, 0.0f, 0.0f};
    gameState.police.speed = 0.0f;
    gameState.playerTrail.clear();
    startTime = currentTime;
    lastFrameTime = currentTime;
  };

  auto renderFrame = [&](float currentTime) {
    float deltaTime = currentTime - lastFrameTime;
    float elapsedTime = currentTime - startTime;
    lastFrameTime = currentTime;

    if (!gameOver) {
      Vec3 prevPlayerPos = gameState.player.position;
      Vec3 prevPolicePos = gameState.police.position;

      InputState input = ReadPlayerInput(window);
      UpdatePlayer(gameState.player, input, deltaTime, movementConfig,
                   carBaseRotation, trackHalfExtent, gameState.playerTrail);
      UpdatePoliceChase(gameState.police, gameState.player, deltaTime,
                        elapsedTime, policeStartDelay, policeMovementConfig,
                        trackHalfExtent, gameState.playerTrail);

      if (!InsideRoad(
              {gameState.player.position.x, gameState.player.position.z},
              gameState.roadTriangles)) {
        gameState.player.position = prevPlayerPos;
        gameState.player.velocity = {0.0f, 0.0f, 0.0f};
        gameState.player.speed = 0.0f;
      }
      if (!InsideRoad(
              {gameState.police.position.x, gameState.police.position.z},
              gameState.roadTriangles)) {
        gameState.police.position = prevPolicePos;
      }

      if (CheckCaught(gameState.police, gameState.player, catchDistance)) {
        std::cout << "Game over, Mr. Bean got caught\n";
        gameOver = true;
        gameState.player.velocity = {0.0f, 0.0f, 0.0f};
        gameState.police.velocity = {0.0f, 0.0f, 0.0f};
      } else if (elapsedTime >= winTime) {
        std::cout << "Venceu! Sobreviveu por 10 segundos.\n";
        gameOver = true;
        playerWon = true;
        gameState.player.velocity = {0.0f, 0.0f, 0.0f};
        gameState.police.velocity = {0.0f, 0.0f, 0.0f};
      }
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
    Mat4 proj =
        Mat4Perspective(45.0f * 3.1415926f / 180.0f, aspect, 0.1f, 100.0f);

    Vec3 carPos = {gameState.player.position.x,
                   -carModel.minY * carScale + carLift + 0.05f,
                   gameState.player.position.z};
    Vec3 policePos = {gameState.police.position.x,
                      -policeCarModel.minY * policeCarScale + policeLift +
                          0.05f,
                      gameState.police.position.z};
    float backYaw = gameState.player.heading + carBaseRotation + 3.1415926f;
    Vec3 backDir = {std::cos(backYaw), 0.0f, std::sin(backYaw)};
    const float followDist = 0.95f; // Extra-close chase view
    const float followHeightBase = 0.6f;
    const float targetHeightBase = 0.45f;
    const float followHeightLow = 0.1f; // Drop even closer to road
    const float targetHeightLow = 0.08f;
    const float introOverviewDuration = 1.5f;
    const float introBlendDuration = 2.0f;
    float lowerBlend = 0.0f;
    if (elapsedTime > introOverviewDuration + introBlendDuration) {
      float t = (elapsedTime - (introOverviewDuration + introBlendDuration)) /
                1.2f; // drop height after zoom finishes
      lowerBlend = SmoothStep01(t);
    }
    float followHeight =
        LerpFloat(followHeightBase, followHeightLow, lowerBlend);
    float targetHeight =
        LerpFloat(targetHeightBase, targetHeightLow, lowerBlend);

    Vec3 chaseEye =
        carPos + backDir * followDist + Vec3{0.0f, followHeight, 0.0f};
    Vec3 chaseTarget = carPos + Vec3{0.0f, targetHeight, 0.0f};
    Vec3 pairCenter = (carPos + policePos) * 0.5f;
    Vec3 overviewEye = pairCenter + Vec3{0.0f, 8.0f, 0.0f};
    Vec3 overviewTarget = pairCenter + Vec3{0.0f, 0.5f, 0.0f};
    float camBlend = 1.0f;
    if (elapsedTime < introOverviewDuration) {
      camBlend = 0.0f;
    } else if (elapsedTime < introOverviewDuration + introBlendDuration) {
      float t = (elapsedTime - introOverviewDuration) / introBlendDuration;
      camBlend = SmoothStep01(t);
    }
    Vec3 eye = LerpVec3(overviewEye, chaseEye, camBlend);
    Vec3 target = LerpVec3(overviewTarget, chaseTarget, camBlend);
    float groundY = -trackModel.minY * worldScale;
    eye.y = std::max(eye.y, groundY + 0.45f);
    target.y = std::max(target.y, groundY + 0.22f);
    Mat4 view = Mat4LookAt(eye, target, {0.0f, 1.0f, 0.0f});
    Mat4 trackMat =
        Mat4Multiply(Mat4Translate({0.0f, -trackModel.minY * worldScale, 0.0f}),
                     Mat4Scale(worldScale));
    Mat4 carMat = Mat4Multiply(
        Mat4Translate(carPos),
        Mat4Multiply(Mat4RotateY(gameState.player.heading + carBaseRotation),
                     Mat4Scale(carScale)));
    Mat4 policeCarMat = Mat4Multiply(
        Mat4Translate(policePos),
        Mat4Multiply(Mat4RotateY(gameState.police.heading + policeBaseRotation),
                     Mat4Scale(policeCarScale)));

    glUseProgram(trackProgram);
    glUniformMatrix4fv(trackLocModel, 1, GL_FALSE, trackMat.m);
    glUniformMatrix4fv(trackLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(trackLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(trackLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(trackLocLight2, 0.25f, -0.35f, 0.3f);
    glUniform3f(trackLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform1i(trackLocTexture, 0);

    for (const auto &mesh : trackModel.meshes) {
      const Material *material = nullptr;
      auto it = trackModel.materials.find(mesh.materialName);
      if (it != trackModel.materials.end()) {
        material = &it->second;
      }
      Vec3 color = material ? material->kd : Vec3{0.6f, 0.6f, 0.6f};
      glUniform3f(trackLocColor, color.x, color.y, color.z);
      if (material && material->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textureId);
        glUniform1i(trackLocUseTexture, 1);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(trackLocUseTexture, 0);
      }
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0,
                   static_cast<GLsizei>(mesh.vertices.size()));
    }

    glUseProgram(carProgram);
    glUniformMatrix4fv(carLocModel, 1, GL_FALSE, carMat.m);
    glUniformMatrix4fv(carLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(carLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(carLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(carLocLight2, 0.25f, -0.35f, 0.3f);
    glUniform3f(carLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform1i(carLocTexture, 0);

    for (const auto &mesh : carModel.meshes) {
      const Material *material = nullptr;
      auto it = carModel.materials.find(mesh.materialName);
      if (it != carModel.materials.end()) {
        material = &it->second;
      }
      Vec3 color = material ? material->kd : Vec3{0.6f, 0.6f, 0.6f};
      glUniform3f(carLocColor, color.x, color.y, color.z);
      if (material && material->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textureId);
        glUniform1i(carLocUseTexture, 1);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(carLocUseTexture, 0);
      }
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0,
                   static_cast<GLsizei>(mesh.vertices.size()));
    }

    glUseProgram(carProgram);
    glUniformMatrix4fv(carLocModel, 1, GL_FALSE, policeCarMat.m);
    glUniformMatrix4fv(carLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(carLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(carLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(carLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform1i(carLocTexture, 0);

    for (const auto &mesh : policeCarModel.meshes) {
      const Material *material = nullptr;
      auto it = policeCarModel.materials.find(mesh.materialName);
      if (it != policeCarModel.materials.end()) {
        material = &it->second;
      }
      Vec3 color = material ? material->kd : Vec3{0.6f, 0.6f, 0.6f};
      glUniform3f(carLocColor, color.x, color.y, color.z);
      if (material && material->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textureId);
        glUniform1i(carLocUseTexture, 1);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(carLocUseTexture, 0);
      }
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0,
                   static_cast<GLsizei>(mesh.vertices.size()));
    }

    if (gameOver && !playerWon) {
      LoseMenuResult loseResult = ShowLoseMenu(menuUi, window, width, height);
      if (loseResult.retry) {
        float now = static_cast<float>(glfwGetTime());
        resetGame(now);
      } else if (loseResult.quit) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
    } else if (gameOver && playerWon) {
      WinMenuResult winResult = ShowWinScreen(menuUi, window, width, height);
      if (winResult.goToMenu) {
        // sinalizar retorno ao menu principal
        gameOver = false;
        playerWon = false;
        resetGame(static_cast<float>(glfwGetTime()));
        glfwSetTime(0.0);
        startTime = static_cast<float>(glfwGetTime());
        lastFrameTime = startTime;
        bool started = RunStartMenu(menuUi, window);
        if (!started) {
          glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else {
          startTime = static_cast<float>(glfwGetTime());
          lastFrameTime = startTime;
        }
      } else if (winResult.quit) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  };

  bool inMenu = (menuUi.startTexture != 0);
  if (inMenu) {
    bool started = RunStartMenu(menuUi, window);
    if (started) {
      glfwSetTime(0.0);
      startTime = static_cast<float>(glfwGetTime());
      lastFrameTime = startTime;
      renderFrame(startTime); // desenha o primeiro frame antes de sair
      inMenu = false;
    } else {
      CleanupMenuUi(menuUi);
      glfwDestroyWindow(window);
      glfwTerminate();
      return 0;
    }
  }

  if (lastFrameTime == 0.0f) {
    glfwSetTime(0.0);
    startTime = static_cast<float>(glfwGetTime());
    lastFrameTime = startTime;
  }
  // Render imediatamente após sair do menu para evitar frame vazio
  renderFrame(startTime);
  while (!glfwWindowShouldClose(window)) {
    float currentTime = static_cast<float>(glfwGetTime());
    renderFrame(currentTime);
  }

  glDeleteProgram(trackProgram);
  glDeleteProgram(carProgram);
  CleanupModel(trackModel);
  CleanupModel(carModel);
  CleanupModel(policeCarModel);
  CleanupMenuUi(menuUi);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
