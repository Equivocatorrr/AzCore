#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;
layout(location=1) flat in uint inObjectIndex;
layout(location=2) centroid in float inDepth;
layout(location=3) flat in uint inTexAtlas;

layout(location=0) out vec4 outColor;

#include "headers/CommonFrag.glsl"
#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"

const float atlasEdgeDistance = 0.12;
const float emPixels = 64.0;
const float edgeFactor = 1.0 / (emPixels * atlasEdgeDistance * 4.0);

void main() {
	ObjectInfo info = objectBuffer.objects[inObjectIndex];
#if 0
	vec2 texSize = textureSize(texSampler[inTexAtlas], 0);
	vec2 edges = texSize * fwidth(inTexCoord) * edgeFactor;
	float edge = mix(edges.x, edges.y, 0.5);
	float alpha = smoothstep(0.5 - edge, 0.5 + edge, texture(texSampler[inTexAtlas], inTexCoord).r);
#else
	float alpha = texture(texSampler[inTexAtlas], inTexCoord).r;
#endif
	if (alpha < 0.5) discard;
	outColor.x = inDepth;
	outColor.y = inDepth*inDepth;
}
