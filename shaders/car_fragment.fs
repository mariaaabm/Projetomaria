#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uColor;
uniform vec3 uLightDir;
uniform vec3 uLightDir2;
uniform vec3 uAmbient;
uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main() {
  vec3 normal = normalize(vNormal);
  float diff1 = max(dot(normal, normalize(-uLightDir)), 0.0);
  float diff2 = max(dot(normal, normalize(-uLightDir2)), 0.0);
  float diff = diff1 + diff2;
  vec3 baseColor = uColor;
  if (uUseTexture) {
    baseColor = texture(uTexture, vTexCoord).rgb;
  }
  vec3 color = uAmbient + baseColor * diff;
  FragColor = vec4(color, 1.0);
}
