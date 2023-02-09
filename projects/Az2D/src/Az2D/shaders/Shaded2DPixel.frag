#version 450
#extension GL_ARB_separate_shader_objects : enable
// This extension allows us to use std430 for our uniform buffer
// Without it, the arrays of scalars would have a stride of 16 bytes
#extension GL_EXT_scalar_block_layout : enable

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec2 inScreenPos;
layout (location = 2) in mat2 inTransform;

layout (location = 0) out vec4 outColor;

const int MAX_LIGHTS = 1024;
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
	uint lightIndices[MAX_LIGHTS_PER_BIN];
};

layout(std430, set=0, binding=0) uniform UniformBuffer {
	vec2 screenSize;
	vec3 ambientLight;
	LightBin lightBins[LIGHT_BIN_COUNT_X*LIGHT_BIN_COUNT_Y];
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

vec2 SharpTexCoord(uint tex) {
	vec2 size = textureSize(texSampler[tex], 0);
	vec2 pixelPos = inTexCoord*size;
	vec2 pixel = floor(pixelPos + 0.5);
	vec2 subPixel = pixelPos - pixel;
	return (pixel + clamp(subPixel / fwidth(pixelPos), -0.5, 0.5)) / size;
}

vec2 NearestTexCoord(uint tex) {
	vec2 size = textureSize(texSampler[tex], 0);
	vec2 pixelPos = inTexCoord*size;
	vec2 pixel = floor(pixelPos - 0.5) + 1.0;
	return pixel / size;
}

vec3 GetNormal(vec2 pixel) {
	vec3 normal = texture(texSampler[pc.texNormal], pixel).rgb * 2.0 - vec3(1.0);
	normal.xy = inTransform * normal.xy;
	normal = mix(vec3(0.0, 0.0, 1.0), normal, pc.normalAttenuation);
	// Conditional normalize
	float len = length(normal);
	if (len > 0.1) normal /= len;
	return normal;
}

float bilerp(float a, float b,
             float c, float d,
             vec2 fac) {
	float top = mix(a, b, fac.x);
	float bot = mix(c, d, fac.x);
	return mix(top, bot, fac.y);
}

void main() {
	vec2 texSizeNormal = textureSize(texSampler[pc.texNormal], 0);
	vec2 pixelPosNormal = inTexCoord*texSizeNormal;
	vec2 pixelNormal = floor(pixelPosNormal + 0.5);
	vec2 subPixelNormal = clamp((pixelPosNormal - pixelNormal) / fwidth(pixelPosNormal) + 0.5, vec2(0.0), vec2(1.0));
	pixelNormal /= texSizeNormal;
	vec2 rSize = 1.0 / texSizeNormal;
	vec3 normal[4];
	normal[0] = GetNormal(pixelNormal + vec2(0.0,     0.0));
	normal[1] = GetNormal(pixelNormal + vec2(rSize.x, 0.0));
	normal[2] = GetNormal(pixelNormal + vec2(rSize.x, rSize.y));
	normal[3] = GetNormal(pixelNormal + vec2(0.0,     rSize.y));
	float normalFac[4];
	normalFac[0] = bilerp(1.0, 0.0,
	                      0.0, 0.0, subPixelNormal);
	normalFac[1] = bilerp(0.0, 1.0,
	                      0.0, 0.0, subPixelNormal);
	normalFac[2] = bilerp(0.0, 0.0,
	                      0.0, 1.0, subPixelNormal);
	normalFac[3] = bilerp(0.0, 0.0,
	                      1.0, 0.0, subPixelNormal);
	float specularity = 0.0;
	for (int i = 0; i < 4; i++) {
		specularity += mix(0.05, 0.5, 1.0 - normal[i].z) * normalFac[i];
	}
	
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);
	int binX = clamp(int(inScreenPos.x * float(LIGHT_BIN_COUNT_X) / ub.screenSize.x), 0, LIGHT_BIN_COUNT_X-1);
	int binY = clamp(int(inScreenPos.y * float(LIGHT_BIN_COUNT_Y) / ub.screenSize.y), 0, LIGHT_BIN_COUNT_Y-1);
	int bindex = binY * LIGHT_BIN_COUNT_X + binX;
	for (int i = 0; i < MAX_LIGHTS_PER_BIN; i++) {
		uint lightIndex = ub.lightBins[bindex].lightIndices[i];
		Light light = ub.lights[lightIndex];
		vec3 dPos = vec3(inScreenPos, 0.0) - light.position;
		float dist = length(dPos);
		dPos /= dist;
		float factor = smoothout(square(mapClamped(dist, light.distMax, light.distMin, 0.0, 1.0)));
		float angle = acos(dot(light.direction, dPos));
		factor *= smoothout(square(mapClamped(angle, light.angleMax, light.angleMin, 0.0, 1.0)));
		float incidence = 0.0;
		for (int j = 0; j < 4; j++) {
			incidence += clamp(dot(normal[j], -dPos), 0.0, 1.0) * normalFac[j];
		}
		// diffuse
		float scattered = factor * mix(light.attenuation, 1.0, incidence);
		diffuse += light.color * scattered;
		// specular
		for (int j = 0; j < 4; j++) {
			float reflected = factor * clamp(reflect(dPos, normal[j]).z, 0.0, 1.0);
			specular += light.color * reflected * normalFac[j];
		}
	}
	
	vec4 albedo = texture(texSampler[pc.texAlbedo], SharpTexCoord(pc.texAlbedo)) * pc.color;
	specular = mix(specular, albedo.rgb * specular, 0.5);
	outColor.rgb = mix(diffuse * albedo.rgb, specular * albedo.a, specularity) + albedo.rgb * ub.ambientLight;
	outColor.a = albedo.a;
	// outColor.r = subPixelNormal.x * outColor.a;
	// outColor.g = subPixelNormal.y * outColor.a;
	// outColor.b = 0.0;
	// outColor.rgb = (normal[3] * 0.5 + 0.5) * outColor.a;
}
