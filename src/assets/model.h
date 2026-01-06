#pragma once

#include <GL/glew.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "math.h"

struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec2 texCoord;
};

struct Material {
  std::string name;
  Vec3 kd = {0.6f, 0.6f, 0.6f};
  std::string mapKd;
  GLuint textureId = 0;
  bool hasTexture = false;
};

struct Mesh {
  std::string materialName;
  std::vector<Vertex> vertices;
  GLuint vao = 0;
  GLuint vbo = 0;
};

struct Model {
  std::vector<Mesh> meshes;
  std::unordered_map<std::string, Material> materials;
  Vec3 center = {0.0f, 0.0f, 0.0f};
  float scale = 1.0f;
  float minY = 0.0f;
};

bool LoadObj(const std::string &path, Model &model);
void SetupMesh(Mesh &mesh);
GLuint LoadTexture2D(const std::string &path);
void SetupTextures(Model &model);
void CleanupModel(Model &model);
