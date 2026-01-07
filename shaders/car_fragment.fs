#version 330 core

// Entradas do vértice para o fragmento
in vec3 vNormal;      // Normal da superfície interpolada
in vec2 vTexCoord;    // Coordenadas de textura interpoladas
in vec3 vWorldPos;    // Posição do vértice no mundo

// Parâmetros uniformes definidos pela aplicação
uniform vec3 uColor;           // Cor base do objeto
uniform vec3 uLightDir;        // Direção da luz principal
uniform vec3 uLightDir2;       // Direção da luz secundária (preenchimento)
uniform vec3 uAmbient;         // Luz ambiente
uniform vec3 uViewPos;         // Posição da câmera
uniform sampler2D uTexture;    // Textura a ser aplicada
uniform bool uUseTexture;      // Usa textura ou não?

// Saída da cor final do fragmento
out vec4 FragColor;

void main() {
  // Normaliza a normal da superfície
  vec3 normal = normalize(vNormal);

  // Calcula a direção da câmera até o ponto
  vec3 viewDir = normalize(uViewPos - vWorldPos);

  // Direções das luzes (negativas porque são direcionais)
  vec3 lightDir1 = normalize(-uLightDir);
  vec3 lightDir2 = normalize(-uLightDir2);

  // Calcula a difusa da luz principal
  float diff1 = max(dot(normal, lightDir1), 0.0);

  // Calcula a difusa da luz secundária (mais fraca)
  float diff2 = max(dot(normal, lightDir2), 0.0) * 0.5;

  // Soma as contribuições difusas
  float diff = diff1 + diff2;

  // Parâmetros do brilho especular
  float specStrength = 0.25;  // Intensidade do brilho
  float shininess = 32.0;     // Concentração do brilho

  // Calcula reflexão especular para a luz principal
  vec3 reflectDir1 = reflect(-lightDir1, normal);
  float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), shininess);

  // Calcula reflexão especular para a luz secundária
  vec3 reflectDir2 = reflect(-lightDir2, normal);
  float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), shininess) * 0.5;

  // Combina os brilhos e aplica força
  float spec = specStrength * (spec1 + spec2);

  // Define a cor base: usa textura se ativada, senão usa a cor uniforme
  vec3 baseColor = uColor;
  if (uUseTexture) {
    baseColor = texture(uTexture, vTexCoord).rgb;
  }

  // Combina ambiente, difusa e especular
  vec3 color = uAmbient + baseColor * diff + vec3(spec);

  // Escreve a cor final no buffer
  FragColor = vec4(color, 1.0);
}