#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 texCoord;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec3 inBitangent;
layout(location=4) flat in int baseInstance;
layout(location=5) in vec3 inWorldPos;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform WorldInfo {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	vec3 eyePos;
} worldInfo;

layout(set=0, binding=2) uniform sampler2D texSampler[1];

const float PI = 3.1415926535897932;

struct Material {
	// The following multiply with any texture bound (with default textures having a value of 1)
	vec4 color;
	vec3 emit;
	float normal;
	float metalness;
	float roughness;
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

const vec3 lightNormal = vec3(0.0, -0.707, 0.707);
const vec3 lightColor = vec3(1.0, 0.7, 0.4);

float sqr(float a) {
	return a * a;
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

const float sssWrap = 0.6;
// TODO: Make this a material property
const float sssAmount = 0.25;

void main() {
	ObjectInfo info = objectBuffer.objects[baseInstance];
	
	vec4 albedo = texture(texSampler[info.material.texAlbedo], texCoord) * info.material.color;
	vec3 emit = texture(texSampler[info.material.texEmit], texCoord).rgb * info.material.emit;
	vec3 normal = texture(texSampler[info.material.texNormal], texCoord).xyz * 2.0 - 1.0;
	float metalness = texture(texSampler[info.material.texMetalness], texCoord).x * info.material.metalness;
	float roughness = texture(texSampler[info.material.texRoughness], texCoord).x * info.material.roughness;
	roughness = sqr(roughness);
	
	vec3 surfaceNormal = normalize(inNormal);
	vec3 surfaceTangent = normalize(inTangent);
	vec3 surfaceBitangent = normalize(inBitangent);
	mat3 invTBN = transpose(mat3(surfaceTangent, surfaceBitangent, surfaceNormal));
	normal = normalize(mix(surfaceNormal, normal * invTBN, info.material.normal));
	
	vec3 viewNormal = normalize(worldInfo.eyePos - inWorldPos);
	vec3 halfNormal = normalize(viewNormal + lightNormal);
	float cosThetaView = max(dot(normal, viewNormal), 0.0);
	float cosThetaLight = dot(normal, lightNormal);
	float cosThetaViewHalfNormal = max(dot(normal, halfNormal), 0.0);
	// Roughness remapping for direct lighting
	float k = sqr(roughness+1.0)/8.0;
	
	vec3 baseReflectivity = mix(vec3(0.04), albedo.rgb, metalness);
	float attenuationGeometry = GeometrySchlickGGX(cosThetaView, k);
	float attenuationLight = GeometrySchlickGGX(max(cosThetaLight, 0.0), k);
	float attenuationWrap = GeometrySchlickGGX(cosThetaLight, k);
	vec3 fresnel = FresnelSchlick(cosThetaView, baseReflectivity);
	
	// This is a way to determine the "pointiness" of the surface to make sharper edges diffuse the subsurface lighting more, but it's flat along the triangle
	// float sssWrap = clamp(fwidth(cosThetaLight) / length(fwidth(viewNormal)) / 5.0, 0.5, 1.0);
	
	float wrapFac = clamp((attenuationWrap + sssWrap) / (1.0 + sssWrap), 0.0, 1.0);
	vec3 diffuse = albedo.rgb * attenuationLight;
	vec3 subsurface = albedo.rgb * wrapFac * mix(vec3(1.0), vec3(1.0, 0.05, 0.025), clamp((wrapFac-attenuationWrap)/sssWrap, 0.0, 1.0));
	diffuse = mix(diffuse, subsurface, sssAmount);
	
	vec3 specular = lightColor * DistributionGGX(cosThetaViewHalfNormal, roughness) * attenuationLight;
	
	// outColor.rgb = normal * 0.5 + 0.5;
	// outColor.rgb = diffuse;
	// outColor.rgb = FresnelSchlick(surfaceNormal, viewNormal, baseReflectivity);
	outColor.rgb = 1.0 / PI * mix(diffuse * (1.0 - metalness), specular, fresnel) * attenuationGeometry;
	outColor.a = albedo.a;
	outColor.rgb += emit;
}
