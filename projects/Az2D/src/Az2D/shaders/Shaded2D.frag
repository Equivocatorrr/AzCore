#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec2 inScreenPos;

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

layout(set=0, binding=0) uniform UniformBuffer {
	vec2 screenSize;
	vec3 ambientLight;
	LightBin lightBins[16*9];
	Light lights[MAX_LIGHTS];
} ub;

layout(set=0, binding=1) uniform sampler2D samplerAlbedo[1];

layout(push_constant) uniform pushConstants {
	layout(offset = 32) vec4 color;
	layout(offset = 48) int texAlbedo;
	// layout(offset = 52) int texNormal;
} pc;

float map(float a, float min1, float max1, float min2, float max2) {
	return (a - min1) / (max1 - min1) * (max2 - min2) + min2;
}

float mapClamped(float a, float min1, float max1, float min2, float max2) {
	return clamp(map(a, min1, max1, min2, max2), min2, max2);
}

void main() {
	// to be replaced with normal maps
	vec3 normal = vec3(0.0, 0.0, 1.0);
	int binX = clamp(int(inScreenPos.x * 16.0 / ub.screenSize.x), 0, 15);
	int binY = clamp(int(inScreenPos.y * 9.0 / ub.screenSize.y), 0, 8);
	int bindex = binY * 16 + binX;
	vec3 lighting = ub.ambientLight;
	for (int i = 0; i < 8; i++) {
		uint lightIndex = ub.lightBins[bindex].lightIndices[i];
		Light light = ub.lights[lightIndex];
		vec3 dPos = vec3(inScreenPos, 0.0) - light.position;
		float dist = dot(dPos, dPos);
		float factor = mapClamped(dist, light.distMax * light.distMax, light.distMin * light.distMin, 0.0, 1.0);
		float cosAngle = dot(light.direction, dPos);
		factor *= mapClamped(cosAngle, light.angleMax, light.angleMin, 0.0, 1.0);
		float incidence = clamp(dot(normal, -dPos), 0.0, 1.0);
		factor *= mix(light.attenuation, 1.0, incidence);
		lighting += light.color * factor;
	}
	
	outColor = texture(samplerAlbedo[pc.texAlbedo], inTexCoord) * pc.color;
	outColor.rgb *= lighting;
}
