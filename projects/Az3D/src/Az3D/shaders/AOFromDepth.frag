#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;

layout(location=0) out float outColor;

layout(set=0, binding=1) uniform sampler2D depthImage;

#include "headers/DepthBufferGeometry.glsl"
#include "headers/CommonFrag.glsl"

const uint numSteps = 24;
const uint numDirs = 4;

const vec2 directions[25] = vec2[25](
	vec2( 0.0, 1.0), vec2( 1.0, 0.5), vec2( 1.0,-0.5), vec2( 1.0/3.0,-1.0), vec2(-1.0,-1.0),
	vec2(-1.0, 0.0), vec2(-1.0, 1.0), vec2( 1.0/3.0, 1.0), vec2( 1.0, 1.0/3.0), vec2( 1.0,-2.0/3.0),
	vec2( 0.0,-1.0), vec2(-1.0,-2.0/3.0), vec2(-1.0, 1.0/3.0), vec2(-0.5, 1.0), vec2( 2.0/3.0, 1.0),
	vec2( 1.0, 0.0), vec2( 1.0,-1.0), vec2(-0.5,-1.0), vec2(-1.0,-0.5), vec2(-1.0, 2.0/3.0),
	vec2(-1.0/3.0, 1.0), vec2( 1.0, 1.0), vec2( 1.0,-0.5), vec2( 0.5,-1.0), vec2(-0.5,-1.0)
);

// const vec2 directions[25] = vec2[25](
// 	vec2( 0.0, 1.0), vec2( 2.0, 1.0), vec2( 2.0,-1.0), vec2( 1.0,-3.0), vec2(-1.0,-1.0),
// 	vec2(-1.0, 0.0), vec2(-1.0, 1.0), vec2( 1.0, 3.0), vec2( 3.0, 1.0), vec2( 3.0,-2.0),
// 	vec2( 0.0,-1.0), vec2(-3.0,-2.0), vec2(-3.0, 1.0), vec2(-1.0, 2.0), vec2( 2.0, 3.0),
// 	vec2( 1.0, 0.0), vec2( 1.0,-1.0), vec2(-1.0,-2.0), vec2(-2.0,-1.0), vec2(-3.0, 2.0),
// 	vec2(-1.0, 3.0), vec2( 1.0, 1.0), vec2( 2.0,-1.0), vec2( 1.0,-2.0), vec2(-1.0,-2.0)
// );

float tanToSin(float tangent) {
	return tangent * inversesqrt(sqr(tangent) + 1.0);
}

const float tanBias = tan(10.0 * PI / 180.0);

float tanToSinBiased(float tangent) {
	return tanToSin(max(tangent - tanBias, 0.0));
}

#define USE_RMS 0

void main() {
	vec2 viewUVScale = vec2(worldInfo.proj[0][0], -worldInfo.proj[2][1]) * 0.5;
	vec2 texelSize = 1.0 / textureSize(depthImage, 0);
	vec2 framebufferDims = round(gl_FragCoord.xy / inTexCoord);
	vec2 relativeSize = framebufferDims * texelSize;
	// Aim for the center of the texels, since we might be off due to scaling, and fp error can be a problem if you're on a texel edge.
	// vec2 centerUV = UVCenteredInTexel(inTexCoord, depthImage);
	vec2 centerUV = UVCenteredInTexel((floor(inTexCoord * framebufferDims) + 1.5 * relativeSize) / framebufferDims, depthImage);
	outColor = 0.0;
	int directionIndex = (int(gl_FragCoord.x) % 5) + (int(gl_FragCoord.y) % 5) * 5;
	vec3 centerPosView = GetViewPosFromDepthTexture(centerUV);
#if 0 // This version calculates positive and negative horizon slopes in screen space, which fails for faces that are perpendicular to each other and to view-z.
	// View-space derivatives across a single depthImage texel
	vec3 centerTangent, centerBitangent;
	GetViewUnnormalizedDerivatives(centerPosView, centerUV, centerTangent, centerBitangent);
	for (uint dir = 0; dir < numDirs; dir++) {
		vec2 stepOffsetDepthTexel = directions[(directionIndex + dir * 16) % 25];
		vec2 stepOffsetUV = stepOffsetDepthTexel * texelSize;
		vec2 stepOffsetView = stepOffsetUV / viewUVScale * centerPosView.z;
		float distPerStep = length(stepOffsetView);
		float startSlope = (stepOffsetDepthTexel.x * centerTangent.z + stepOffsetDepthTexel.y * centerBitangent.z) / distPerStep;
		float minSlope[2] = float[2](startSlope, -startSlope);
		vec2 offset[2] = vec2[2](centerUV, centerUV);
		float dist = 0.0;
		for (uint i = 0; i < numSteps; i++) {
			offset[0] += stepOffsetUV;
			offset[1] -= stepOffsetUV;
			dist += distPerStep;
			for (uint j = 0; j < 2; j++) {
				float z = GetZFromDepth(texture(depthImage, offset[j]).r);
				float offsetZ = z - centerPosView.z;
				float slope = offsetZ / dist;
				// if (abs(slope) <= 4.0) {
					minSlope[j] = min(minSlope[j], slope);
				// }
			}
		}
		outColor += clamp(1.0/PI / atan(max(-((minSlope[0] + minSlope[1]) / (1.0 - minSlope[0] * minSlope[1])), 0.01)), 0.0, 1.0);
	}
#else
	mat3 centerBasis;
	GetViewBasisFromDepthTexture(centerPosView, centerUV, centerBasis[0], centerBasis[1], centerBasis[2]);
	// mat3 viewToHorizonSpace = centerBasis;
	mat3 viewToHorizonSpace = transpose(centerBasis);
	const float maxDist = 3.0 * float(numSteps) * length(texelSize / viewUVScale * centerPosView.z);
	for (uint dir = 0; dir < numDirs; dir++) {
		vec2 stepOffsetDepthTexel = directions[(directionIndex + dir * 4) % 25];
		vec2 stepOffsetUV = stepOffsetDepthTexel * texelSize;
		vec2 stepOffsetView = stepOffsetUV / viewUVScale * centerPosView.z;
		float distPerStep = length(stepOffsetView);
		vec2 offset = centerUV;
		float dist = 0.0;
		float horizonTangent = 0.0;
		float horizonSin = 0.0;

		for (uint i = 0; i < numSteps; i++) {
			offset += stepOffsetUV;
			dist += distPerStep;
			vec3 posView = GetViewPosFromDepthTextureInterpolated(offset);
			vec3 delta = (centerPosView - posView);
			vec3 posHorizon = viewToHorizonSpace * delta;
			float tangent = posHorizon.z / length(posHorizon.xy);
			float myDist = length(delta);
			// float amount = clamp((2.0 - myDist / maxDist), 0.0, 1.0);
			// float amount = clamp((2.0 - myDist / maxDist - float(i)/float(numSteps)), 0.0, 1.0);
			float amount = clamp(sqrt((1.0 - myDist / maxDist) * (1.0 - float(i)/float(numSteps))), 0.0, 1.0);
			// float amount = 1.0;
			if (myDist < maxDist && tangent > horizonTangent) {
				horizonTangent = tangent;
				horizonSin += amount * (tanToSinBiased(tangent) - horizonSin);
			}
		}

		outColor += 1.0 - horizonSin;
		// outColor += max(1.0 - sqr(horizonSin) * 2.0, 0.0);
	}
#endif
	outColor /= float(numDirs);
#if USE_RMS
	// Store the inverted color so we can do a root-mean-square in AOConvolution instead of the square-mean-root, which would require 25 sqrts per pixel
	outColor *= outColor;
#endif
	outColor = 1.0 - outColor;
	// outColor = clamp(abs(2.0 / mix(mix(PI/2.0, atan(minSlope[0]), confidence[0]), -mix(PI, atan(minSlope[1]), confidence[1]), confidence[1] / (confidence[0] + confidence[1])) / PI), 0.0, 1.0);
}
