#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include "assets/model.h"
#include "audio.h"
#include "game/collision.h"
#include "game/game_state.h"
#include "game/police.h"
#include "game/road.h"
#include "gl_utils.h"
#include "math.h"
#include "menu/menu.h"

// função para fazer uma transição linear suave entre dois valores float- usada em altura, distancias e velocidades
static float LerpFloat(float a, float b, float t) { return a + (b - a) * t; }

//função para fazer uma transição suave entre duas posições 3D- usada na movimentação da câmera
static Vec3 LerpVec3(const Vec3 &a, const Vec3 &b, float t) {
  // Interpolacao com clamp
  float clamped = std::clamp(t, 0.0f, 1.0f);
  return {LerpFloat(a.x, b.x, clamped), LerpFloat(a.y, b.y, clamped),
          LerpFloat(a.z, b.z, clamped)};
}

//função de suavização - usada para transições de câmera e efeitos visuais
static float SmoothStep01(float t) {
  // Suavizacao [0,1]
  float clamped = std::clamp(t, 0.0f, 1.0f);
  return clamped * clamped * (3.0f - 2.0f * clamped);
}

int main() {
  // Inicializa GLFW e contexto OpenGL
  if (!glfwInit()) {
    std::cerr << "Falha ao iniciar GLFW.\n";
    return 1;
  }

  //define a versão do OpenGL 
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  //cria a janela e verifica se erro
  GLFWwindow *window = glfwCreateWindow(1280, 720, "Pista", nullptr, nullptr);
  if (!window) {
    std::cerr << "Falha ao criar janela.\n";
    glfwTerminate();
    return 1;
  }

  //define o contexto da janela criada
  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Falha ao iniciar GLEW.\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.18f, 0.19f, 0.21f, 1.0f); //define cor de fundo 

  // Audio em loop
  const char *musicPath = "src/music/Mr Bean Music.mp3";
  if (!InitAudioEngine(musicPath)) {
    std::cerr << "Falha ao iniciar áudio em loop: " << musicPath << "\n";
  }

  // Hotspots do menu (UV)
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
  
  // Inicializa UI do menu
  if (!InitMenuUi(menuUi, menuBounds, "src/menu/images/menu_inicial.png",
                  "src/menu/images/menu_perder.png",
                  "src/menu/images/menu_ganhar.png")) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  // Carrega modelos 3D
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

  // Prepara texturas e buffers de cada modelo
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

  // HUD simples (barra de tempo)
  GLuint hudTexture = 0;
  glGenTextures(1, &hudTexture);
  glBindTexture(GL_TEXTURE_2D, hudTexture);
  unsigned int whitePixel = 0xffffffff;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               &whitePixel);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  GLuint hudVao = 0;
  GLuint hudVbo = 0;
  glGenVertexArrays(1, &hudVao);
  glGenBuffers(1, &hudVbo);
  glBindVertexArray(hudVao);
  glBindBuffer(GL_ARRAY_BUFFER, hudVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)(sizeof(float) * 2));
  glBindVertexArray(0);

  // Compila shaders
  GLuint trackProgram =
      CreateProgram("shaders/track_vertex.vs", "shaders/track_fragment.fs");
  GLuint carProgram =
      CreateProgram("shaders/car_vertex.vs", "shaders/car_fragment.fs");
  if (!trackProgram || !carProgram) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  // Locacoes de uniforms
  GLint trackLocModel = glGetUniformLocation(trackProgram, "uModel");
  GLint trackLocView = glGetUniformLocation(trackProgram, "uView");
  GLint trackLocProj = glGetUniformLocation(trackProgram, "uProj");
  GLint trackLocColor = glGetUniformLocation(trackProgram, "uColor");
  GLint trackLocLight = glGetUniformLocation(trackProgram, "uLightDir");
  GLint trackLocLight2 = glGetUniformLocation(trackProgram, "uLightDir2");
  GLint trackLocAmbient = glGetUniformLocation(trackProgram, "uAmbient");
  GLint trackLocViewPos = glGetUniformLocation(trackProgram, "uViewPos");
  GLint trackLocTexture = glGetUniformLocation(trackProgram, "uTexture");
  GLint trackLocUseTexture = glGetUniformLocation(trackProgram, "uUseTexture");

  GLint carLocModel = glGetUniformLocation(carProgram, "uModel");
  GLint carLocView = glGetUniformLocation(carProgram, "uView");
  GLint carLocProj = glGetUniformLocation(carProgram, "uProj");
  GLint carLocColor = glGetUniformLocation(carProgram, "uColor");
  GLint carLocLight = glGetUniformLocation(carProgram, "uLightDir");
  GLint carLocLight2 = glGetUniformLocation(carProgram, "uLightDir2");
  GLint carLocAmbient = glGetUniformLocation(carProgram, "uAmbient");
  GLint carLocViewPos = glGetUniformLocation(carProgram, "uViewPos");
  GLint carLocTexture = glGetUniformLocation(carProgram, "uTexture");
  GLint carLocUseTexture = glGetUniformLocation(carProgram, "uUseTexture");

  // Escalas do mundo e veiculos
  const float worldScale = 40.0f;
  const float trackHalfExtent = worldScale * 0.5f - 0.5f;
  const float carScale = worldScale * 0.005f;
  const float policeCarScale = worldScale * 0.0075f;
  const float carBaseRotation = 3.1415926f / 2.0f;
  float carLift = 0.02f * carScale;
  float policeLift = 0.02f * policeCarScale;

  // Estado inicial e configuracao do movimento
  GameState gameState;
  MovementConfig movementConfig;
  movementConfig.maxSpeed = 3.3f;      // Slower player
  movementConfig.acceleration = 2.0f;  // Menor sensibilidade no W/S
  movementConfig.turnRate = 1.3f;      // Um pouco mais sensível no A/D
  MovementConfig policeMovementConfig;
  policeMovementConfig.maxSpeed = 4.5f;
  policeMovementConfig.acceleration = 5.0f; // Mais aceleração que o jogador
  policeMovementConfig.drag = 1.5f;
  policeMovementConfig.braking = 30.0f;
  policeMovementConfig.turnRate = 2.2f; // Melhor manobrabilidade
  gameState.player.position = {0.0f, 0.0f, -6.0f};
  gameState.player.heading = 0.0f;
  gameState.police.position = {0.0f, 0.0f, -8.0f};
  gameState.police.heading = 0.0f;
  ExtractRoadPoints(trackModel, worldScale, gameState.roadPoints,
                    gameState.roadTriangles);

  bool gameOver = false;
  bool playerWon = false;
  const float catchDistance = 0.3f;
  const float policeStartDelay = 0.0f; // Começa imediatamente
  const float winTime = 30.0f;

  float startTime = 0.0f;
  float lastFrameTime = 0.0f;

  auto resetGame = [&](float currentTime) {
    // Reinicia estado do jogo
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
    ResetPoliceChaseState();
    startTime = currentTime;
    lastFrameTime = currentTime;
  };

  auto renderFrame = [&](float currentTime) {
    // Atualiza tempo e HUD do titulo
    float deltaTime = currentTime - lastFrameTime;
    float elapsedTime = currentTime - startTime;
    lastFrameTime = currentTime;
    float remaining = std::max(0.0f, winTime - elapsedTime);
    {
      char title[128];
      std::snprintf(title, sizeof(title), "Pista - Tempo restante: %.1fs",
                    remaining);
      glfwSetWindowTitle(window, title);
    }

    if (!gameOver) {
      // Atualiza fisica e IA
      Vec3 prevPlayerPos = gameState.player.position;
      Vec3 prevPolicePos = gameState.police.position;

      InputState input = ReadPlayerInput(window);
      UpdatePlayer(gameState.player, input, deltaTime, movementConfig,
                   carBaseRotation, trackHalfExtent, gameState.playerTrail);
      UpdatePoliceChase(gameState.police, gameState.player, deltaTime,
                        elapsedTime, policeStartDelay, policeMovementConfig,
                        trackHalfExtent, gameState.playerTrail);

      auto trySlideOnRoad = [&](Vec3 prevPos, Vec3 &pos, VehicleState &state) {
        // Resolve colisao com a borda da estrada
        Vec2 pos2 = {pos.x, pos.z};
        if (InsideRoad(pos2, gameState.roadTriangles)) {
          return;
        }

        Vec3 moveDir = pos - prevPos;
        float moveLen = Length(moveDir);

        // Passo 1: recuar na direção oposta ao movimento para tentar voltar à estrada.
        if (moveLen > 0.0001f) {
          Vec3 push = moveDir / moveLen * std::min(1.0f, moveLen * 0.9f);
          Vec3 backedPos = pos - push;
          if (InsideRoad({backedPos.x, backedPos.z},
                         gameState.roadTriangles)) {
            pos = backedPos;
            float sign = (state.speed >= 0.0f) ? 1.0f : -1.0f;
            float mag = std::max(std::abs(state.speed) * 0.65f, 0.8f);
            state.speed = sign * mag;
            state.velocity = {state.velocity.x * 0.65f, 0.0f,
                              state.velocity.z * 0.65f};
            return;
          }
        }

        // Tentar movimentar apenas num eixo para permitir "raspar" na parede.
        Vec3 slideX = {pos.x, prevPos.y, prevPos.z};
        Vec3 slideZ = {prevPos.x, prevPos.y, pos.z};
        bool fixed = false;
        if (InsideRoad({slideX.x, slideX.z}, gameState.roadTriangles)) {
          pos = slideX;
          fixed = true;
        } else if (InsideRoad({slideZ.x, slideZ.z}, gameState.roadTriangles)) {
          pos = slideZ;
          fixed = true;
        }

        if (fixed) {
          // Mantém velocidade mas abafa um pouco para simular fricção na parede.
          float sign = (state.speed >= 0.0f) ? 1.0f : -1.0f;
          float mag = std::max(std::abs(state.speed) * 0.8f, 0.6f);
          state.speed = sign * mag;
          state.velocity = {state.velocity.x * 0.8f, 0.0f,
                            state.velocity.z * 0.8f};
        } else {
          pos = prevPos;
          // Impulso curto para trás seguindo a direção do movimento/heading.
          Vec3 dir = moveDir;
          if (Length(dir) < 0.0001f) {
            dir = state.velocity;
          }
          if (Length(dir) < 0.0001f) {
            dir = {-std::cos(state.heading), 0.0f, -std::sin(state.heading)};
          }
          dir = Normalize(dir) * -1.0f;
          float pushBack = 1.5f;
          state.speed = -pushBack;
          state.velocity = dir * pushBack;
        }
      };

      trySlideOnRoad(prevPlayerPos, gameState.player.position,
                     gameState.player);
      trySlideOnRoad(prevPolicePos, gameState.police.position,
                     gameState.police);

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

    // Prepara frame
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
    Mat4 proj =
        Mat4Perspective(45.0f * 3.1415926f / 180.0f, aspect, 0.1f, 100.0f);

    // Posicoes e camera
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
        Mat4Multiply(Mat4RotateY(gameState.police.heading + carBaseRotation),
                     Mat4Scale(policeCarScale)));

    // Desenha pista
    glUseProgram(trackProgram);
    glUniformMatrix4fv(trackLocModel, 1, GL_FALSE, trackMat.m);
    glUniformMatrix4fv(trackLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(trackLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(trackLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(trackLocLight2, 0.25f, -0.35f, 0.3f);
    glUniform3f(trackLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform3f(trackLocViewPos, eye.x, eye.y, eye.z);
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

    // Desenha carro do jogador
    glUseProgram(carProgram);
    glUniformMatrix4fv(carLocModel, 1, GL_FALSE, carMat.m);
    glUniformMatrix4fv(carLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(carLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(carLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(carLocLight2, 0.25f, -0.35f, 0.3f);
    glUniform3f(carLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform3f(carLocViewPos, eye.x, eye.y, eye.z);
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

    // Desenha carro da policia
    glUniformMatrix4fv(carLocModel, 1, GL_FALSE, policeCarMat.m);
    glUniformMatrix4fv(carLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(carLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(carLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(carLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform3f(carLocViewPos, eye.x, eye.y, eye.z);
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

    // Menus de fim de jogo
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

    // HUD: barra de contagem decrescente no canto superior esquerdo
    if (!gameOver) {
      float tNorm = (winTime > 0.0f) ? (remaining / winTime) : 0.0f;
      tNorm = std::clamp(tNorm, 0.0f, 1.0f);
      float padding = 0.02f;
      float barWidth = 0.35f;
      float barHeight = 0.04f;
      float x0 = -1.0f + padding * 2.0f;
      float y0 = 1.0f - padding;
      float x1 = x0 + barWidth * tNorm * 2.0f; // NDC width scaled
      float y1 = y0 - barHeight * 2.0f;
      float verts[] = {
          x0, y0, 0.0f, 1.0f, //
          x1, y0, 1.0f, 1.0f, //
          x0, y1, 0.0f, 0.0f, //
          x1, y1, 1.0f, 0.0f  //
      };
      glDisable(GL_DEPTH_TEST);
      glUseProgram(menuUi.program);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, hudTexture);
      glUniform1i(menuUi.locTexture, 0);
      glBindVertexArray(hudVao);
      glBindBuffer(GL_ARRAY_BUFFER, hudVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glEnable(GL_DEPTH_TEST);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  };

  if (menuUi.startTexture != 0) {
    // Mostra menu inicial antes de iniciar o jogo
    bool started = RunStartMenu(menuUi, window);
    if (started) {
      glfwSetTime(0.0);
      startTime = static_cast<float>(glfwGetTime());
      lastFrameTime = startTime;
    } else {
      CleanupMenuUi(menuUi);
      ShutdownAudioEngine();
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
  // Loop principal
  while (!glfwWindowShouldClose(window)) {
    float currentTime = static_cast<float>(glfwGetTime());
    renderFrame(currentTime);
  }

  // Limpeza de recursos
  glDeleteProgram(trackProgram);
  glDeleteProgram(carProgram);
  CleanupModel(trackModel);
  CleanupModel(carModel);
  CleanupModel(policeCarModel);
  CleanupMenuUi(menuUi);
  if (hudTexture) {
    glDeleteTextures(1, &hudTexture);
  }
  if (hudVbo) {
    glDeleteBuffers(1, &hudVbo);
  }
  if (hudVao) {
    glDeleteVertexArrays(1, &hudVao);
  }

  ShutdownAudioEngine();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
