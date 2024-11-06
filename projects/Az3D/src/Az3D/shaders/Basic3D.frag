#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec3 inBitangent;
layout(location=4) flat in int inInstanceIndex;
layout(location=5) in vec3 inWorldPos;
layout(location=6) in vec4 inProjPos;

layout(location=0) out vec4 outColor;

#include "headers/Bindings3D.glsl"
#include "headers/CommonFrag.glsl"
#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"
#include "headers/Lighting.glsl"

void main() {
	ObjectInfo info = objectBuffer.objects[inInstanceIndex];

	outColor = CalculateAllLighting(info, inTexCoord, normalize(inNormal), normalize(inTangent), normalize(inBitangent));

	// outColor.rgb = TonemapACES(outColor.rgb);
}