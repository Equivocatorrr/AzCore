#version 450
#extension GL_ARB_separate_shader_objects : enable
// This extension allows us to use std430 for our uniform buffer
// Without it, the arrays of scalars would have a stride of 16 bytes
#extension GL_EXT_scalar_block_layout : enable
// So we can have a smaller uniform buffer
#extension GL_EXT_shader_8bit_storage : enable

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inScreenPos;
layout (location = 2) in mat2 inTransform;
layout (location = 4) in float inZShear;

layout (location = 0) out vec4 outColor;

const int MAX_LIGHTS = 256;
const int MAX_LIGHTS_PER_BIN = 16;
const int LIGHT_BIN_COUNT_X = 32;
const int LIGHT_BIN_COUNT_Y = 18;

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
	uint8_t lightIndices[MAX_LIGHTS_PER_BIN];
};

layout(std430, set=0, binding=0) uniform UniformBuffer {
	vec2 screenSize;
	vec3 ambientLight;
	LightBin lightBins[LIGHT_BIN_COUNT_X*LIGHT_BIN_COUNT_Y];
	Light lights[MAX_LIGHTS];
} ub;

layout(set=0, binding=1) uniform sampler2D texSampler[1];

struct Material {
	vec4 color;
	float emitStrength;
	float normalDepth;
};

struct TexIndices {
	int albedo;
	int normal;
	int emit;
};

layout(push_constant) uniform pushConstants {
	layout(offset = 64) Material mat;
	layout(offset = 88) TexIndices tex;
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

void main() {
	vec4 albedo = texture(texSampler[pc.tex.albedo], inTexCoord) * pc.mat.color;
	vec3 normal = texture(texSampler[pc.tex.normal], inTexCoord).rgb * 2.0 - vec3(1.0);
	vec3 emit = texture(texSampler[pc.tex.emit], inTexCoord).rgb * pc.mat.color.rgb * pc.mat.color.a;
	normal.xy = inTransform * normal.xy;
	normal = mix(vec3(0.0, 0.0, 1.0), normal, pc.mat.normalDepth);
	// Conditional normalize
	float len = length(normal);
	if (len > 0.1) normal /= len;
	
	float specularity = mix(0.05, 0.5, 1.0 - normal.z);
	
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);
	int binX = clamp(int(inScreenPos.x * float(LIGHT_BIN_COUNT_X) / ub.screenSize.x), 0, LIGHT_BIN_COUNT_X-1);
	int binY = clamp(int(inScreenPos.y * float(LIGHT_BIN_COUNT_Y) / ub.screenSize.y), 0, LIGHT_BIN_COUNT_Y-1);
	int bindex = binY * LIGHT_BIN_COUNT_X + binX;
	for (int i = 0; i < MAX_LIGHTS_PER_BIN; i++) {
		uint lightIndex = uint(ub.lightBins[bindex].lightIndices[i]);
		Light light = ub.lights[lightIndex];
		vec3 dPos = inScreenPos - light.position;
		float dist = length(dPos);
		dPos /= dist;
		float factor = smoothout(square(mapClamped(dist, light.distMax, light.distMin, 0.0, 1.0)));
		float angle = acos(dot(light.direction, dPos));
		factor *= smoothout(square(mapClamped(angle, light.angleMax, light.angleMin, 0.0, 1.0)));
		float incidence = clamp(dot(normal, -dPos), 0.0, 1.0);
		// diffuse
		float scattered = factor * mix(light.attenuation, 1.0, incidence);
		diffuse += light.color * scattered;
		// specular
		float reflected = factor * clamp(reflect(dPos, normal).z, 0.0, 1.0);
		specular += light.color * reflected;
	}
	
	specular = mix(specular, albedo.rgb * specular, 0.5);
	outColor.rgb = mix(diffuse * albedo.rgb, specular * albedo.a, specularity) + albedo.rgb * ub.ambientLight + emit * pc.mat.emitStrength;
	outColor.a = albedo.a;
}
