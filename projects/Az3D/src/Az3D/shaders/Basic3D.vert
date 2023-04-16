#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec3 outNormal;
layout(location=2) out int outBaseInstance;
layout(location=3) out vec3 outWorldPos;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	vec3 eyePos;
} worldInfo;

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
	mat4 model = objectBuffer.objects[gl_BaseInstance].model;
	mat3 modelRotationScale = mat3(
		model[0].xyz,
		model[1].xyz,
		model[2].xyz
	);
	vec4 worldPos = vec4(inPosition, 1.0) * model;
	gl_Position = worldPos * worldInfo.viewProj;
	outTexCoord = inTexCoord;
	outNormal = inNormal * modelRotationScale;
	outBaseInstance = gl_BaseInstance;
	outWorldPos = worldPos.xyz;
}