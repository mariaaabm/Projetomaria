#pragma once

#include <GLFW/glfw3.h>
#include <vector>

#include "math.h"

struct InputState {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool brake = false;
};

struct MovementConfig {
  float acceleration = 12.0f;
  float braking = 30.0f;
  float drag = 1.5f;
  float maxSpeed = 18.0f;
  float turnRate = 2.8f;
};

struct VehicleState {
  Vec3 position = {0.0f, 0.0f, 0.0f};
  Vec3 velocity = {0.0f, 0.0f, 0.0f};
  float heading = 0.0f;
  float speed = 0.0f;
};

struct Triangle2 {
  Vec2 a;
  Vec2 b;
  Vec2 c;
};

struct GameState {
  VehicleState player;
  VehicleState police;
  std::vector<Vec2> roadPoints;
  std::vector<Triangle2> roadTriangles;
};

InputState ReadPlayerInput(GLFWwindow *window);
void UpdatePlayer(VehicleState &player, const InputState &input, float dt, const MovementConfig &config, float headingOffset, float trackHalfExtent);
