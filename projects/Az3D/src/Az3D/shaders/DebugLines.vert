#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec4 inColor;

layout(location=0) out vec4 outColor;

#include "headers/WorldInfo.glsl"

void main() {
	gl_Position = worldInfo.viewProj * vec4(inPosition, 1.0);
	outColor = inColor;
}
