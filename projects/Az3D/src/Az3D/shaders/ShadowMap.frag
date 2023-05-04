#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 texCoord;
layout(location=1) flat in int baseInstance;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 sun;
	vec3 sunDir;
	vec3 eyePos;
} worldInfo;

layout(set=0, binding=2) uniform sampler2D texSampler[1];

const float PI = 3.1415926535897932;

struct Material {
	// The following multiply with any texture bound (with default textures having a value of 1)
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
};

struct ObjectInfo {
	mat4 model;
	Material material;
};

layout(std140, set=0, binding=1) readonly buffer ObjectBuffer {
	ObjectInfo objects[];
} objectBuffer;

void main() {
	ObjectInfo info = objectBuffer.objects[baseInstance];
	
	float alpha = texture(texSampler[info.material.texAlbedo], texCoord).a * info.material.color.a;
	if (alpha < 0.5) {
		discard;
	}
}
