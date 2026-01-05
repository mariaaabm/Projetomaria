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

void UpdatePoliceChase(VehicleState &police, const VehicleState &target, float dt, float elapsedSeconds, float startDelaySeconds, const MovementConfig &config, float trackHalfExtent) {
  float clampedDt = std::max(dt, 0.0f);
  if (elapsedSeconds < startDelaySeconds) {
    police.velocity = {0.0f, 0.0f, 0.0f};
    police.speed = 0.0f;
    return;
  }

  Vec3 toTarget = target.position - police.position;
  float distance = Length(toTarget);
  if (distance < 0.0001f) {
    return;
  }

  const float headingOffset = 3.1415926f / 2.0f;
  float worldHeading = police.heading + headingOffset;

  float desiredYaw = std::atan2(toTarget.z, toTarget.x);
  float yawDiff = NormalizeAngle(desiredYaw - worldHeading);

  float steerInput = std::clamp(yawDiff, -1.0f, 1.0f);
  if (steerInput != 0.0f) {
    float speedFactor = std::clamp(std::abs(police.speed) / config.maxSpeed, 0.0f, 1.0f);
    police.heading += steerInput * config.turnRate * speedFactor * clampedDt;
    worldHeading = police.heading + headingOffset;
  }

  Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};

  float desiredSpeed = std::max(config.maxSpeed * 0.3f, std::min(config.maxSpeed, distance * 0.8f));
  float speedError = desiredSpeed - police.speed;
  float accelCmd = std::clamp(speedError, -config.acceleration, config.acceleration);
  police.speed += accelCmd * clampedDt;

  if (police.speed > 0.0f) {
    police.speed = std::max(0.0f, police.speed - config.drag * clampedDt);
  } else if (police.speed < 0.0f) {
    police.speed = std::min(0.0f, police.speed + config.drag * clampedDt);
  }

  police.speed = std::clamp(police.speed, -config.maxSpeed * 0.2f, config.maxSpeed);

  police.velocity = forwardDir * police.speed;
  police.position = police.position + police.velocity * clampedDt;
  police.position.x = std::clamp(police.position.x, -trackHalfExtent, trackHalfExtent);
  police.position.z = std::clamp(police.position.z, -trackHalfExtent, trackHalfExtent);
}
