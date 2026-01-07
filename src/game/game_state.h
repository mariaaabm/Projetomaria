#pragma once

#include <GLFW/glfw3.h>
#include <vector>

#include "math.h"

struct InputState {
  // Acelerar para a frente
  bool forward = false;
  // Acelerar para trás
  bool backward = false;
  // Virar à esquerda
  bool left = false;
  // Virar à direita
  bool right = false;
  // Travar
  bool brake = false;
};

struct MovementConfig {
  // Aceleração base
  float acceleration = 4.0f;
  // Força de travagem
  float braking = 30.0f;
  // Arrasto para reduzir a velocidade
  float drag = 1.5f;
  // Velocidade máxima
  float maxSpeed = 5.0f;
  // Velocidade de rotação
  float turnRate = 2.5f;
};

struct VehicleState {
  // Posição no mundo
  Vec3 position = {0.0f, 0.0f, 0.0f};
  // Velocidade no mundo
  Vec3 velocity = {0.0f, 0.0f, 0.0f};
  // Direção em radianos
  float heading = 0.0f;
  // Velocidade escalar
  float speed = 0.0f;
};

struct Triangle2 {
  // Vértices 2D
  Vec2 a;
  Vec2 b;
  Vec2 c;
};

struct GameState {
  // Estado do jogador
  VehicleState player;
  // Estado da polícia
  VehicleState police;
  // Pontos do eixo da estrada
  std::vector<Vec2> roadPoints;
  // Triângulos para teste de dentro/fora
  std::vector<Triangle2> roadTriangles;
  // Rasto do jogador para a IA seguir
  std::vector<Vec3> playerTrail;
};

// Lê teclas e devolve o estado do input
InputState ReadPlayerInput(GLFWwindow *window);
// Atualiza o jogador com base no input e na física
void UpdatePlayer(VehicleState &player, const InputState &input, float dt,
                  const MovementConfig &config, float headingOffset,
                  float trackHalfExtent, std::vector<Vec3> &trail);
