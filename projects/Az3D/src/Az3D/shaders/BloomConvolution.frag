#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 inTexCoord;

layout (location = 0) out vec3 outColor;

layout(set=0, binding=0) uniform sampler2D image;

layout(push_constant) uniform PushConstants {
	vec2 direction;
} constants;

const uint numSamples = 7;
const float offsets[numSamples] = float[numSamples](
	-5.21052631578947,
	-3.31578947368421,
	-1.42105263157895,
	0.0,
	1.42105263157895,
	3.31578947368421,
	5.21052631578947
);
const float amounts[numSamples] = float[numSamples](
	0.0148051948051948,
	0.103636363636364,
	0.288701298701299,
	0.185714285714286,
	0.288701298701299,
	0.103636363636364,
	0.0148051948051948
);

void main() {
	vec2 texelSize = 1.0 / textureSize(image, 0);
	vec3 final = vec3(0.0);
	for (int i = 0; i < numSamples; i++) {
		final += texture(image, inTexCoord + texelSize * constants.direction * offsets[i]).rgb * amounts[i];
	}
	outColor = final;
}
