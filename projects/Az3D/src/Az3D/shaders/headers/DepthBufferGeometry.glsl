#ifndef DEPTH_BUFFER_GEOMETRY_GLSL
#define DEPTH_BUFFER_GEOMETRY_GLSL

#include "WorldInfo.glsl"

float GetZFromDepth(float depth) {
	return worldInfo.proj[3][2] / (depth - worldInfo.proj[1][2]);
}

vec3 GetViewPosFromZ(vec2 uv, float z) {
	vec3 position;
	position.z = z;
	position.x = (uv.x * 2.0 - 1.0) / worldInfo.proj[0][0] * z;
	position.y = (uv.y * 2.0 - 1.0) / -worldInfo.proj[2][1] * z;
	return position;
}

vec3 GetViewPosFromDepth(vec2 uv, float depth) {
	return GetViewPosFromZ(uv, GetZFromDepth(depth));
}

vec3 GetViewPosFromDepthTexture(vec2 uv) {
	return GetViewPosFromDepth(uv, texture(depthImage, uv).r);
}

vec3 GetViewPosFromDepthTextureInterpolated(vec2 uv) {
	vec2 size = textureSize(depthImage, 0);
	vec2 pixel = uv * size - 0.5;
	ivec2 texel = ivec2(floor(pixel));
	vec2 interp = (pixel - vec2(texel));
	float z = mix(
		mix(
			GetZFromDepth(texelFetch(depthImage, texel + ivec2(0, 0), 0).r),
			GetZFromDepth(texelFetch(depthImage, texel + ivec2(1, 0), 0).r),
			interp.x
		),
		mix(
			GetZFromDepth(texelFetch(depthImage, texel + ivec2(0, 1), 0).r),
			GetZFromDepth(texelFetch(depthImage, texel + ivec2(1, 1), 0).r),
			interp.x
		),
		interp.y
	);
	return GetViewPosFromZ(uv, z);
}

void GetViewUnnormalizedDerivatives(vec3 center, vec2 centerUV, out vec3 tangent, out vec3 bitangent) {
	vec2 texelSize = 1.0 / textureSize(depthImage, 0);
	vec3 up     = GetViewPosFromDepthTexture(centerUV - vec2(0.0, texelSize.y));
	vec3 down   = GetViewPosFromDepthTexture(centerUV + vec2(0.0, texelSize.y));
	vec3 left   = GetViewPosFromDepthTexture(centerUV - vec2(texelSize.x, 0.0));
	vec3 right  = GetViewPosFromDepthTexture(centerUV + vec2(texelSize.x, 0.0));

	vec3 dPdx1 = center - left;
	vec3 dPdx2 = right - center;
	tangent = normSqr(dPdx1) < normSqr(dPdx2) ? dPdx1 : dPdx2;

	vec3 dPdy1 = center - up;
	vec3 dPdy2 = down - center;
	bitangent = normSqr(dPdy1) < normSqr(dPdy2) ? dPdy1 : dPdy2;
}

void GetViewBasisFromDepthTexture(vec3 center, vec2 centerUV, out vec3 tangent, out vec3 bitangent, out vec3 normal) {
	GetViewUnnormalizedDerivatives(center, centerUV, tangent, bitangent);
	tangent = normalize(tangent);
	bitangent = normalize(bitangent);

	normal = cross(tangent, bitangent);
}

#endif // DEPTH_BUFFER_GEOMETRY_GLSL