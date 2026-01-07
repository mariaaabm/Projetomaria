#version 330 core

// Coordenadas de textura recebidas do vertex shader
in vec2 vTexCoord;

// Cor final do fragmento a ser escrita no buffer
out vec4 FragColor;

// Textura a ser utilizada
uniform sampler2D uTexture;

void main() {
  // Busca a cor da textura nas coordenadas especificadas
  FragColor = texture(uTexture, vTexCoord);
}