#pragma once

#include "game_state.h"

void UpdatePoliceChase(VehicleState &police, const VehicleState &target, float dt, float elapsedSeconds, float startDelaySeconds, const MovementConfig &config);
