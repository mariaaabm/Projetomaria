#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vWorldPos;

uniform vec3 uColor;
uniform vec3 uLightDir;
uniform vec3 uLightDir2;
uniform vec3 uAmbient;
uniform vec3 uViewPos;
uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main() {
  vec3 normal = normalize(vNormal);
  vec3 viewDir = normalize(uViewPos - vWorldPos);
  vec3 lightDir1 = normalize(-uLightDir);
  vec3 lightDir2 = normalize(-uLightDir2);

  float diff1 = max(dot(normal, lightDir1), 0.0);
  float diff2 = max(dot(normal, lightDir2), 0.0) * 0.5; // soften fill light
  float diff = diff1 + diff2;

  float specStrength = 0.25;
  float shininess = 32.0;
  float spec1 = pow(max(dot(viewDir, reflect(-lightDir1, normal)), 0.0), shininess);
  float spec2 = pow(max(dot(viewDir, reflect(-lightDir2, normal)), 0.0), shininess) * 0.5;
  float spec = specStrength * (spec1 + spec2);

  vec3 baseColor = uColor;
  if (uUseTexture) {
    baseColor = texture(uTexture, vTexCoord).rgb;
  }
  vec3 color = uAmbient + baseColor * diff + vec3(spec);
  FragColor = vec4(color, 1.0);
}
