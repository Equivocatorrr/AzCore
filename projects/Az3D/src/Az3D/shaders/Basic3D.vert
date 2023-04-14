#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec3 outNormal;
layout(location=2) out int outBaseInstance;
layout(location=3) out vec3 outViewNormal;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
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
	mat4 finalTransformation = objectBuffer.objects[gl_BaseInstance].model * worldInfo.viewProj;
	mat4 modelView = objectBuffer.objects[gl_BaseInstance].model * worldInfo.view;
	// mat4 finalTransformation = objectBuffer.objects[gl_BaseInstance].model * worldInfo.view * worldInfo.proj;
	mat3 rotationScale = mat3(
		modelView[0].xyz,
		modelView[1].xyz,
		modelView[2].xyz
	);
	gl_Position = vec4(inPosition, 1.0) * finalTransformation;
	outTexCoord = inTexCoord;
	outNormal = -inNormal * rotationScale;
	outBaseInstance = gl_BaseInstance;
	outViewNormal = normalize((vec4(inPosition, 1.0) * modelView).xyz);
}
