#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec4 inColor;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
} worldInfo;

void main() {
	gl_Position = vec4(inPosition, 1.0) * worldInfo.viewProj;
	outColor = inColor;
}
