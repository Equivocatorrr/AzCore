#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out int outInstanceIndex;
layout(location=2) out float outDepth;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 sun;
	vec3 sunDir;
	vec3 eyePos;
} worldInfo;

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
	mat4 model = objectBuffer.objects[gl_InstanceIndex].model;
	vec4 worldPos = vec4(inPosition, 1.0) * model;
	gl_Position = worldPos * worldInfo.sun;
	outTexCoord = inTexCoord;
	outInstanceIndex = gl_InstanceIndex;
	outDepth = gl_Position.z;
}
