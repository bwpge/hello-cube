#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inLightPos;

layout(location = 0) out vec4 outColor;

const float LIGHT_MIN = 0.05;

void main() {
    vec3 lightDir = normalize(inLightPos - inPosition);
    float light = max(LIGHT_MIN, dot(lightDir, inNormal));

    outColor = light * vec4(inColor, 1.0);
}
