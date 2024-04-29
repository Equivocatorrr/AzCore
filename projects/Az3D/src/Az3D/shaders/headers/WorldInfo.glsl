#ifndef WORLD_INFO_GLSL
#define WORLD_INFO_GLSL

#include "Constants.glsl"

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

float GetZFromDepth(float depth)
{
	return worldInfo.proj[2][3] / (depth - worldInfo.proj[2][2]);
}

const float sunRadiusDegrees = 0.6/2.0;
const float sunRadiusRadians = PI * sunRadiusDegrees / 180.0;
const float sunTanRadius = tan(sunRadiusRadians);
const vec3 sunLightColor = vec3(1.0, 0.9, 0.8) * 4.0;

#endif // WORLD_INFO_GLSL