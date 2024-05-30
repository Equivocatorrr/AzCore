#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 inTexCoord;

layout (location = 0) out vec2 outColor;

layout(set=0, binding=0) uniform sampler2D shadowMap;

layout(push_constant) uniform PushConstants {
	vec2 direction;
} constants;

const uint numSamples = 5;
const float offsets[numSamples] = float[numSamples](-2.0, -1.0, 0.0, 1.0, 2.0);
const float amounts[numSamples] = float[numSamples](1.0, 1.0, 1.0, 1.0, 1.0);

void main() {
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	vec2 final = vec2(0.0);
	float totalContribution = 0.0;
	for (int i = 0; i < numSamples; i++) {
		final += texture(shadowMap, inTexCoord + texelSize * constants.direction * offsets[i]).xy * amounts[i];
		totalContribution += amounts[i];
	}
	final /= totalContribution;
	outColor = final;
}
