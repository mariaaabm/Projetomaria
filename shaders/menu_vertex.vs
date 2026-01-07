#version 330 core

// Define as entradas do vértice com seus respectivos locais
layout(location = 0) in vec2 aPos;      // Posição 2D do vértice
layout(location = 1) in vec2 aTexCoord; // Coordenadas de textura

// Saída para o próximo estágio do pipeline (fragment shader)
out vec2 vTexCoord; // Passa as coordenadas de textura adiante

void main() {
  // Repassa as coordenadas de textura para o fragment shader
  vTexCoord = aTexCoord;

  // Define a posição final do vértice diretamente (já está em clip space)
  gl_Position = vec4(aPos, 0.0, 1.0);
}