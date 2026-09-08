#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;
layout(location=1) flat in vec3 inNormal;
layout(location=2) flat in vec3 inTangent;
layout(location=3) flat in vec3 inBitangent;
layout(location=4) flat in uint inObjectIndex;
layout(location=5) in vec3 inWorldPos;
layout(location=6) in vec4 inProjPos;
layout(location=7) flat in uint inTexAtlas;

layout(location=0) out vec4 outColor;

#include "headers/Bindings3D.glsl"
#include "headers/CommonFrag.glsl"
#include "headers/ObjectBuffer.glsl"
#include "headers/Font3D.glsl"
#include "headers/Lighting.glsl"

void main() {
	ObjectInfo info = objectBuffer.objects[inObjectIndex];
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	float alpha;
	Font3DGetGeometry(normal, tangent, bitangent, alpha);
	outColor = CalculateAllLighting(info, inTexCoord, normal, tangent, bitangent);
	// outColor.rgb = TonemapACES(outColor.rgb);
	outColor *= alpha;
}
