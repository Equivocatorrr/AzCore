#ifndef COMMON_FRAG_GLSL
#define COMMON_FRAG_GLSL

layout(set=0, binding=3) uniform sampler2D texSampler[1];
layout(set=0, binding=4) uniform sampler2D shadowMap;

#include "Constants.glsl"
#include "Helpers.glsl"

float DistributionGGX(float cosThetaHalfViewNorm, float roughness) {
	roughness = sqr(roughness);
	float denominator = sqr(cosThetaHalfViewNorm) * (roughness - 1.0) + 1.0;
	return roughness / (PI * sqr(denominator));
}

// k is a remapping of roughness that depends on whether it's direct or indirect lighting
float GeometrySchlickGGX(float cosTheta, float k) {
	return cosTheta / ((abs(cosTheta) * (1.0 - k)) + k);
}

float GeometrySmith(float cosThetaView, float cosThetaLight, float k) {
	return GeometrySchlickGGX(cosThetaView, k) * GeometrySchlickGGX(cosThetaLight, k);
}

vec3 FresnelSchlick(float cosThetaView, vec3 baseReflectivity) {
	return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - cosThetaView, 5.0);
}

vec3 FresnelSchlickRoughness(float cosThetaView, vec3 baseReflectivity, float roughness) {
	return baseReflectivity + (max(vec3(1.0 - roughness), baseReflectivity) - baseReflectivity) * pow(1.0 - cosThetaView, 5.0);
}

vec3 wrap(float attenuation, vec3 wrapFac) {
	return clamp((vec3(attenuation) + wrapFac) / sqr(1.0 + wrapFac), 0.0, 1.0);
}

float linstep(float edge0, float edge1, float x)
{
	return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
}

const float minPenumbraVal = 0.5;
const float maxPenumbraVal = 1.0;

float ChebyshevInequality(vec2 moments, float depth) {
	float squaredMean = moments.y;
	float meanSquared = sqr(moments.x);
	float minVariance = sqr(0.001) * depth;
	float variance = max(squaredMean - meanSquared, minVariance);
	float pMax = variance / (variance + sqr(moments.x - depth));
	if (depth < 0.0) return 1.0;

	// Lower bound helps reduce light bleeding
	return max(linstep(minPenumbraVal, maxPenumbraVal, pMax), float(depth >= moments.x));
}

vec3 TonemapACES(vec3 color)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
}

#endif // COMMON_FRAG_GLSL