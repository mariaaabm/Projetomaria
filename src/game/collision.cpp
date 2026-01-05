#include "collision.h"

#include <cmath>

bool CheckCaught(const VehicleState &police, const VehicleState &player, float catchDistance) {
  Vec3 delta = player.position - police.position;
  float distSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
  return distSquared <= catchDistance * catchDistance;
}
