#ifndef FONT_3D_GLSL
#define FONT_3D_GLSL

#include "Helpers.glsl"
#include "headers/WorldInfo.glsl"

const float atlasEdgeDistance = 0.12;
const float emPixels = 64.0;
const float edgeFactor = 1.0 / (emPixels * atlasEdgeDistance * 4.0);

float SDF(float edge, float value) {
	return linstep(0.5 - edge, 0.5 + edge, value);
}

void Font3DGetGeometry(out vec3 normal, out vec3 tangent, out vec3 bitangent, out float alpha) {
	vec2 texSize = textureSize(texSampler[inTexAtlas], 0);
	vec2 edges = texSize * fwidth(inTexCoord) * edgeFactor;
	float edge = mix(edges.x, edges.y, 0.5);
	float sdf = texture(texSampler[inTexAtlas], inTexCoord).r;
	alpha = SDF(edge, sdf);

	vec3 forward = normalize(inWorldPos - worldInfo.eyePos);
	vec2 parallax = vec2(dot(inTangent, forward), dot(inBitangent, forward));
	float sdf2 = texture(texSampler[inTexAtlas], inTexCoord + parallax / (texSize * edgeFactor * 10.0)).r;
	float backAlpha = SDF(edge, sdf2);

	vec2 edgeDirection = normalizeFallback(vec2(-dFdx(sdf), dFdy(sdf)), 1.0e-12, vec2(1.0, 0.0));
	float backFaceFac = -float(gl_FrontFacing) * 2.0 + 1.0;
	edgeDirection.y *= backFaceFac;
	float cornerAlpha = SDF(edge * 2.0, sdf);
	normal = normalize(mix(inTangent * edgeDirection.x + inBitangent * edgeDirection.y, inNormal, cornerAlpha));
	tangent = normalize(mix((inNormal * edgeDirection.x + inTangent * edgeDirection.y) * backFaceFac, inTangent, cornerAlpha));
	bitangent = normalize(mix(cross(normal, tangent), inBitangent, cornerAlpha));
	alpha = max(alpha, backAlpha);
}

#endif // FONT_3D_GLSL