#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform SceneData {
	vec4 lightColor;
	vec4 lightDir;
} scene;

const float LIGHT_MIN = 0.05;

void main() {
    float light = max(LIGHT_MIN, dot(scene.lightDir.xyz, inNormal));

    outColor = vec4(light * inColor, 1.0) * scene.lightColor;
}
