#version 450
#extension GL_ARB_separate_shader_objects : enable
// This extension allows us to use std430 for our uniform buffer
// Without it, the arrays of scalars would have a stride of 16 bytes
#extension GL_EXT_scalar_block_layout : enable

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec2 inScreenPos;
layout (location = 2) in mat2 inTransform;

layout (location = 0) out vec4 outColor;

const int MAX_LIGHTS = 128;

struct Light {
	// pixel-space position
	vec3 position;
	vec3 color;
	// How much light reaches the surface at a 90-degree angle of incidence in the range of 0.0 to 1.0
	float attenuation;
	// A normalized vector
	vec3 direction;
	// angular falloff in cos(radians) where < min is 100% brightness, between min and max blends, and > max is 0% brightness
	float angleMin;
	float angleMax;
	// distance-based falloff in pixel-space where < min is 100% brightness, between min and max blends, and > max is 0% brightness
	float distMin;
	float distMax;
};

struct LightBin {
	uint lightIndices[8];
};

layout(std430, set=0, binding=0) uniform UniformBuffer {
	vec2 screenSize;
	vec3 ambientLight;
	LightBin lightBins[16*9];
	Light lights[MAX_LIGHTS];
} ub;

layout(set=0, binding=1) uniform sampler2D texSampler[1];

layout(push_constant) uniform pushConstants {
	layout(offset = 32) vec4 color;
	layout(offset = 48) int texAlbedo;
	layout(offset = 52) int texNormal;
	layout(offset = 56) float normalAttenuation;
} pc;

float map(float a, float min1, float max1, float min2, float max2) {
	return (a - min1) / (max1 - min1) * (max2 - min2) + min2;
}

float mapClamped(float a, float min1, float max1, float min2, float max2) {
	return clamp(map(a, min1, max1, min2, max2), min2, max2);
}

float square(float a) {
	return a * a;
}
float smoothout(float a) {
	return smoothstep(0.0, 1.0, a);
}

vec3 DoLighting(vec3 normal) {
	int binX = clamp(int(inScreenPos.x * 16.0 / ub.screenSize.x), 0, 15);
	int binY = clamp(int(inScreenPos.y * 9.0 / ub.screenSize.y), 0, 8);
	int bindex = binY * 16 + binX;
	vec3 lighting = ub.ambientLight;
	for (int i = 0; i < 8; i++) {
		uint lightIndex = ub.lightBins[bindex].lightIndices[i];
		Light light = ub.lights[lightIndex];
		vec3 dPos = vec3(inScreenPos, 0.0) - light.position;
		float dist = length(dPos);
		dPos /= dist;
		float factor = smoothout(square(mapClamped(dist, light.distMax, light.distMin, 0.0, 1.0)));
		float angle = acos(dot(light.direction, dPos));
		factor *= smoothout(square(mapClamped(angle, light.angleMax, light.angleMin, 0.0, 1.0)));
		float incidence = clamp(dot(normal, -dPos), 0.0, 1.0);
		factor *= mix(light.attenuation, 1.0, incidence);
		lighting += light.color * factor;
	}
	return lighting;
}

vec2 SharpTexCoord(uint tex) {
	vec2 size = textureSize(texSampler[tex], 0);
	vec2 pixelPos = inTexCoord*size;
	vec2 pixel = floor(pixelPos - 0.5) + 1.0;
	vec2 subPixel = fract(pixelPos - 0.5) - 0.5;
	return (pixel + clamp(subPixel / fwidth(pixelPos), -0.5, 0.5)) / size;
}

void main() {
	vec3 normal = texture(texSampler[pc.texNormal], SharpTexCoord(pc.texNormal)).rgb * 2.0 - vec3(1.0);
	normal.xy = inTransform * normal.xy;
	normal = mix(vec3(0.0, 0.0, 1.0), normal, pc.normalAttenuation);
	outColor = texture(texSampler[pc.texAlbedo], SharpTexCoord(pc.texAlbedo)) * pc.color;
	outColor.rgb *= DoLighting(normal);
}
