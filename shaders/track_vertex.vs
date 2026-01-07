#version 330 core

// Define as entradas do vértice com seus respectivos locais
layout(location = 0) in vec3 aPos;      // Posição do vértice
layout(location = 1) in vec3 aNormal;   // Normal do vértice
layout(location = 2) in vec2 aTexCoord; // Coordenadas de textura

// Matrizes de transformação definidas pela aplicação
uniform mat4 uModel;  // Matriz de modelo (objeto -> mundo)
uniform mat4 uView;   // Matriz de visão (mundo -> câmera)
uniform mat4 uProj;   // Matriz de projeção (câmera -> tela)

// Saídas para o próximo estágio do pipeline (fragment shader)
out vec3 vNormal;     // Normal transformada para o espaço do mundo
out vec2 vTexCoord;   // Coordenadas de textura repassadas
out vec3 vWorldPos;   // Posição do vértice no espaço do mundo

void main() {
  // Transforma a posição do vértice para o espaço do mundo
  vec4 worldPos = uModel * vec4(aPos, 1.0);

  // Transforma a normal para o espaço do mundo (sem translação)
  vNormal = mat3(uModel) * aNormal;

  // Passa as coordenadas de textura para o fragment shader
  vTexCoord = aTexCoord;

  // Passa a posição no mundo para o fragment shader
  vWorldPos = worldPos.xyz;

  // Calcula a posição final do vértice na tela
  gl_Position = uProj * uView * worldPos;
}