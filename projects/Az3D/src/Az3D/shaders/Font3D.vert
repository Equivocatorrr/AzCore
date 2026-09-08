#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location=0) out vec2 outTexCoord;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec3 outTangent;
layout(location=3) out vec3 outBitangent;
layout(location=4) out uint outObjectIndex;
layout(location=5) out vec3 outWorldPos;
layout(location=6) out vec4 outProjPos;
layout(location=7) out uint outTexAtlas;

#include "headers/Helpers.glsl"
#include "headers/WorldInfo.glsl"
#include "headers/ObjectBuffer.glsl"
#include "headers/FontBuffer.glsl"

/*
0 1
2
  4
3 5
*/

const uint xIndex[6] = uint[6](
	0, 1, 0, 0, 1, 1
);
const uint yIndex[6] = uint[6](
	0, 0, 1, 1, 0, 1
);

void main() {
	TextInfo textInfo = textBuffer.texts[gl_InstanceIndex];
	mat4 model = objectBuffer.objects[textInfo.objectIndex].model;
	vec2 glyphOffset = textInfo.glyphOffsets[gl_VertexIndex / 6];
	uint glyphIndex = textInfo.glyphIndices[gl_VertexIndex / 6];
	GlyphInfo glyph = fontBuffer[textInfo.fontIndex].glyphs[glyphIndex];
	vec3 position = vec3(
		glyph.offsets[xIndex[gl_VertexIndex % 6]].x,
		glyph.offsets[yIndex[gl_VertexIndex % 6]].y,
		0.0
	);
	position.xy += glyphOffset;
	vec4 worldPos = model * vec4(position, 1.0);
	gl_Position = worldInfo.viewProj * worldPos;
	outTexCoord = vec2(
		glyph.uvs[xIndex[gl_VertexIndex % 6]].x,
		glyph.uvs[yIndex[gl_VertexIndex % 6]].y
	);
	// Z-basis
	outNormal = -normalize(model[2].xyz);
	// X-basis
	outTangent = normalize(model[0].xyz);
	// Y-basis
	outBitangent = normalize(model[1].xyz);
	outObjectIndex = textInfo.objectIndex;
	outWorldPos = worldPos.xyz;
	outProjPos = gl_Position;
	outTexAtlas = fontBuffer[textInfo.fontIndex].texAtlas;
}
