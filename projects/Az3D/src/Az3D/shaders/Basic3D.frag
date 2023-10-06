#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 texCoord;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec3 inBitangent;
layout(location=4) flat in int inInstanceIndex;
layout(location=5) in vec3 inWorldPos;
layout(location=6) in vec3 inProjPos;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 sun;
	vec3 sunDir;
	vec3 eyePos;
	vec3 ambientLight;
	vec3 fogColor;
} worldInfo;

layout(set=0, binding=2) uniform sampler2D texSampler[1];
layout(set=0, binding=3) uniform sampler2D shadowMap;

const float PI = 3.1415926535897932;

struct Material {
	// The following multiply with any texture bound (with default textures having a value of 1)
	vec4 color;
	vec3 emit;
	float normal;
	vec3 sssColor;
	float metalness;
	vec3 sssRadius;
	float roughness;
	float sssFactor;
	uint isFoliage;
	// Texture indices
	uint texAlbedo;
	uint texEmit;
	uint texNormal;
	uint texMetalness;
	uint texRoughness;
};

struct ObjectInfo {
	mat4 model;
	Material material;
};

layout(std140, set=0, binding=1) readonly buffer ObjectBuffer {
	ObjectInfo objects[];
} objectBuffer;

const vec3 lightColor = vec3(1.0, 0.9, 0.8) * 4.0;

float sqr(float a) {
	return a * a;
}

float sqrNorm(vec3 a) {
	return dot(a, a);
}

vec3 sqr(vec3 a) {
	return a * a;
}

float GetZFromDepth(float depth)
{
	return worldInfo.proj[2][3] / (depth - worldInfo.proj[2][2]);
}

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

float ChebyshevInequality(vec2 moments, float depth) {
	float squaredMean = moments.y;
	float meanSquared = sqr(moments.x);
	float minVariance = sqr(0.004);
	float variance = max(squaredMean - meanSquared, minVariance);
	float pMax = variance / (variance + sqr(moments.x - depth));
	
	// Lower bound of 0.25 helps reduce light bleeding
	return smoothstep(0.25, 1.0, pMax);
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

void main() {
	ObjectInfo info = objectBuffer.objects[inInstanceIndex];
	
	vec4 sunCoord = vec4(inWorldPos, 1.0) * worldInfo.sun;
	const vec2 shadowMapSize = textureSize(shadowMap, 0);
	const float boxBlurDimension = 5.0;
	// Converts meters to UV-space units
	const vec2 worldToUV = vec2(length(worldInfo.sun[0].xyz), length(worldInfo.sun[1].xyz));
	const vec2 lightWidth = max(shadowMapSize.x, shadowMapSize.y) * 0.25 * worldToUV;
	vec2 filterSize = boxBlurDimension / shadowMapSize;
	vec2 moments[3][3];
	float blockerDepth = -1000.0;
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			vec2 uv = sunCoord.xy * 0.5 + 0.5 + filterSize * vec2(float(x-1), float(y-1));
			moments[y][x] = texture(shadowMap, uv).xy;
			blockerDepth = max(blockerDepth, moments[y][x].x);
		}
	}
	vec2 penumbraWidth = lightWidth * (blockerDepth - sunCoord.z) / blockerDepth;
	vec2 avgMoments = vec2(0.0);
	float avgContributions = 0.0;
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			float dist = length(vec2(float(x-1), float(y-1)) / penumbraWidth) * boxBlurDimension;
			float contribution = 1.0 - clamp(dist, 0.0, 1.0);
			avgMoments += moments[y][x] * contribution;
			avgContributions += contribution;
		}
	}
	avgMoments /= avgContributions;
	float sssDepth = avgMoments.x;
	float shrink = clamp(boxBlurDimension / max(penumbraWidth.x, penumbraWidth.y) / 2.0 - 1.0, 0.0, 0.4);
	float sunFactor = smoothstep(shrink, 1.0 - shrink, ChebyshevInequality(avgMoments, sunCoord.z));
	
	float sssDistance = (sssDepth - sunCoord.z) / length(worldInfo.sun[2].xyz);
	
	vec4 albedo = texture(texSampler[info.material.texAlbedo], texCoord) * info.material.color;
	vec3 emit = texture(texSampler[info.material.texEmit], texCoord).rgb * info.material.emit;
	vec3 normal = texture(texSampler[info.material.texNormal], texCoord).xyz * 2.0 - 1.0;
	float metalness = texture(texSampler[info.material.texMetalness], texCoord).x * info.material.metalness;
	float roughness = texture(texSampler[info.material.texRoughness], texCoord).x * info.material.roughness;
	roughness = sqr(roughness);
	float sssFactor = info.material.sssFactor;
	
	vec3 surfaceNormal = normalize(inNormal);
	vec3 surfaceTangent = normalize(inTangent);
	vec3 surfaceBitangent = normalize(inBitangent);
	
	vec3 lightNormal = worldInfo.sunDir;
	
	vec3 viewDelta = worldInfo.eyePos - inWorldPos;
	vec3 viewNormal = normalize(viewDelta);
	vec3 halfNormal = normalize(viewNormal + lightNormal);
	
	
	surfaceNormal *= sign(dot(surfaceNormal, viewNormal));
	mat3 invTBN = transpose(mat3(surfaceTangent, surfaceBitangent, surfaceNormal));
	normal = normalize(mix(surfaceNormal, normal * invTBN, info.material.normal));
	float cosThetaView = max(dot(normal, viewNormal), 0.0);
	float cosThetaLight = dot(normal, lightNormal);
	float cosThetaViewHalfNormal = max(dot(normal, halfNormal), 0.0);
	// Roughness remapping for direct lighting
	float k = sqr(roughness+1.0)/8.0;
	
	vec3 baseReflectivity = mix(vec3(0.04), albedo.rgb, metalness);
	float attenuationGeometry = GeometrySchlickGGX(cosThetaView, k);
	float attenuationLight = GeometrySchlickGGX(max(cosThetaLight, 0.0), k);
	float attenuationSpecular = DistributionGGX(cosThetaViewHalfNormal, roughness);
	// Linear attenuation looks nicer because it's softer. 
	float attenuationWrap = cosThetaLight; //GeometrySchlickGGX(cosThetaLight, k);
	vec3 fresnel = FresnelSchlick(cosThetaView, baseReflectivity);
	vec3 fresnelAmbient = FresnelSchlickRoughness(cosThetaView, baseReflectivity, roughness);
	
	float isFoliage = float(info.material.isFoliage);
	vec3 sssWrap = tanh(info.material.sssRadius);
	
	vec3 wrapFac = wrap(attenuationWrap, sssWrap);
	vec3 diffuse = albedo.rgb * attenuationLight * (1.0 - sssFactor) * lightColor * attenuationGeometry;
	
	vec3 sssFac = min(1.0 - vec3(sssDistance) / info.material.sssRadius, 1.0);
	sssFac = pow(vec3(5.0 - isFoliage*3.0), sssFac - 1.0);
	sssFac = max(sssFac, 0.0);
	// 0.5 comes from the fact that light is only coming from one direction, but scattering in all directions
	vec3 subsurface = sssFac * lightColor * 0.5 + worldInfo.ambientLight * 0.5;
	subsurface *= info.material.sssColor * sssFactor;
	
	vec3 specular = lightColor * attenuationSpecular * attenuationLight * attenuationGeometry;
	
	vec3 ambientDiffuse = albedo.rgb * worldInfo.ambientLight * 0.5;
	vec3 ambientSpecular = worldInfo.ambientLight;
	
	outColor.rgb = 1.0 / PI * mix(diffuse * (1.0 - metalness), specular, fresnel) * sunFactor + subsurface + mix(ambientDiffuse, ambientSpecular, fresnelAmbient * 0.25 * (1.0 - roughness));
	outColor.a = albedo.a;
	outColor.rgb += emit;
	
	outColor.rgb = TonemapACES(outColor.rgb);
}