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
} // namespace

void UpdatePoliceChase(VehicleState &police, const VehicleState &target,
                       float dt, float elapsedSeconds, float startDelaySeconds,
                       const MovementConfig &config, float trackHalfExtent,
                       const std::vector<Vec3> &trail) {
  float clampedDt = std::max(dt, 0.0f);

  // Stuck Detection State
  static float stuckTimer = 0.0f;
  static float reverseTimer = 0.0f;

  // Unstuck Mode (Reversing)
  if (reverseTimer > 0.0f) {
    reverseTimer -= clampedDt;

    // Reverse Action
    police.speed = std::max(police.speed - config.braking * clampedDt,
                            -config.maxSpeed * 0.5f);

    // Counter-steer while reversing to back out of corner
    police.heading -= config.turnRate * clampedDt * 0.5f;

    // Apply physics
    const float headingOffset = 3.1415926f / 2.0f;
    float worldHeading = police.heading + headingOffset;
    Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};
    police.velocity = forwardDir * police.speed;
    police.position = police.position + police.velocity * clampedDt;
    police.position.x =
        std::clamp(police.position.x, -trackHalfExtent, trackHalfExtent);
    police.position.z =
        std::clamp(police.position.z, -trackHalfExtent, trackHalfExtent);
    return;
  }

  if (elapsedSeconds < startDelaySeconds) {
    police.velocity = {0.0f, 0.0f, 0.0f};
    police.speed = 0.0f;
    return;
  }

  Vec3 targetPos = target.position;

  bool chasingTrail = false;

  // Trail following logic
  if (!trail.empty()) {
    float minDistSq = 1e9f;
    size_t closestIdx = 0;

    // Find closest trail point to police
    for (size_t i = 0; i < trail.size(); ++i) {
      Vec3 d = trail[i] - police.position;
      float dSq = d.x * d.x + d.z * d.z; // Ignore Y for distance
      if (dSq < minDistSq) {
        minDistSq = dSq;
        closestIdx = i;
      }
    }

    // Target a point ahead in the trail
    size_t lookAhead = 1;
    size_t targetIdx = closestIdx + lookAhead;
    if (targetIdx < trail.size()) {
      targetPos = trail[targetIdx];
      chasingTrail = true;
    }
  }

  Vec3 toTarget = targetPos - police.position;
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
    float speedFactor =
        std::clamp(std::abs(police.speed) / config.maxSpeed, 0.0f, 1.0f);
    police.heading += steerInput * config.turnRate * speedFactor * clampedDt;
    worldHeading = police.heading + headingOffset;
  }

  Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};

  float desiredSpeed = config.maxSpeed;

  // Only slow down if we are catching the player directly (end of trail)
  if (!chasingTrail) {
    desiredSpeed = std::max(config.maxSpeed * 0.3f,
                            std::min(config.maxSpeed, distance * 0.8f));
  }

  float speedError = desiredSpeed - police.speed;
  float accelCmd =
      std::clamp(speedError, -config.acceleration, config.acceleration);
  police.speed += accelCmd * clampedDt;

  if (police.speed > 0.0f) {
    police.speed = std::max(0.0f, police.speed - config.drag * clampedDt);
  } else if (police.speed < 0.0f) {
    police.speed = std::min(0.0f, police.speed + config.drag * clampedDt);
  }

  police.speed =
      std::clamp(police.speed, -config.maxSpeed * 0.2f, config.maxSpeed);

  police.velocity = forwardDir * police.speed;
  police.position = police.position + police.velocity * clampedDt;
  police.position.x =
      std::clamp(police.position.x, -trackHalfExtent, trackHalfExtent);
  police.position.z =
      std::clamp(police.position.z, -trackHalfExtent, trackHalfExtent);

  // Update Stuck Timer
  // Consider stuck if speed is very low AND we are not "done" (distance > 1.0)
  if (std::abs(police.speed) < 1.0f && distance > 1.5f &&
      elapsedSeconds > startDelaySeconds) {
    stuckTimer += clampedDt;
  } else {
    stuckTimer = 0.0f;
  }

  // Trigger Force Reverse if stuck for too long
  if (stuckTimer > 1.5f) {
    reverseTimer = 1.5f; // Reverse for 1.5 seconds
    stuckTimer = 0.0f;
  }
}
