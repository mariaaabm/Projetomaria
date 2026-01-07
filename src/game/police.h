#pragma once

#include "game_state.h"

// Atualiza a IA da polícia para perseguir o jogador
void UpdatePoliceChase(VehicleState &police, const VehicleState &target,
                       float dt, float elapsedSeconds, float startDelaySeconds,
                       const MovementConfig &config, float trackHalfExtent,
                       const std::vector<Vec3> &trail);

// Reinicia o estado interno da perseguição
void ResetPoliceChaseState();
