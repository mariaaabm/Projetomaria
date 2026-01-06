#pragma once

#include <vector>

#include "assets/model.h"
#include "game_state.h"

void ExtractRoadPoints(const Model &model, float worldScale,
                       std::vector<Vec2> &outPoints,
                       std::vector<Triangle2> &outTriangles);
bool InsideRoad(const Vec2 &p, const std::vector<Triangle2> &tris);
