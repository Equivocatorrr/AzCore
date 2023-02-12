#version 450
#extension GL_ARB_separate_shader_objects : enable
// This extension allows us to use std430 for our uniform buffer
// Without it, the arrays of scalars would have a stride of 16 bytes
#extension GL_EXT_scalar_block_layout : enable

layout(location=0) in vec2 inPosition;
layout(location=1) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec3 outScreenPos;
layout(location=2) out mat2 outTransform;
layout(location=4) out float outZShear;

layout(push_constant) uniform pushConstants {
	layout(offset = 0) mat2 transform;
	layout(offset = 16) vec2 origin;
	layout(offset = 24) vec2 position;
	layout(offset = 32) float z;
	layout(offset = 36) float zShear;
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
	vec2 localPos = pc.transform * (inPosition-pc.origin);
	vec2 pos = localPos + pc.position;
	float z = pc.z + pc.zShear * localPos.y;
	gl_Position = vec4(pos, 0.0, 1.0);
	outTexCoord = inTexCoord;
	outScreenPos = (vec3(pos, z) + 1.0) * 0.5 * vec3(ub.screenSize, ub.screenSize.y);
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
	outZShear = pc.zShear;
}
