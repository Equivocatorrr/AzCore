#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 inPosition;
layout(location=1) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;

layout(push_constant) uniform pushConstants {
	layout(offset = 0) mat2 transform;
	layout(offset = 16) vec2 origin;
	layout(offset = 24) vec2 position;
} pc;

void main() {
	gl_Position = vec4(pc.transform * (inPosition - pc.origin) + pc.position, 0.0, 1.0);
	outTexCoord = inTexCoord;
}
