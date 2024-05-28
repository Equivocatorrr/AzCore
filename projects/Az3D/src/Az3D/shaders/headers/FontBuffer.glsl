#ifndef FONT_BUFFER_GLSL
#define FONT_BUFFER_GLSL

struct GlyphInfo {
	// Top left and bottom right uv coordinates in the atlas
	vec2 uvs[2];
	// Top left and bottom right offset from cursor
	vec2 offsets[2];
};

layout(scalar, set=0, binding=5) uniform FontBuffer {
	uint texAtlas;
	uint _pad; // Force alignment of glyphs to an 8-byte boundary even when using scalarBlockLayout
	GlyphInfo glyphs[1];
} fontBuffer[1];

struct TextInfo {
	mat2 glyphTransforms[36];
	vec2 glyphOffsets[36];
	uint glyphIndices[36];
	uint fontIndex;
	uint objectIndex;
	// probably unnecessary explicit padding
	uint _pad[2];
};
// 36 * 7 + 2 = 254
layout(std430, set=0, binding=6) readonly buffer TextBuffer {
	TextInfo texts[];
} textBuffer;

#endif // FONT_BUFFER_GLSL