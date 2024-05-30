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

#include "headers/CommonFrag.glsl"
#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"
#include "headers/Lighting.glsl"

const float atlasEdgeDistance = 0.12;
const float emPixels = 64.0;
const float edgeFactor = 1.0 / (emPixels * atlasEdgeDistance * 4.0);

float SDF(float edge, float value) {
	return linstep(0.5 - edge, 0.5 + edge, value);
}

void main() {
	ObjectInfo info = objectBuffer.objects[inObjectIndex];
	vec2 texSize = textureSize(texSampler[inTexAtlas], 0);
	vec2 edges = texSize * fwidth(inTexCoord) * edgeFactor;
	float edge = mix(edges.x, edges.y, 0.5);
	float sdf = texture(texSampler[inTexAtlas], inTexCoord).r;
	float alpha = SDF(edge, sdf);

	vec3 forward = normalize(inWorldPos - worldInfo.eyePos);
	vec2 parallax = vec2(dot(inTangent, forward), dot(inBitangent, forward));
	float sdf2 = texture(texSampler[inTexAtlas], inTexCoord + parallax / (texSize * edgeFactor * 10.0)).r;
	float backAlpha = SDF(edge, sdf2);

	vec2 edgeDirection = normalize(vec2(-dFdx(sdf), dFdy(sdf)));
	float backFaceFac = -float(gl_FrontFacing) * 2.0 + 1.0;
	edgeDirection.y *= backFaceFac;
	float cornerAlpha = SDF(edge * 2.0, sdf);
	vec3 normal = normalize(mix(inTangent * edgeDirection.x + inBitangent * edgeDirection.y, inNormal, cornerAlpha));
	vec3 tangent = normalize(mix((inNormal * edgeDirection.x + inTangent * edgeDirection.y) * backFaceFac, inTangent, cornerAlpha));
	vec3 bitangent = normalize(mix(cross(normal, tangent), inBitangent, cornerAlpha));
	outColor = CalculateAllLighting(info, inTexCoord, normal, tangent, bitangent);
	outColor.rgb = TonemapACES(outColor.rgb);
	outColor *= max(alpha, backAlpha);
}
