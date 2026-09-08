#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "CommonFrag.glsl"
#include "WorldInfo.glsl"
#include "ObjectBuffer.glsl"

vec3 CalculateIncomingAmbientLight(vec3 normal, float roughness) {
	return mix(worldInfo.ambientLightDown, worldInfo.ambientLightUp, 0.5 + normal.z * 0.5 * (1.0 - roughness * 0.5));
}

vec4 CalculateAllLighting(ObjectInfo info, vec2 texCoord, vec3 surfaceNormal, vec3 surfaceTangent, vec3 surfaceBitangent) {
	vec4 sunCoord = worldInfo.sun * vec4(inWorldPos, 1.0);
	sunCoord.z = 1.0 - sunCoord.z;
	const vec2 shadowMapSize = textureSize(shadowMap, 0);
	const float boxBlurDimension = 5.0;
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
		// sunFactor = ChebyshevInequality(avgMoments, sunCoord.z);
		sunFactor = smoothstep(shrink, 1.0 - shrink, ChebyshevInequality(avgMoments, sunCoord.z));
	} else {
		sunFactor = 1.0;
	}

	float sssDistance = (sssDepth - sunCoord.z) / length(worldInfo.sun[2].xyz);

	vec4 albedo = texture(texSampler[info.texAlbedo], texCoord) * info.color;
	vec3 emit = texture(texSampler[info.texEmit], texCoord).rgb * info.emit;
	vec3 textureNormal = texture(texSampler[info.texNormal], texCoord).xyz * 2.0 - 1.0;
	float metalness = texture(texSampler[info.texMetalness], texCoord).x * info.metalness;
	float roughness = texture(texSampler[info.texRoughness], texCoord).x * info.roughness;
	roughness = max(0.001, sqr(roughness));
	float sssFactor = info.sssFactor;

	vec3 viewDelta = worldInfo.eyePos - inWorldPos;
	vec3 viewNormal = normalize(viewDelta);

	if (!gl_FrontFacing) {
		surfaceNormal = -surfaceNormal;
	}
	mat3 invTBN = transpose(mat3(surfaceTangent, surfaceBitangent, surfaceNormal));
	textureNormal = textureNormal * invTBN;
	vec3 actualNormal = normalize(mix(surfaceNormal, textureNormal, info.normal));
	// Treat the surface normal as an occluder for integrating ambient lighting
	vec3 bentNormal = normalize(mix(surfaceNormal, textureNormal, info.normal * 0.5));
	// When surface and texture normals are aligned, we have the whole hemisphere to integrate. When they're orthogonal we have a quarter of a sphere, so use this to reduce total incoming light contribution.
	float bentFactor = max(dot(surfaceNormal, textureNormal), 0.0) * 0.5 + 0.5;
	// NOTE: This isn't all that physically accurate as it makes the integration function shrink in both axes, as opposed to just the one with the angle between surfaceNormal and textureNormal, but that probably won't make enough difference to matter
	float roughnessAmbient = roughness * (bentFactor * 0.5 + 0.5);

	// Negate viewNormal because reflect expects the ray to be pointing towards the surface
	vec3 viewReflect = reflect(-viewNormal, actualNormal);

	vec3 lightNormal = worldInfo.sunDir;
	// Approximate spherical area lighting by moving our light normal towards the reflection vector
	// NOTE: This approximation can be improved by methods described here: https://advances.realtimerendering.com/s2017/DecimaSiggraph2017.pdf
	vec3 viewReflectedToLightNormal = viewReflect - dot(lightNormal, viewReflect) * lightNormal;
	vec3 lightNormalAdjusted = normalize(lightNormal + viewReflectedToLightNormal * clamp(sunTanRadius / length(viewReflectedToLightNormal), 0.0, 1.0));

	vec3 halfNormal = normalize(viewNormal + lightNormalAdjusted);

	float cosThetaView = max(dot(actualNormal, viewNormal), 0.0);
	float cosThetaLight = dot(actualNormal, lightNormal);
	float cosThetaViewHalfNormal = max(dot(actualNormal, halfNormal), 0.0);
	// Roughness remapping for direct lighting
	float k = sqr(roughness+1.0)/8.0;

	vec3 baseReflectivity = mix(vec3(0.04), albedo.rgb, metalness);
	float attenuationGeometry = GeometrySchlickGGX(cosThetaView, k);
	float attenuationLight = GeometrySchlickGGX(max(cosThetaLight, 0.0), k);
	float attenuationSpecular = DistributionGGX(cosThetaViewHalfNormal, roughness);
	float attenuationAmbient = mix(GeometrySchlickGGX(cosThetaView, roughnessAmbient), 1.0, 0.5);
	// Linear attenuation looks nicer because it's softer.
	float attenuationWrap = cosThetaLight; //GeometrySchlickGGX(cosThetaLight, k);
	vec3 fresnel = FresnelSchlick(cosThetaView, baseReflectivity);
	vec3 fresnelAmbient = FresnelSchlickRoughness(cosThetaView, baseReflectivity, roughness);

	float isFoliage = float(info.isFoliage);
	vec3 sssWrap = tanh(info.sssRadius);

	vec3 wrapFac = wrap(attenuationWrap, sssWrap);
	vec3 diffuse = albedo.rgb * attenuationLight * (1.0 - sssFactor) * sunLightColor * attenuationGeometry;

	vec3 ambientDiffuseColor = CalculateIncomingAmbientLight(bentNormal, roughnessAmbient);
	// Tint ambient color by albedo to fake a local surface bounce, rather than just darkening.
	ambientDiffuseColor *= mix(albedo.rgb, vec3(1.0), bentFactor);

	vec3 sssFac = min(1.0 - vec3(sssDistance) / info.sssRadius, 1.0);
	sssFac = pow(vec3(5.0 - isFoliage*3.0), sssFac - 1.0);
	sssFac = max(sssFac, 0.0);
	// 0.5 comes from the fact that light is only coming from one direction, but scattering in all directions
	vec3 subsurface = (sssFac * sunLightColor * 0.5 + ambientDiffuseColor) * attenuationAmbient;
	subsurface *= info.sssColor * sssFactor;

	vec3 specular = sunLightColor * attenuationSpecular * attenuationLight * attenuationGeometry;

	vec3 ambientDiffuse = albedo.rgb * ambientDiffuseColor * attenuationAmbient;
	vec3 ambientSpecular = CalculateIncomingAmbientLight(viewReflect, roughness);

	float ambientAmount = texture(aoSampler, inProjPos.xy / inProjPos.w * 0.5 + 0.5).r;

	vec3 ambient = mix(ambientDiffuse, ambientSpecular, fresnelAmbient * 0.5 * (1.0 - roughness)) * ambientAmount;

	vec4 result;
	result.rgb = 1.0 / PI * mix(diffuse * (1.0 - metalness), specular, fresnel) * sunFactor + subsurface + ambient;
	result.a = albedo.a;
	result.rgb += emit;
	// result.rgb = vec3(ambientAmount);
	return result;
}

#endif // LIGHTING_GLSL