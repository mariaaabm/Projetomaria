#pragma once

#include <GL/glew.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "math.h"

struct Vertex {
  // Posição do vértice no espaço 3D
  Vec3 position;
  // Normal do vértice para iluminação
  Vec3 normal;
  // Coordenada de textura
  Vec2 texCoord;
};

struct Material {
  // Nome do material no ficheiro MTL
  std::string name;
  // Cor difusa base
  Vec3 kd = {0.6f, 0.6f, 0.6f};
  // Caminho da textura (map_Kd)
  std::string mapKd;
  // ID da textura no OpenGL
  GLuint textureId = 0;
  // Indica se tem textura válida
  bool hasTexture = false;
};

struct Mesh {
  // Material usado por este mesh
  std::string materialName;
  // Lista de vértices
  std::vector<Vertex> vertices;
  // VAO e VBO para desenhar
  GLuint vao = 0;
  GLuint vbo = 0;
};

struct Model {
  // Lista de meshes do modelo
  std::vector<Mesh> meshes;
  // Materiais encontrados no MTL
  std::unordered_map<std::string, Material> materials;
  // Centro do modelo original
  Vec3 center = {0.0f, 0.0f, 0.0f};
  // Escala aplicada para normalizar o tamanho
  float scale = 1.0f;
  // Y mínimo após escalar (útil para alinhar ao chão)
  float minY = 0.0f;
};

// Carrega um ficheiro OBJ e preenche a estrutura Model
bool LoadObj(const std::string &path, Model &model);
// Cria buffers OpenGL para um mesh
void SetupMesh(Mesh &mesh);
// Carrega uma textura 2D a partir de um ficheiro
GLuint LoadTexture2D(const std::string &path);
// Carrega e associa texturas aos materiais
void SetupTextures(Model &model);
// Liberta recursos do modelo (VAO/VBO/texturas)
void CleanupModel(Model &model);
