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
layout(location=1) out vec3 outNormal;
layout(location=2) out vec3 outTangent;
layout(location=3) out vec3 outBitangent;
layout(location=4) out int outInstanceIndex;
layout(location=5) out vec3 outWorldPos;
layout(location=6) out vec4 outProjPos;

#include "headers/Helpers.glsl"
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
	// With positions being scaled by S, normals must be scaled by 1/S
	// Scale is represented as the norm of the basis vectors, so scale them by 1/norm(B)^2
	mat3 modelRotationScale = mat3(
		model[0].xyz / normSqr(model[0].xyz),
		model[1].xyz / normSqr(model[1].xyz),
		model[2].xyz / normSqr(model[2].xyz)
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
