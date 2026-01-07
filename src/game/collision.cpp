#include "collision.h"

#include <cmath>

bool CheckCaught(const VehicleState &police, const VehicleState &player, float catchDistance) {
  // Distância entre polícia e jogador
  Vec3 delta = player.position - police.position;
  float distSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
  // Compara com o raio de captura
  return distSquared <= catchDistance * catchDistance;
}
