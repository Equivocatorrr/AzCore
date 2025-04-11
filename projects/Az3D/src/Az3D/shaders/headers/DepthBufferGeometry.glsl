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
	vec3 up     = GetViewPosFromDepthTexture(vec2(centerUV.x, centerUV.y - texelSize.y));
	vec3 down   = GetViewPosFromDepthTexture(vec2(centerUV.x, centerUV.y + texelSize.y));
	vec3 left   = GetViewPosFromDepthTexture(vec2(centerUV.x - texelSize.x, centerUV.y));
	vec3 right  = GetViewPosFromDepthTexture(vec2(centerUV.x + texelSize.x, centerUV.y));

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

void GetViewUnnormalizedDerivativesAdvanced(vec3 center, vec2 centerUV, out vec3 tangent, out vec3 bitangent) {
	vec2 texelSize = 1.0 / textureSize(depthImage, 0);
	vec3 positions[9];
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			positions[x*3+y] = GetViewPosFromDepthTexture(centerUV + vec2(float(x-1), float(y-1)) * texelSize);
		}
	}
	float dPdxNormSqr = 10.0;
	for (int i = 0; i < 3; i++) {
		vec3 dPdx1 = positions[3 + i] - positions[0 + i];
		vec3 dPdx2 = positions[6 + i] - positions[3 + i];
		float myNormSqr1 = normSqr(dPdx1);
		float myNormSqr2 = normSqr(dPdx2);
		vec3 dPdx;
		float myNormSqr;
		if (myNormSqr1 < myNormSqr2) {
			dPdx = dPdx1;
			myNormSqr = myNormSqr1;
		} else {
			dPdx = dPdx2;
			myNormSqr = myNormSqr2;
		}
		if (myNormSqr < dPdxNormSqr) {
			tangent = dPdx;
			dPdxNormSqr = myNormSqr;
		}
	}
	float dPdyNormSqr = 10.0;
	for (int i = 0; i < 3; i++) {
		vec3 dPdy1 = positions[i*3 + 1] - positions[i*3 + 0];
		vec3 dPdy2 = positions[i*3 + 2] - positions[i*3 + 1];
		float myNormSqr1 = normSqr(dPdy1);
		float myNormSqr2 = normSqr(dPdy2);
		vec3 dPdy;
		float myNormSqr;
		if (myNormSqr1 < myNormSqr2) {
			dPdy = dPdy1;
			myNormSqr = myNormSqr1;
		} else {
			dPdy = dPdy2;
			myNormSqr = myNormSqr2;
		}
		if (myNormSqr < dPdyNormSqr) {
			bitangent = dPdy;
			dPdyNormSqr = myNormSqr;
		}
	}
}

void GetViewBasisFromDepthTextureAdvanced(vec3 center, vec2 centerUV, out vec3 tangent, out vec3 bitangent, out vec3 normal) {
	GetViewUnnormalizedDerivativesAdvanced(center, centerUV, tangent, bitangent);
	tangent = normalize(tangent);
	bitangent = normalize(bitangent);

	normal = cross(tangent, bitangent);
}

void GetViewBasisFromDepthTextureSimple(vec3 center, vec2 centerUV, out vec3 tangent, out vec3 bitangent, out vec3 normal) {
	tangent = dFdx(center);
	tangent = normalize(tangent);
	bitangent = dFdy(center);
	bitangent = normalize(bitangent);

	normal = cross(tangent, bitangent);
}

#endif // DEPTH_BUFFER_GEOMETRY_GLSL