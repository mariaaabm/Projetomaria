#include "road.h"

#include <cmath>
#include <unordered_set>
#include <vector>

namespace {
// Gera uma chave inteira para um ponto 2D (com quantização)
long long HashPoint(const Vec2 &p, float quantize) {
  int ix = static_cast<int>(std::round(p.x * quantize));
  int iy = static_cast<int>(std::round(p.y * quantize));
  return (static_cast<long long>(ix) << 32) ^
         static_cast<unsigned long long>(iy);
}

// Testa se um ponto está dentro de um triângulo 2D
bool PointInTriangle(const Vec2 &p, const Triangle2 &t) {
  auto sign = [](const Vec2 &p1, const Vec2 &p2, const Vec2 &p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
  };
  const float eps = 1e-4f;
  float d1 = sign(p, t.a, t.b);
  float d2 = sign(p, t.b, t.c);
  float d3 = sign(p, t.c, t.a);
  bool hasNeg = (d1 < -eps) || (d2 < -eps) || (d3 < -eps);
  bool hasPos = (d1 > eps) || (d2 > eps) || (d3 > eps);
  return !(hasNeg && hasPos);
}
}

void ExtractRoadPoints(const Model &model, float worldScale,
                       std::vector<Vec2> &outPoints,
                       std::vector<Triangle2> &outTriangles) {
  // Materiais que representam a estrada
  static const std::unordered_set<std::string> kRoadMaterials = {
      "Material.007", "Material.008"};
  std::unordered_set<long long> seen;
  const float quantize = 1000.0f; // ~1mm em unidades de mundo

  for (const auto &mesh : model.meshes) {
    if (kRoadMaterials.find(mesh.materialName) == kRoadMaterials.end()) {
      continue;
    }
    const auto &verts = mesh.vertices;
    for (size_t i = 0; i + 2 < verts.size(); i += 3) {
      // Converte para 2D e guarda o triângulo
      Vec3 p0 = verts[i].position * worldScale;
      Vec3 p1 = verts[i + 1].position * worldScale;
      Vec3 p2 = verts[i + 2].position * worldScale;
      Vec2 pts[3] = {{p0.x, p0.z}, {p1.x, p1.z}, {p2.x, p2.z}};
      outTriangles.push_back({pts[0], pts[1], pts[2]});
      for (const auto &pt : pts) {
        // Evita pontos duplicados
        long long key = HashPoint(pt, quantize);
        if (seen.insert(key).second) {
          outPoints.push_back(pt);
        }
      }
    }
  }
}

bool InsideRoad(const Vec2 &p, const std::vector<Triangle2> &tris) {
  // Se não há triângulos, aceita tudo
  if (tris.empty()) {
    return true;
  }
  // Procura um triângulo que contenha o ponto
  for (const auto &tri : tris) {
    if (PointInTriangle(p, tri)) {
      return true;
    }
  }
  return false;
}
