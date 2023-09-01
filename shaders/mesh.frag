#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;

// DEBUG: hardcoded light position
const vec3 LIGHT_POSITION = vec3(1.2, 1.0, 2.0);
const float LIGHT_MIN = 0.15;

void main() {
    vec3 lightDir = normalize(LIGHT_POSITION - inPosition);

    vec3 dX = dFdx(inPosition);
    vec3 dY = dFdy(inPosition);
    vec3 normal = normalize(cross(dX, dY));
    float light = max(LIGHT_MIN, dot(lightDir, normal));

    outColor = light * vec4(inColor, 1.0);
}
