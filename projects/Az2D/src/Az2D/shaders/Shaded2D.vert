#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 inPosition;
layout(location=1) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec2 outScreenPos;

layout(push_constant) uniform pushConstants {
	layout(offset = 0) mat2 transform;
	layout(offset = 16) vec2 origin;
	layout(offset = 24) vec2 position;
} pc;

layout(set=0, binding=0) uniform UniformBuffer {
	vec2 screenSize;
} ub;

vec2 map(vec2 a, vec2 min1, vec2 max1, vec2 min2, vec2 max2) {
	return (a - min1) / (max1 - min1) * (max2 - min2) + min2;
}

float sqr(float a) {
	return a * a;
}

void main() {
	vec2 pos = pc.transform * (inPosition-pc.origin) + pc.position;
	gl_Position = vec4(pos, 0.0, 1.0);
	outTexCoord = inTexCoord;
	outScreenPos = (pos - 1.0) * 0.5 * ub.screenSize;
}
