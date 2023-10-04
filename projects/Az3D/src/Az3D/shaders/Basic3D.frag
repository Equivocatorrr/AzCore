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

const vec3 lightColor = vec3(1.0, 0.9, 0.8) * 2.0;

float sqr(float a) {
	return a * a;
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

vec3 wrap(float attenuation, vec3 wrapFac) {
	return clamp((vec3(attenuation) + wrapFac) / sqr(1.0 + wrapFac), 0.0, 1.0);
}

float ChebyshevInequality(vec2 moments, float depth) {
	float squaredMean = moments.y;
	float meanSquared = sqr(moments.x);
	float minVariance = sqr(0.004 * depth);
	float variance = max(squaredMean - meanSquared, minVariance);
	float pMax = variance / (variance + sqr(moments.x - depth));
	
	// return max(pMax, float(depth > moments.x));
	return pMax;
}

void main() {
	ObjectInfo info = objectBuffer.objects[inInstanceIndex];
	
	vec4 sunCoord = vec4(inWorldPos, 1.0) * worldInfo.sun;
	vec2 moments = texture(shadowMap, sunCoord.xy * 0.5 + 0.5).xy;
	float sunFactor = ChebyshevInequality(moments, sunCoord.z);
	
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
	// Linear attenuation looks nicer because it's softer. 
	float attenuationWrap = cosThetaLight; //GeometrySchlickGGX(cosThetaLight, k);
	vec3 fresnel = FresnelSchlick(cosThetaView, baseReflectivity);
	
	// This is a way to determine the "pointiness" of the surface to make sharper edges diffuse the subsurface lighting more, but it's flat along the triangle
	// The increased accuracy we get from this is nice, but gets outweighed by making the triangles obvious with sharp edges.
	// float sssSharpness = clamp(fwidth(cosThetaLight) / length(fwidth(viewDelta)) * 0.3, 0.0, 1.0);
	
	float isFoliage = float(info.material.isFoliage);
	// TODO: Use Radius and shadow maps to calculate back-facing lighting
	// This current method only looks convincing on rotund surfaces where scattering all the way through the object wouldn't be expected
	// For things like foliage, this is very unconvincing.
	vec3 sssWrap = tanh(info.material.sssRadius);
	
	// Squaring the denominator makes our output more physically accurate since otherwise we're generating more light than we absorb.
	vec3 wrapFac = wrap(attenuationWrap, sssWrap);
	vec3 invWrapFac = wrap(-attenuationWrap, sssWrap);
	vec3 diffuse = albedo.rgb * attenuationLight * (1.0 - sssFactor) * lightColor;
	// TODO: Maybe eliminate the 0.75 magic constant since it doesn't really come from anything. I just thought it looked nice.
	vec3 subsurface = mix(wrapFac, wrapFac + invWrapFac * info.material.sssColor * 0.75, isFoliage) * info.material.sssColor * sssFactor * lightColor;
	
	vec3 specular = lightColor * DistributionGGX(cosThetaViewHalfNormal, roughness) * attenuationLight;
	
	// outColor.rgb = normal * 0.5 + 0.5;
	// outColor.rgb = vec3(sssSharpness);
	// outColor.rgb = subsurface;
	// outColor.rgb = FresnelSchlick(surfaceNormal, viewNormal, baseReflectivity);
	outColor.rgb = 1.0 / PI * (mix(diffuse * (1.0 - metalness), specular, fresnel) * attenuationGeometry + subsurface) * sunFactor;
	// outColor.rg = texCoord;
	outColor.a = albedo.a;
	outColor.rgb += emit;
	outColor.rgb += albedo.rgb * worldInfo.ambientLight;
	// outColor.rgb = mix(outColor.rgb, worldInfo.fogColor, GetZFromDepth(inProjPos.z)/-(worldInfo.proj[2][3]/worldInfo.proj[2][2]));
}
