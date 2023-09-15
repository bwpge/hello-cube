#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform SceneData {
	vec4 lightColor;
	vec4 lightDir;
} scene;

layout (set = 1, binding = 0) uniform sampler2D tex;

const float LIGHT_MIN = 0.5;

void main() {
    vec4 color = texture(tex, inTexCoord);
    if (color.a < 0.01) {
        discard;
    }

    float light = max(LIGHT_MIN, dot(scene.lightDir.rgb, inNormal));
    outColor = vec4(light * color.rgb, color.a) * scene.lightColor;
}
