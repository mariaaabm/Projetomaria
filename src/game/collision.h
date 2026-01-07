#pragma once

#include "game_state.h"

// Verifica se a polícia apanha o jogador
bool CheckCaught(const VehicleState &police, const VehicleState &player, float catchDistance);
