#version 330 core

// Entradas do fragment shader
in vec3 vNormal;    // Normal interpolada do vértice
in vec2 vTexCoord;  // Coordenadas de textura
in vec3 vWorldPos;  // Posição do fragmento no espaço do mundo

// Uniformes para controle de cor, luz e textura
uniform vec3 uColor;       // Cor base do objeto
uniform vec3 uLightDir;    // Direção da luz principal
uniform vec3 uLightDir2;   // Direção da luz secundária (fill light)
uniform vec3 uAmbient;     // Luz ambiente
uniform vec3 uViewPos;     // Posição da câmera/observador
uniform sampler2D uTexture;// Textura para aplicar
uniform bool uUseTexture;  // Controla se usa textura ou cor base

// Saída da cor final do fragmento
out vec4 FragColor;

void main() {
  // Normaliza a normal para cálculo correto de iluminação
  vec3 normal = normalize(vNormal);

  // Calcula direção do observador para o fragmento
  vec3 viewDir = normalize(uViewPos - vWorldPos);

  // Normaliza as direções das luzes (invertidas para cálculo)
  vec3 lightDir1 = normalize(-uLightDir);
  vec3 lightDir2 = normalize(-uLightDir2);

  // Calcula componente difusa da luz principal e secundária (mais suave)
  float diff1 = max(dot(normal, lightDir1), 0.0);
  float diff2 = max(dot(normal, lightDir2), 0.0) * 0.5; // luz de preenchimento mais fraca
  float diff = diff1 + diff2;

  // Define força e brilho do reflexo especular
  float specStrength = 0.25;
  float shininess = 32.0;

  // Calcula componente especular para as duas luzes
  float spec1 = pow(max(dot(viewDir, reflect(-lightDir1, normal)), 0.0), shininess);
  float spec2 = pow(max(dot(viewDir, reflect(-lightDir2, normal)), 0.0), shininess) * 0.5;
  float spec = specStrength * (spec1 + spec2);

  // Define a cor base, usando textura se habilitada
  vec3 baseColor = uColor;
  if (uUseTexture) {
    baseColor = texture(uTexture, vTexCoord).rgb;
  }

  // Combina luz ambiente, difusa e especular para cor final
  vec3 color = uAmbient + baseColor * diff + vec3(spec);

  // Define a cor final do fragmento com alfa 1 (opaco)
  FragColor = vec4(color, 1.0);
}