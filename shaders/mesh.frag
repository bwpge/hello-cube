#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(0.424, 0.566, 0.707);

void main() {
    vec3 dX = dFdx(inPosition);
    vec3 dY = dFdy(inPosition);
    vec3 normal = normalize(cross(dX, dY));
    float light = max(0.0, dot(lightDir, normal));

    outColor = light * vec4(inColor, 1.0);
}
