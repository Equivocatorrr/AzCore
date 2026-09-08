#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 inTexCoord;

layout(location=0) out float outColor;

layout(set=0, binding=1) uniform sampler2D depthImage;
layout(set=0, binding=2) uniform sampler2D noisyImage;

#include "headers/DepthBufferGeometry.glsl"
#include "headers/CommonFrag.glsl"

const int numSamples = 5;
const float maxNormalDiff = 0.05;

const float aoStrength = 0.75;

#define USE_RMS 0

void main() {
	vec2 texSizeImage = textureSize(noisyImage, 0);
	vec2 texelSizeDepth = 1.0 / textureSize(depthImage, 0);
	vec2 texelSizeImage = 1.0 / texSizeImage;
	vec2 relativeSize = texSizeImage * texelSizeDepth;

	// vec2 centerUVDepth = UVCenteredInTexel(inTexCoord, depthImage);
	vec2 centerUVDepth = UVCenteredInTexel((floor(inTexCoord * texSizeImage) + 1.5 * relativeSize) / texSizeImage, depthImage);
	vec2 centerUVImage = UVCenteredInTexel(inTexCoord, noisyImage);

	vec3 centerPosView = GetViewPosFromDepthTexture(centerUVDepth);
	vec3 centerTangent, centerBitangent;
	GetViewUnnormalizedDerivatives(centerPosView, centerUVDepth, centerTangent, centerBitangent);
	centerTangent *= texelSizeImage.x / texelSizeDepth.x;
	centerBitangent *= texelSizeImage.x / texelSizeDepth.x;


	float final = 0.0;
	float totalContribution = 0.0;
	for (int x = -numSamples/2; x <= numSamples/2; x++) {
		for (int y = -numSamples/2; y <= numSamples/2; y++) {
			vec2 offsetTexels = vec2(x, y);
			vec2 offset = offsetTexels * texelSizeImage;
			vec2 depthUV = centerUVDepth + offset;
			vec2 imageUV = centerUVImage + offset;
			vec3 position = GetViewPosFromDepthTextureInterpolated(depthUV);
			vec3 planePosition = centerPosView + centerTangent * offsetTexels.x + centerBitangent * offsetTexels.y;
			float weight = clamp(1.0 - abs(position.z - planePosition.z) / maxNormalDiff, 0.0, 1.0);
#if USE_RMS
			final += sqr(texture(noisyImage, imageUV).r) * weight;
#else
			final += texture(noisyImage, imageUV).r * weight;
#endif
			totalContribution += weight;
		}
	}
#if USE_RMS
	// Root mean square helps deal with our undersampling erring on the side of allowing ambient light through, so we want the darker pixels to matter more (which in this case are brighter pixels because we inverted it).
	outColor = 1.0 - sqrt(final / totalContribution) * aoStrength;
#else
	outColor = 1.0 - final / totalContribution * aoStrength;
#endif
	// outColor *= outColor;
}
