#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec2 inTexCoord;
layout(location=4) in uint inBoneIDs;
layout(location=5) in uint inBoneWeights;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec3 outTangent;
layout(location=3) out vec3 outBitangent;
layout(location=4) out int outInstanceIndex;
layout(location=5) out vec3 outWorldPos;
layout(location=6) out vec4 outProjPos;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 sun;
	vec3 sunDir;
	vec3 eyePos;
} worldInfo;

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

layout(std140, set=0, binding=2) readonly buffer BonesBuffer {
	mat4 bones[];
} bonesBuffer;

float sqrNorm(vec3 a) {
	return dot(a, a);
}

void main() {
	mat4 model = objectBuffer.objects[gl_InstanceIndex].model;
	mat4 boneAccum = mat4(0);
	if (inBoneWeights != 0) {
		for (int i = 0; i < 4; i++) {
			uint boneID = (inBoneIDs >> i*8) & 255;
			float boneWeight = float((inBoneWeights >> i*8) & 255) / 255.0;
			boneAccum += bonesBuffer.bones[boneID + objectBuffer.objects[gl_InstanceIndex].bonesOffset] * boneWeight;
		}
		model = model * boneAccum;
	}
	// With positions being scaled by S, normals must be scaled by 1/S
	// Scale is represented as the norm of the basis vectors, so scale them by 1/norm(B)^2
	mat3 modelRotationScale = mat3(
		model[0].xyz / sqrNorm(model[0].xyz),
		model[1].xyz / sqrNorm(model[1].xyz),
		model[2].xyz / sqrNorm(model[2].xyz)
	);
	vec4 worldPos = model * vec4(inPosition, 1.0);
	gl_Position = worldInfo.viewProj * worldPos;
	outTexCoord = inTexCoord;
	outNormal = normalize(modelRotationScale * inNormal);
	outTangent = normalize(modelRotationScale * inTangent);
	outBitangent = normalize(cross(outNormal, outTangent));
	outInstanceIndex = gl_InstanceIndex;
	outWorldPos = worldPos.xyz;
	outProjPos = gl_Position;
}
