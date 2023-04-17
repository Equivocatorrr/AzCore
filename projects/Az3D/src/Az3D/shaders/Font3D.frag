#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 texCoord;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec3 inBitangent;
layout(location=4) flat in int baseInstance;
layout(location=5) in vec3 inWorldPos;

layout(location=0) out vec4 outColor;

layout(set=0, binding=2) uniform sampler2D texSampler[1];

struct Material {
	// The following multiply with any texture bound (with default textures having a value of 1)
	vec4 color;
	vec3 emit;
	float normal;
	float metalness;
	float roughness;
	// Texture indices
	uint texAlbedo;
	uint texEmit;
	uint texNormal;
	uint texMetalness;
	uint texRoughness;
};

struct ObjectInfo {
	mat4 model;
	Material material;
};

layout(std140, set=0, binding=1) readonly buffer ObjectBuffer {
	ObjectInfo objects[];
} objectBuffer;

void main() {
	uint texAlbedo = objectBuffer.objects[baseInstance].material.texAlbedo;
	float alpha = smoothstep(0.4, 0.6, texture(texSampler[texAlbedo], texCoord).r);
	outColor = objectBuffer.objects[baseInstance].material.color;
	outColor *= alpha;
}
