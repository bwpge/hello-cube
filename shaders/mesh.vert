#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragColor;
layout (location = 3) out vec3 fragLightPos;

layout(push_constant) uniform constants
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec3 lightPos;
} pc;

void main()
{
	gl_Position = pc.projection * pc.view * pc.model * vec4(inPosition, 1.0);
	fragPos = vec3(pc.model * vec4(inPosition, 1.0));
	fragNormal = normalize(mat3(transpose(inverse(pc.model))) * inNormal);
	fragColor = inColor;
	fragLightPos = pc.lightPos;
}
