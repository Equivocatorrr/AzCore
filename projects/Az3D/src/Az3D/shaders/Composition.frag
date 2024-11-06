#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D rawSampler;

#include "headers/CommonFrag.glsl"
// #include "headers/WorldInfo.glsl"

void main() {
	vec4 rawColor = texture(rawSampler, inTexCoord);

	outColor.rgb = TonemapACES(rawColor.rgb);
	outColor.a = rawColor.a;
}