#include "assets/model.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gl_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
std::string Trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return {};
  }
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

std::vector<std::string> SplitWhitespace(const std::string &s) {
  std::istringstream iss(s);
  std::vector<std::string> parts;
  std::string part;
  while (iss >> part) {
    parts.push_back(part);
  }
  return parts;
}

int ResolveIndex(int idx, int size) {
  if (idx > 0) {
    return idx - 1;
  }
  return size + idx;
}

struct ObjIndex {
  int v = 0;
  int vt = 0;
  int vn = 0;
};

ObjIndex ParseFaceToken(const std::string &token) {
  ObjIndex out;
  size_t first = token.find('/');
  if (first == std::string::npos) {
    out.v = std::stoi(token);
    return out;
  }

  size_t second = token.find('/', first + 1);
  std::string vStr = token.substr(0, first);
  std::string vtStr;
  std::string vnStr;

  if (second == std::string::npos) {
    vtStr = token.substr(first + 1);
  } else {
    vtStr = token.substr(first + 1, second - first - 1);
    vnStr = token.substr(second + 1);
  }

  if (!vStr.empty()) {
    out.v = std::stoi(vStr);
  }
  if (!vtStr.empty()) {
    out.vt = std::stoi(vtStr);
  }
  if (!vnStr.empty()) {
    out.vn = std::stoi(vnStr);
  }

  return out;
}

std::string Dirname(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos) {
    return "";
  }
  return path.substr(0, pos + 1);
}

bool LoadMtl(const std::string &path,
             std::unordered_map<std::string, Material> &materials) {
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Nao foi possivel abrir MTL: " << path << "\n";
    return false;
  }

  std::string baseDir = Dirname(path);
  Material *current = nullptr;
  std::string line;
  while (std::getline(file, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    if (line.rfind("newmtl ", 0) == 0) {
      Material mat;
      mat.name = Trim(line.substr(7));
      materials[mat.name] = mat;
      current = &materials[mat.name];
    } else if (line.rfind("Kd ", 0) == 0 && current) {
      auto parts = SplitWhitespace(line);
      if (parts.size() >= 4) {
        current->kd = {std::stof(parts[1]), std::stof(parts[2]),
                       std::stof(parts[3])};
      }
    } else if (line.rfind("map_Kd ", 0) == 0 && current) {
      std::string mapName = Trim(line.substr(7));
      if (!mapName.empty()) {
        current->mapKd = baseDir + mapName;
      }
    }
  }

  return true;
}
} // namespace

bool LoadObj(const std::string &path, Model &model) {
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Nao foi possivel abrir OBJ: " << path << "\n";
    return false;
  }

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> texcoords;
  std::unordered_map<std::string, Material> materials;
  std::unordered_map<std::string, Mesh> meshBuilders;
  std::string currentMaterial = "default";
  meshBuilders[currentMaterial] = Mesh{};
  meshBuilders[currentMaterial].materialName = currentMaterial;
  std::string currentObject;
  bool skipCurrentObject = false;

  std::string baseDir = Dirname(path);

  std::string line;
  while (std::getline(file, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    auto parts = SplitWhitespace(line);
    if (parts.empty()) {
      continue;
    }

    if (parts[0] == "o" && parts.size() >= 2) {
      currentObject = parts[1];
      skipCurrentObject = (currentObject == "Cube");
    } else if (parts[0] == "mtllib") {
      size_t mtllibPos = line.find("mtllib");
      std::string mtlName = Trim(line.substr(mtllibPos + 6));
      std::string mtlPath = baseDir + mtlName;
      if (!LoadMtl(mtlPath, materials)) {
        std::string fallback = baseDir + "pista.mtl";
        LoadMtl(fallback, materials);
      }
    } else if (parts[0] == "v" && parts.size() >= 4) {
      positions.push_back(
          {std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])});
    } else if (parts[0] == "vt" && parts.size() >= 3) {
      texcoords.push_back({std::stof(parts[1]), std::stof(parts[2])});
    } else if (parts[0] == "vn" && parts.size() >= 4) {
      normals.push_back(
          {std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])});
    } else if (parts[0] == "usemtl" && parts.size() >= 2) {
      currentMaterial = parts[1];
      if (meshBuilders.find(currentMaterial) == meshBuilders.end()) {
        Mesh mesh;
        mesh.materialName = currentMaterial;
        meshBuilders[currentMaterial] = mesh;
      }
    } else if (parts[0] == "f" && parts.size() >= 4) {
      if (skipCurrentObject) {
        continue;
      }
      std::vector<ObjIndex> indices;
      indices.reserve(parts.size() - 1);
      for (size_t i = 1; i < parts.size(); ++i) {
        indices.push_back(ParseFaceToken(parts[i]));
      }

      for (size_t i = 1; i + 1 < indices.size(); ++i) {
        ObjIndex i0 = indices[0];
        ObjIndex i1 = indices[i];
        ObjIndex i2 = indices[i + 1];

        Vec3 p0 =
            positions[ResolveIndex(i0.v, static_cast<int>(positions.size()))];
        Vec3 p1 =
            positions[ResolveIndex(i1.v, static_cast<int>(positions.size()))];
        Vec3 p2 =
            positions[ResolveIndex(i2.v, static_cast<int>(positions.size()))];

        bool hasNormals =
            (i0.vn != 0 && i1.vn != 0 && i2.vn != 0 && !normals.empty());
        Vec3 faceNormal = Normalize(Cross(p1 - p0, p2 - p0));

        auto addVertex = [&](const ObjIndex &idx, const Vec3 &fallbackNormal,
                             const Vec3 &pos) {
          Vec3 normal = fallbackNormal;
          if (hasNormals) {
            int normalIndex =
                ResolveIndex(idx.vn, static_cast<int>(normals.size()));
            if (normalIndex >= 0 &&
                normalIndex < static_cast<int>(normals.size())) {
              normal = normals[normalIndex];
            }
          }

          Vec2 texCoord = {0.0f, 0.0f};
          if (idx.vt != 0 && !texcoords.empty()) {
            int texIndex =
                ResolveIndex(idx.vt, static_cast<int>(texcoords.size()));
            if (texIndex >= 0 &&
                texIndex < static_cast<int>(texcoords.size())) {
              texCoord = texcoords[texIndex];
            }
          }

          meshBuilders[currentMaterial].vertices.push_back(
              {pos, normal, texCoord});
        };

        addVertex(i0, faceNormal, p0);
        addVertex(i1, faceNormal, p1);
        addVertex(i2, faceNormal, p2);
      }
    }
  }

  bool hasBounds = false;
  Vec3 minPos;
  Vec3 maxPos;

  for (auto &entry : meshBuilders) {
    Mesh &mesh = entry.second;
    if (mesh.vertices.empty()) {
      continue;
    }

    for (const auto &vertex : mesh.vertices) {
      if (!hasBounds) {
        minPos = vertex.position;
        maxPos = vertex.position;
        hasBounds = true;
      } else {
        minPos.x = std::min(minPos.x, vertex.position.x);
        minPos.y = std::min(minPos.y, vertex.position.y);
        minPos.z = std::min(minPos.z, vertex.position.z);
        maxPos.x = std::max(maxPos.x, vertex.position.x);
        maxPos.y = std::max(maxPos.y, vertex.position.y);
        maxPos.z = std::max(maxPos.z, vertex.position.z);
      }
    }
  }

  if (!hasBounds) {
    std::cerr << "OBJ sem vertices validos.\n";
    return false;
  }

  model.center = (minPos + maxPos) * 0.5f;
  Vec3 size = maxPos - minPos;
  float maxDim = std::max({size.x, size.y, size.z});
  model.scale = (maxDim > 0.0f) ? (1.0f / maxDim) : 1.0f;
  Vec3 scaledMin = (minPos - model.center) * model.scale;
  model.minY = scaledMin.y;

  model.meshes.reserve(meshBuilders.size());
  for (auto &entry : meshBuilders) {
    Mesh mesh = entry.second;
    if (mesh.vertices.empty()) {
      continue;
    }

    for (auto &vertex : mesh.vertices) {
      vertex.position = (vertex.position - model.center) * model.scale;
    }

    model.meshes.push_back(std::move(mesh));
  }

  model.materials = std::move(materials);
  return true;
}

void SetupMesh(Mesh &mesh) {
  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex),
               mesh.vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(offsetof(Vertex, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(offsetof(Vertex, texCoord)));

  glBindVertexArray(0);
}

GLuint LoadTexture2D(const std::string &path) {
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc *data =
      stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!data) {
    std::cerr << "Falha ao carregar textura: " << path << "\n";
    return 0;
  }

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);
  return texture;
}

void SetupTextures(Model &model) {
  for (auto &entry : model.materials) {
    Material &material = entry.second;
    if (!material.mapKd.empty()) {
      material.textureId = LoadTexture2D(material.mapKd);
      material.hasTexture = (material.textureId != 0);
    }
  }
}

void CleanupModel(Model &model) {
  for (auto &mesh : model.meshes) {
    if (mesh.vbo) {
      glDeleteBuffers(1, &mesh.vbo);
      mesh.vbo = 0;
    }
    if (mesh.vao) {
      glDeleteVertexArrays(1, &mesh.vao);
      mesh.vao = 0;
    }
  }

  for (auto &entry : model.materials) {
    if (entry.second.textureId) {
      glDeleteTextures(1, &entry.second.textureId);
      entry.second.textureId = 0;
      entry.second.hasTexture = false;
    }
  }
}
