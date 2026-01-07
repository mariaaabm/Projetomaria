#include "game_state.h"

#include <algorithm>
#include <cmath>

InputState ReadPlayerInput(GLFWwindow *window) {
  InputState input;
  // Mapeia WASD e espaço
  input.forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
  input.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
  input.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
  input.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
  input.brake = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
  return input;
}

void UpdatePlayer(VehicleState &player, const InputState &input, float dt,
                  const MovementConfig &config, float headingOffset,
                  float trackHalfExtent, std::vector<Vec3> &trail) {
  // Guarda pontos do rasto
  if (trail.empty()) {
    trail.push_back(player.position);
  } else {
    Vec3 last = trail.back();
    float distSq = (player.position.x - last.x) * (player.position.x - last.x) +
                   (player.position.z - last.z) * (player.position.z - last.z);
    if (distSq > 2.0f * 2.0f) { // A cada 2 metros
      trail.push_back(player.position);
    }
  }

  // Evita dt negativo
  float clampedDt = std::max(dt, 0.0f);

  // Direção em mundo
  float worldHeading = player.heading + headingOffset;
  Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};

  // Aceleração base
  float targetAccel = 0.0f;
  if (input.forward) {
    targetAccel += config.acceleration;
  }
  if (input.backward) {
    targetAccel -= config.acceleration;
  }
  player.speed += targetAccel * clampedDt;

  // Travagem
  if (input.brake) {
    if (player.speed > 0.0f) {
      player.speed = std::max(0.0f, player.speed - config.braking * clampedDt);
    } else if (player.speed < 0.0f) {
      player.speed = std::min(0.0f, player.speed + config.braking * clampedDt);
    }
  }

  // Arrasto
  if (player.speed > 0.0f) {
    player.speed = std::max(0.0f, player.speed - config.drag * clampedDt);
  } else if (player.speed < 0.0f) {
    player.speed = std::min(0.0f, player.speed + config.drag * clampedDt);
  }

  float maxReverse = -config.maxSpeed * 0.5f;
  player.speed = std::clamp(player.speed, maxReverse, config.maxSpeed);

  // Direção 
  float steerInput = 0.0f;
  if (input.left) {
    steerInput -= 1.0f;
  }
  if (input.right) {
    steerInput += 1.0f;
  }
  if (steerInput != 0.0f) {
    float speedAbs = std::abs(player.speed);
    if (speedAbs > 0.05f) {
      float speedFactor = std::clamp(speedAbs / config.maxSpeed, 0.0f, 1.0f);
      float directionSign = (player.speed >= 0.0f) ? 1.0f : -1.0f;
      player.heading += steerInput * config.turnRate * speedFactor * clampedDt *
                        directionSign;
    }
    worldHeading = player.heading + headingOffset;
    forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};
  }

  // Atualiza posição
  player.velocity = forwardDir * player.speed;
  player.position = player.position + player.velocity * clampedDt;
  // Mantém o jogador dentro dos limites
  player.position.x =
      std::clamp(player.position.x, -trackHalfExtent, trackHalfExtent);
  player.position.z =
      std::clamp(player.position.z, -trackHalfExtent, trackHalfExtent);
}
