#pragma once

#include <vector>

#include "assets/model.h"
#include "game_state.h"

// Extrai pontos e triângulos da estrada a partir do modelo
void ExtractRoadPoints(const Model &model, float worldScale,
                       std::vector<Vec2> &outPoints,
                       std::vector<Triangle2> &outTriangles);
// Testa se um ponto está dentro da estrada
bool InsideRoad(const Vec2 &p, const std::vector<Triangle2> &tris);
