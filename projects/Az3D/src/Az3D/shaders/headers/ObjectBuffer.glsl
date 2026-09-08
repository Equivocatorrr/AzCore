#ifndef OBJECT_BUFFER_GLSL
#define OBJECT_BUFFER_GLSL

struct ObjectInfo {
	mat4 model;
	// Material must be laid out like this because if it's a struct, it will get padded and bonesOffset will also be padded out, wasting some space.
	vec4 color;
	vec3 emit;
	float normal;
	vec3 sssColor;
	float metalness;
	vec3 sssRadius;
	float roughness;
	float sssFactor;
	uint isFoliage;
	// Texture indices
	uint texAlbedo;
	uint texEmit;
	uint texNormal;
	uint texMetalness;
	uint texRoughness;
	// BONES
	uint bonesOffset;
};

layout(std140, set=0, binding=1) readonly buffer ObjectBuffer {
	ObjectInfo objects[];
} objectBuffer;

#endif // OBJECT_BUFFER_GLSL