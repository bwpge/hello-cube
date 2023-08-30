#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragColor;

layout(push_constant) uniform constants
{
	mat4 projection;
	mat4 view;
	mat4 model;
} pc;

void main()
{
	gl_Position = pc.projection * pc.view * pc.model * vec4(inPosition, 1.0);
	fragPos = gl_Position.xyz;
	fragColor = inColor;
}
