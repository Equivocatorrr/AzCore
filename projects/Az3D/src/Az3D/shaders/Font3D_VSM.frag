#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;
layout(location=1) flat in vec3 inTangent;
layout(location=2) flat in vec3 inBitangent;
layout(location=3) in vec3 inWorldPos;
layout(location=4) centroid in float inDepth;
layout(location=5) flat in uint inTexAtlas;

layout(location=0) out vec4 outColor;

layout(set=0, binding=3) uniform sampler2D texSampler[1];

const vec3 inNormal = vec3(0.0); // We don't actually need anything other than alpha, so this value doesn't matter

#include "headers/Font3D.glsl"

void main() {
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	float alpha;
	Font3DGetGeometry(normal, tangent, bitangent, alpha);
	if (alpha < 0.5) discard;
	outColor.x = inDepth;
	outColor.y = inDepth*inDepth;
}
