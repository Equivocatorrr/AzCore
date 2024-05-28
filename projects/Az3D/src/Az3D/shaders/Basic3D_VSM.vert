#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec2 inTexCoord;
layout(location=4) in uint inBoneIDs;
layout(location=5) in uint inBoneWeights;

layout(location=0) out vec2 outTexCoord;
layout(location=1) out int outInstanceIndex;
layout(location=2) out float outDepth;

#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"
#include "headers/BonesBuffer.glsl"

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
	vec4 worldPos = model * vec4(inPosition, 1.0);
	gl_Position = worldInfo.sun * worldPos;
	outTexCoord = inTexCoord;
	outInstanceIndex = gl_InstanceIndex;
	outDepth = 1.0 - gl_Position.z;
}
