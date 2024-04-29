#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 texCoord;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec3 inBitangent;
layout(location=4) flat in int inInstanceIndex;
layout(location=5) in vec3 inWorldPos;
layout(location=6) in vec4 inProjPos;

layout(location=0) out vec4 outColor;

#include "headers/CommonFrag.glsl"
#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"

void main() {
	ObjectInfo info = objectBuffer.objects[inInstanceIndex];

	vec4 sunCoord = worldInfo.sun * vec4(inWorldPos, 1.0);
	sunCoord.z = 1.0 - sunCoord.z;
	const vec2 shadowMapSize = textureSize(shadowMap, 0);
	const float boxBlurDimension = 3.0;
	// Converts meters to UV-space units
	const vec2 worldToUV = vec2(
		length(vec3(
			worldInfo.sun[0][0],
			worldInfo.sun[1][0],
			worldInfo.sun[2][0]
		)), length(vec3(
			worldInfo.sun[0][1],
			worldInfo.sun[1][1],
			worldInfo.sun[2][1]
		))
	);
	const vec2 lightWidth = max(shadowMapSize.x, shadowMapSize.y) * sunTanRadius * worldToUV;
	vec2 filterSize = boxBlurDimension / shadowMapSize;
	const int maxPCFDims = 3;
	const int maxPCFOffset = 1;
	vec2 moments[maxPCFDims][maxPCFDims];
	float momentContributions[maxPCFDims][maxPCFDims];
	float blockerDepth = 0.0;
	float blockerContribution = 0.0;
	for (int y = 0; y < maxPCFDims; y++) {
		for (int x = 0; x < maxPCFDims; x++) {
			vec2 uv = sunCoord.xy * 0.5 + 0.5 + filterSize * vec2(float(x-maxPCFOffset), float(y-maxPCFOffset));
			moments[y][x] = texture(shadowMap, uv).xy;
			momentContributions[y][x] = 1.0;
			if (uv.x <= 1.0/shadowMapSize.x || uv.x >= 1.0-1.0/shadowMapSize.x || uv.y <= 1.0/shadowMapSize.y || uv.y >= 1.0-1.0/shadowMapSize.y) {
				momentContributions[y][x] = 0.0;
			}
			if (moments[y][x].x >= sunCoord.z) {
				blockerDepth += moments[y][x].x;
				blockerContribution += 1.0;
			}
		}
	}
	if (blockerContribution == 0.0) {
		blockerDepth = moments[maxPCFOffset][maxPCFOffset].x;
	} else {
		blockerDepth /= blockerContribution;
	}
	// lightWidth is in unnormalized texture coordinates
	// depth ratio is unitless
	vec2 penumbraWidth = lightWidth * (blockerDepth - sunCoord.z) / blockerDepth;
	vec2 avgMoments = vec2(0.0);
	float avgContributions = 0.0;
	for (int y = 0; y < maxPCFDims; y++) {
		for (int x = 0; x < maxPCFDims; x++) {
			float dist = length(vec2(float(x-maxPCFOffset), float(y-maxPCFOffset)) / penumbraWidth) * boxBlurDimension;
			float contribution = 1.0 - clamp(dist*2.0 - 1.0, 0.0, 1.0);
			contribution *= momentContributions[y][x];
			avgMoments += moments[y][x] * contribution;
			avgContributions += contribution;
		}
	}
	avgMoments /= avgContributions;
	float sssDepth = avgMoments.x;
	float shrink = clamp((0.5 - min(penumbraWidth.x, penumbraWidth.y) / boxBlurDimension / 2.0) / (1.0-minPenumbraVal), 0.0, 0.3);
	float sunFactor;
	if (avgContributions != 0.0) {
		sunFactor = smoothstep(shrink, 1.0 - shrink, ChebyshevInequality(avgMoments, sunCoord.z));
	} else {
		sunFactor = 1.0;
	}

	float sssDistance = (sssDepth - sunCoord.z) / length(worldInfo.sun[2].xyz);

	vec4 albedo = texture(texSampler[info.texAlbedo], texCoord) * info.color;
	vec3 emit = texture(texSampler[info.texEmit], texCoord).rgb * info.emit;
	vec3 normal = texture(texSampler[info.texNormal], texCoord).xyz * 2.0 - 1.0;
	float metalness = texture(texSampler[info.texMetalness], texCoord).x * info.metalness;
	float roughness = texture(texSampler[info.texRoughness], texCoord).x * info.roughness;
	roughness = max(0.001, sqr(roughness));
	float sssFactor = info.sssFactor;

	vec3 surfaceNormal = normalize(inNormal);
	vec3 surfaceTangent = normalize(inTangent);
	vec3 surfaceBitangent = normalize(inBitangent);

	vec3 viewDelta = worldInfo.eyePos - inWorldPos;
	vec3 viewNormal = normalize(viewDelta);

	if (!gl_FrontFacing) {
		surfaceNormal = -surfaceNormal;
	}
	mat3 invTBN = transpose(mat3(surfaceTangent, surfaceBitangent, surfaceNormal));
	normal = normalize(mix(surfaceNormal, normal * invTBN, info.normal));

	// Negate viewNormal because reflect expects the ray to be pointing towards the surface
	vec3 viewReflect = reflect(-viewNormal, normal);

	vec3 lightNormal = worldInfo.sunDir;
	// Approximate spherical area lighting by moving our light normal towards the reflection vector
	// NOTE: This approximation can be improved by methods described here: https://advances.realtimerendering.com/s2017/DecimaSiggraph2017.pdf
	vec3 viewReflectedToLightNormal = viewReflect - dot(lightNormal, viewReflect) * lightNormal;
	vec3 lightNormalAdjusted = normalize(lightNormal + viewReflectedToLightNormal * clamp(sunTanRadius / length(viewReflectedToLightNormal), 0.0, 1.0));

	vec3 halfNormal = normalize(viewNormal + lightNormalAdjusted);

	float cosThetaView = max(dot(normal, viewNormal), 0.0);
	float cosThetaLight = dot(normal, lightNormal);
	float cosThetaViewHalfNormal = max(dot(normal, halfNormal), 0.0);
	// Roughness remapping for direct lighting
	float k = sqr(roughness+1.0)/8.0;

	vec3 baseReflectivity = mix(vec3(0.04), albedo.rgb, metalness);
	float attenuationGeometry = GeometrySchlickGGX(cosThetaView, k);
	float attenuationLight = GeometrySchlickGGX(max(cosThetaLight, 0.0), k);
	float attenuationSpecular = DistributionGGX(cosThetaViewHalfNormal, roughness);
	float attenuationAmbient = mix(GeometrySchlickGGX(cosThetaView, roughness), 1.0, 0.5);
	// Linear attenuation looks nicer because it's softer.
	float attenuationWrap = cosThetaLight; //GeometrySchlickGGX(cosThetaLight, k);
	vec3 fresnel = FresnelSchlick(cosThetaView, baseReflectivity);
	vec3 fresnelAmbient = FresnelSchlickRoughness(cosThetaView, baseReflectivity, roughness);

	float isFoliage = float(info.isFoliage);
	vec3 sssWrap = tanh(info.sssRadius);

	vec3 wrapFac = wrap(attenuationWrap, sssWrap);
	vec3 diffuse = albedo.rgb * attenuationLight * (1.0 - sssFactor) * sunLightColor * attenuationGeometry;

	vec3 sssFac = min(1.0 - vec3(sssDistance) / info.sssRadius, 1.0);
	sssFac = pow(vec3(5.0 - isFoliage*3.0), sssFac - 1.0);
	sssFac = max(sssFac, 0.0);
	// 0.5 comes from the fact that light is only coming from one direction, but scattering in all directions
	vec3 subsurface = (sssFac * sunLightColor * 0.5 + worldInfo.ambientLight) * attenuationAmbient;
	subsurface *= info.sssColor * sssFactor;

	vec3 specular = sunLightColor * attenuationSpecular * attenuationLight * attenuationGeometry;

	vec3 ambientDiffuse = albedo.rgb * worldInfo.ambientLight * attenuationAmbient;
	vec3 ambientSpecular = worldInfo.ambientLight;

	outColor.rgb = 1.0 / PI * mix(diffuse * (1.0 - metalness), specular, fresnel) * sunFactor + subsurface + mix(ambientDiffuse, ambientSpecular, fresnelAmbient * 0.5 * (1.0 - roughness));
	outColor.a = albedo.a;
	outColor.rgb += emit;

	outColor.rgb = TonemapACES(outColor.rgb);

	// outColor.rgb = vec3(inBoneAccum[0][0], inBoneAccum[1][1], inBoneAccum[2][2]);
}