#include "police.h"

#include <algorithm>
#include <cmath>

namespace {
float NormalizeAngle(float angle) {
  const float twoPi = 6.2831853f;
  const float pi = 3.1415926f;
  while (angle > pi) {
    angle -= twoPi;
  }
  while (angle < -pi) {
    angle += twoPi;
  }
  return angle;
}
}  // namespace

void UpdatePoliceChase(VehicleState &police, const VehicleState &target, float dt, float elapsedSeconds, float startDelaySeconds, const MovementConfig &config) {
  float clampedDt = std::max(dt, 0.0f);
  if (elapsedSeconds < startDelaySeconds) {
    police.velocity = {0.0f, 0.0f, 0.0f};
    return;
  }

  Vec3 toTarget = target.position - police.position;
  float distance = Length(toTarget);
  if (distance < 0.0001f) {
    return;
  }

  float desiredYaw = std::atan2(toTarget.x, toTarget.z);
  float yawDiff = NormalizeAngle(desiredYaw - police.heading);

  const float turnRate = 2.5f;
  float steer = std::clamp(yawDiff, -1.0f, 1.0f);
  police.heading += steer * turnRate * clampedDt;

  float speedFactor = std::clamp(1.0f - (std::abs(yawDiff) / (3.1415926f * 0.5f)), 0.25f, 1.0f);
  Vec3 forwardDir = {std::sin(police.heading), 0.0f, std::cos(police.heading)};
  float desiredSpeed = std::min(config.maxSpeed * speedFactor, distance * 0.8f);
  float forwardSpeed = Dot(police.velocity, forwardDir);
  float speedError = desiredSpeed - forwardSpeed;
  float accelCmd = std::clamp(speedError, -config.acceleration, config.acceleration);
  forwardSpeed += accelCmd * clampedDt;

  float dragFactor = std::max(0.0f, 1.0f - config.drag * clampedDt);
  forwardSpeed *= dragFactor;
  forwardSpeed = std::clamp(forwardSpeed, -config.maxSpeed, config.maxSpeed);

  police.velocity = forwardDir * forwardSpeed;

  police.position = police.position + police.velocity * clampedDt;
}
