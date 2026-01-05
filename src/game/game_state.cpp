#include "game_state.h"

#include <algorithm>
#include <cmath>

InputState ReadPlayerInput(GLFWwindow *window) {
  InputState input;
  input.forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
  input.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
  input.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
  input.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
  input.brake = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
  return input;
}

void UpdatePlayer(VehicleState &player, const InputState &input, float dt, const MovementConfig &config, float headingOffset) {
  float clampedDt = std::max(dt, 0.0f);

  float worldHeading = player.heading + headingOffset;
  Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};

  float targetAccel = 0.0f;
  if (input.forward) {
    targetAccel += config.acceleration;
  }
  if (input.backward) {
    targetAccel -= config.acceleration;
  }
  player.speed += targetAccel * clampedDt;

  if (input.brake) {
    if (player.speed > 0.0f) {
      player.speed = std::max(0.0f, player.speed - config.braking * clampedDt);
    } else if (player.speed < 0.0f) {
      player.speed = std::min(0.0f, player.speed + config.braking * clampedDt);
    }
  }

  if (player.speed > 0.0f) {
    player.speed = std::max(0.0f, player.speed - config.drag * clampedDt);
  } else if (player.speed < 0.0f) {
    player.speed = std::min(0.0f, player.speed + config.drag * clampedDt);
  }

  float maxReverse = -config.maxSpeed * 0.5f;
  player.speed = std::clamp(player.speed, maxReverse, config.maxSpeed);

  float steerInput = 0.0f;
  if (input.left) {
    steerInput -= 1.0f;
  }
  if (input.right) {
    steerInput += 1.0f;
  }
  if (steerInput != 0.0f) {
    float speedFactor = std::clamp(std::abs(player.speed) / config.maxSpeed, 0.0f, 1.0f);
    float directionSign = (player.speed >= 0.0f) ? 1.0f : -1.0f;
    player.heading += steerInput * config.turnRate * speedFactor * clampedDt * directionSign;
    worldHeading = player.heading + headingOffset;
    forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};
  }

  player.velocity = forwardDir * player.speed;
  player.position = player.position + player.velocity * clampedDt;
}
