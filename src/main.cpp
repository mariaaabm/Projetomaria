#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "game/game_state.h"
#include "game/police.h"
#include "game/collision.h"
#include "gl_utils.h"
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

struct ObjIndex {
  int v = 0;
  int vt = 0;
  int vn = 0;
};

static bool gDragging = false;
static double gLastX = 0.0;
static double gLastY = 0.0;
static float gYaw = 0.7f;
static float gPitch = 0.12f;
static float gDistance = 3.0f;
static Vec3 gTarget = {0.0f, 0.0f, 0.0f};

static std::string Trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return {};
  }
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

static std::vector<std::string> SplitWhitespace(const std::string &s) {
  std::istringstream iss(s);
  std::vector<std::string> parts;
  std::string part;
  while (iss >> part) {
    parts.push_back(part);
  }
  return parts;
}

static int ResolveIndex(int idx, int size) {
  if (idx > 0) {
    return idx - 1;
  }
  return size + idx;
}

static ObjIndex ParseFaceToken(const std::string &token) {
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

static std::string Dirname(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos) {
    return "";
  }
  return path.substr(0, pos + 1);
}

static bool LoadMtl(const std::string &path, std::unordered_map<std::string, Material> &materials) {
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
        current->kd = {std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])};
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

static bool LoadObj(const std::string &path, Model &model) {
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
      positions.push_back({std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])});
    } else if (parts[0] == "vt" && parts.size() >= 3) {
      texcoords.push_back({std::stof(parts[1]), std::stof(parts[2])});
    } else if (parts[0] == "vn" && parts.size() >= 4) {
      normals.push_back({std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])});
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

        Vec3 p0 = positions[ResolveIndex(i0.v, static_cast<int>(positions.size()))];
        Vec3 p1 = positions[ResolveIndex(i1.v, static_cast<int>(positions.size()))];
        Vec3 p2 = positions[ResolveIndex(i2.v, static_cast<int>(positions.size()))];

        bool hasNormals = (i0.vn != 0 && i1.vn != 0 && i2.vn != 0 && !normals.empty());
        Vec3 faceNormal = Normalize(Cross(p1 - p0, p2 - p0));

        auto addVertex = [&](const ObjIndex &idx, const Vec3 &fallbackNormal, const Vec3 &pos) {
          Vec3 normal = fallbackNormal;
          if (hasNormals) {
            int normalIndex = ResolveIndex(idx.vn, static_cast<int>(normals.size()));
            if (normalIndex >= 0 && normalIndex < static_cast<int>(normals.size())) {
              normal = normals[normalIndex];
            }
          }

          Vec2 texCoord = {0.0f, 0.0f};
          if (idx.vt != 0 && !texcoords.empty()) {
            int texIndex = ResolveIndex(idx.vt, static_cast<int>(texcoords.size()));
            if (texIndex >= 0 && texIndex < static_cast<int>(texcoords.size())) {
              texCoord = texcoords[texIndex];
            }
          }

          meshBuilders[currentMaterial].vertices.push_back({pos, normal, texCoord});
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

static void SetupMesh(Mesh &mesh) {
  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(offsetof(Vertex, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(offsetof(Vertex, texCoord)));

  glBindVertexArray(0);
}

static GLuint LoadTexture2D(const std::string &path) {
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!data) {
    std::cerr << "Falha ao carregar textura: " << path << "\n";
    return 0;
  }

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);
  return texture;
}

static void SetupTextures(Model &model) {
  for (auto &entry : model.materials) {
    Material &material = entry.second;
    if (!material.mapKd.empty()) {
      material.textureId = LoadTexture2D(material.mapKd);
      material.hasTexture = (material.textureId != 0);
    }
  }
}

static void CursorPosCallback(GLFWwindow *, double xpos, double ypos) {
  if (!gDragging) {
    return;
  }
  float dx = static_cast<float>(xpos - gLastX);
  float dy = static_cast<float>(ypos - gLastY);
  gLastX = xpos;
  gLastY = ypos;

  gYaw += dx * 0.005f;
  gPitch += dy * 0.005f;
  gPitch = std::clamp(gPitch, 0.02f, 1.4f);
}

static void MouseButtonCallback(GLFWwindow *window, int button, int action, int) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      gDragging = true;
      glfwGetCursorPos(window, &gLastX, &gLastY);
    } else if (action == GLFW_RELEASE) {
      gDragging = false;
    }
  }
}

static void ScrollCallback(GLFWwindow *, double, double yoffset) {
  gDistance *= (1.0f - static_cast<float>(yoffset) * 0.1f);
  gDistance = std::clamp(gDistance, 0.6f, 12.0f);
}

static void KeyCallback(GLFWwindow *, int key, int, int action, int) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT) {
    return;
  }

  const float rotateSpeed = 0.08f;
  const float zoomSpeed = 0.3f;

  if (key == GLFW_KEY_UP) {
    gPitch -= rotateSpeed;
    gPitch = std::clamp(gPitch, 0.02f, 1.4f);
  } else if (key == GLFW_KEY_DOWN) {
    gPitch += rotateSpeed;
    gPitch = std::clamp(gPitch, 0.02f, 1.4f);
  } else if (key == GLFW_KEY_LEFT) {
    gYaw += rotateSpeed;
  } else if (key == GLFW_KEY_RIGHT) {
    gYaw -= rotateSpeed;
  }
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Falha ao iniciar GLFW.\n";
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "Pista", nullptr, nullptr);
  if (!window) {
    std::cerr << "Falha ao criar janela.\n";
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSetCursorPosCallback(window, CursorPosCallback);
  glfwSetMouseButtonCallback(window, MouseButtonCallback);
  glfwSetScrollCallback(window, ScrollCallback);
  glfwSetKeyCallback(window, KeyCallback);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Falha ao iniciar GLEW.\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.18f, 0.19f, 0.21f, 1.0f);

  Model trackModel;
  if (!LoadObj("assets/textures/pista/pista.obj", trackModel)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  Model carModel;
  if (!LoadObj("assets/textures/carro/MrBeanCarFinal.obj", carModel)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  Model policeCarModel;
  if (!LoadObj("assets/textures/carro_policia/Ford Crown Victoria Police Interceptor.obj", policeCarModel)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  SetupTextures(trackModel);
  SetupTextures(carModel);
  SetupTextures(policeCarModel);

  for (auto &mesh : trackModel.meshes) {
    SetupMesh(mesh);
  }
  for (auto &mesh : carModel.meshes) {
    SetupMesh(mesh);
  }
  for (auto &mesh : policeCarModel.meshes) {
    SetupMesh(mesh);
  }

  GLuint trackProgram = CreateProgram("TrackVertexShader.glsl", "TrackFragmentShader.glsl");
  GLuint carProgram = CreateProgram("CarVertexShader.glsl", "CarFragmentShader.glsl");
  if (!trackProgram || !carProgram) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  GLint trackLocModel = glGetUniformLocation(trackProgram, "uModel");
  GLint trackLocView = glGetUniformLocation(trackProgram, "uView");
  GLint trackLocProj = glGetUniformLocation(trackProgram, "uProj");
  GLint trackLocColor = glGetUniformLocation(trackProgram, "uColor");
  GLint trackLocLight = glGetUniformLocation(trackProgram, "uLightDir");
  GLint trackLocLight2 = glGetUniformLocation(trackProgram, "uLightDir2");
  GLint trackLocAmbient = glGetUniformLocation(trackProgram, "uAmbient");
  GLint trackLocTexture = glGetUniformLocation(trackProgram, "uTexture");
  GLint trackLocUseTexture = glGetUniformLocation(trackProgram, "uUseTexture");

  GLint carLocModel = glGetUniformLocation(carProgram, "uModel");
  GLint carLocView = glGetUniformLocation(carProgram, "uView");
  GLint carLocProj = glGetUniformLocation(carProgram, "uProj");
  GLint carLocColor = glGetUniformLocation(carProgram, "uColor");
  GLint carLocLight = glGetUniformLocation(carProgram, "uLightDir");
  GLint carLocLight2 = glGetUniformLocation(carProgram, "uLightDir2");
  GLint carLocAmbient = glGetUniformLocation(carProgram, "uAmbient");
  GLint carLocTexture = glGetUniformLocation(carProgram, "uTexture");
  GLint carLocUseTexture = glGetUniformLocation(carProgram, "uUseTexture");

  const float worldScale = 40.0f;
  const float carScale = worldScale * 0.012f;
  const float policeCarScale = worldScale * 0.016f;
  const float carBaseRotation = 3.1415926f / 2.0f;
  const float policeBaseRotation = 3.1415926f * 2.0f;
  float carLift = 0.02f * carScale;
  float policeLift = 0.02f * policeCarScale;

  GameState gameState;
  MovementConfig movementConfig;
  MovementConfig policeMovementConfig = movementConfig;
  gameState.player.position = {0.0f, 0.0f, 0.0f};
  gameState.player.heading = 0.0f;
  gameState.police.position = {0.0f, 0.0f, -4.0f};
  gameState.police.heading = 0.0f;

  bool gameOver = false;
  const float catchDistance = 0.6f;
  const float policeStartDelay = 5.0f;

  float startTime = static_cast<float>(glfwGetTime());
  float lastFrameTime = startTime;
  gTarget = {0.0f, 0.0f, 0.0f};
  gYaw = 3.1415926f;
  gPitch = 0.12f;
  gDistance = 8.0f;

  while (!glfwWindowShouldClose(window)) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastFrameTime;
    float elapsedTime = currentTime - startTime;
    lastFrameTime = currentTime;

    if (!gameOver) {
      InputState input = ReadPlayerInput(window);
      UpdatePlayer(gameState.player, input, deltaTime, movementConfig, carBaseRotation);
      UpdatePoliceChase(gameState.police, gameState.player, deltaTime, elapsedTime, policeStartDelay, policeMovementConfig);

      if (CheckCaught(gameState.police, gameState.player, catchDistance)) {
        std::cout << "Game over, Mr. Bean got caught\n";
        gameOver = true;
        gameState.player.velocity = {0.0f, 0.0f, 0.0f};
        gameState.police.velocity = {0.0f, 0.0f, 0.0f};
      }
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
    Mat4 proj = Mat4Perspective(45.0f * 3.1415926f / 180.0f, aspect, 0.1f, 100.0f);

    Vec3 carPos = {
        gameState.player.position.x,
        -carModel.minY * carScale + carLift + 0.05f,
        gameState.player.position.z};
    gTarget = carPos;
    
    float camX = gTarget.x + gDistance * std::sin(gYaw) * std::cos(gPitch);
    float camY = gTarget.y + gDistance * std::sin(gPitch);
    float camZ = gTarget.z + gDistance * std::cos(gYaw) * std::cos(gPitch);
    Vec3 eye = {camX, camY, camZ};
    
    Mat4 view = Mat4LookAt(eye, gTarget, {0.0f, 1.0f, 0.0f});
    Mat4 trackMat = Mat4Multiply(
        Mat4Translate({0.0f, -trackModel.minY * worldScale, 0.0f}),
        Mat4Scale(worldScale));
    Mat4 carMat = Mat4Multiply(
        Mat4Translate(carPos),
        Mat4Multiply(Mat4RotateY(gameState.player.heading + carBaseRotation), Mat4Scale(carScale)));
    
    Vec3 policePos = {
        gameState.police.position.x,
        -policeCarModel.minY * policeCarScale + policeLift + 0.05f,
        gameState.police.position.z};
    Mat4 policeCarMat = Mat4Multiply(
        Mat4Translate(policePos),
        Mat4Multiply(Mat4RotateY(gameState.police.heading + policeBaseRotation), Mat4Scale(policeCarScale)));

    glUseProgram(trackProgram);
    glUniformMatrix4fv(trackLocModel, 1, GL_FALSE, trackMat.m);
    glUniformMatrix4fv(trackLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(trackLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(trackLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(trackLocLight2, 0.5f, -0.7f, 0.6f);
    glUniform3f(trackLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform1i(trackLocTexture, 0);

    for (const auto &mesh : trackModel.meshes) {
      const Material *material = nullptr;
      auto it = trackModel.materials.find(mesh.materialName);
      if (it != trackModel.materials.end()) {
        material = &it->second;
      }
      Vec3 color = material ? material->kd : Vec3{0.6f, 0.6f, 0.6f};
      glUniform3f(trackLocColor, color.x, color.y, color.z);
      if (material && material->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textureId);
        glUniform1i(trackLocUseTexture, 1);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(trackLocUseTexture, 0);
      }
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));
    }

    glUseProgram(carProgram);
    glUniformMatrix4fv(carLocModel, 1, GL_FALSE, carMat.m);
    glUniformMatrix4fv(carLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(carLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(carLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(carLocLight2, 0.5f, -0.7f, 0.6f);
    glUniform3f(carLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform1i(carLocTexture, 0);

    for (const auto &mesh : carModel.meshes) {
      const Material *material = nullptr;
      auto it = carModel.materials.find(mesh.materialName);
      if (it != carModel.materials.end()) {
        material = &it->second;
      }
      Vec3 color = material ? material->kd : Vec3{0.6f, 0.6f, 0.6f};
      glUniform3f(carLocColor, color.x, color.y, color.z);
      if (material && material->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textureId);
        glUniform1i(carLocUseTexture, 1);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(carLocUseTexture, 0);
      }
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));
    }

    glUseProgram(carProgram);
    glUniformMatrix4fv(carLocModel, 1, GL_FALSE, policeCarMat.m);
    glUniformMatrix4fv(carLocView, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(carLocProj, 1, GL_FALSE, proj.m);
    glUniform3f(carLocLight, -0.6f, -1.0f, -0.3f);
    glUniform3f(carLocAmbient, 0.22f, 0.22f, 0.22f);
    glUniform1i(carLocTexture, 0);

    for (const auto &mesh : policeCarModel.meshes) {
      const Material *material = nullptr;
      auto it = policeCarModel.materials.find(mesh.materialName);
      if (it != policeCarModel.materials.end()) {
        material = &it->second;
      }
      Vec3 color = material ? material->kd : Vec3{0.6f, 0.6f, 0.6f};
      glUniform3f(carLocColor, color.x, color.y, color.z);
      if (material && material->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textureId);
        glUniform1i(carLocUseTexture, 1);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(carLocUseTexture, 0);
      }
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteProgram(trackProgram);
  glDeleteProgram(carProgram);
  for (auto &mesh : trackModel.meshes) {
    if (mesh.vbo) {
      glDeleteBuffers(1, &mesh.vbo);
    }
    if (mesh.vao) {
      glDeleteVertexArrays(1, &mesh.vao);
    }
  }
  for (auto &mesh : carModel.meshes) {
    if (mesh.vbo) {
      glDeleteBuffers(1, &mesh.vbo);
    }
    if (mesh.vao) {
      glDeleteVertexArrays(1, &mesh.vao);
    }
  }
  for (auto &mesh : policeCarModel.meshes) {
    if (mesh.vbo) {
      glDeleteBuffers(1, &mesh.vbo);
    }
    if (mesh.vao) {
      glDeleteVertexArrays(1, &mesh.vao);
    }
  }
  for (auto &entry : trackModel.materials) {
    if (entry.second.textureId) {
      glDeleteTextures(1, &entry.second.textureId);
    }
  }
  for (auto &entry : carModel.materials) {
    if (entry.second.textureId) {
      glDeleteTextures(1, &entry.second.textureId);
    }
  }
  for (auto &entry : policeCarModel.materials) {
    if (entry.second.textureId) {
      glDeleteTextures(1, &entry.second.textureId);
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
