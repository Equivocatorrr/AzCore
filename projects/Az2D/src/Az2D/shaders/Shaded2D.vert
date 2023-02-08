#version 450
#extension GL_ARB_separate_shader_objects : enable
// This extension allows us to use std430 for our uniform buffer
// Without it, the arrays of scalars would have a stride of 16 bytes
#extension GL_EXT_scalar_block_layout : enable

layout(location=0) in vec2 inPosition;
layout(location=1) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec2 outScreenPos;
layout(location=2) out mat2 outTransform;

layout(push_constant) uniform pushConstants {
	layout(offset = 0) mat2 transform;
	layout(offset = 16) vec2 origin;
	layout(offset = 24) vec2 position;
} pc;

layout(std430, set=0, binding=0) uniform UniformBuffer {
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
	outScreenPos = (pos + 1.0) * 0.5 * ub.screenSize;
	// vec2 basisX = normalize(vec2(pc.transform[0][0], pc.transform[1][0]));
	// vec2 basisY = normalize(vec2(pc.transform[0][1], pc.transform[1][1]));
	// outTransform = mat2(
	// 	basisX.x, basisY.x,
	// 	basisX.y, basisY.y
	// );
	outTransform = mat2(
		normalize(pc.transform[0]),
		normalize(pc.transform[1])
	);
}
