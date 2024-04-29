#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location=0) in vec2 texCoord;
layout(location=1) flat in int inInstanceIndex;
layout(location=2) centroid in float inDepth;

layout(location=0) out vec2 outColor;

#include "headers/CommonFrag.glsl"
#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"

void main() {
	ObjectInfo info = objectBuffer.objects[inInstanceIndex];

	// float alpha = texture(texSampler[info.material.texAlbedo], texCoord).a * info.material.color.a;
	// if (alpha < 0.5) {
	// 	discard;
	// }
	outColor.x = inDepth;
	outColor.y = inDepth*inDepth;
}
