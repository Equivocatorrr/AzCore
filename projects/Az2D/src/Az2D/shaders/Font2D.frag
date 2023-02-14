#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D texSampler[1];

struct Material {
	vec4 color;
	float emitStrength;
	float normalDepth;
};

struct TexIndices {
	int albedo;
	int normal;
	int emit;
};

layout(push_constant) uniform pushConstants {
	layout(offset = 48) Material mat;
	layout(offset = 72) TexIndices tex;
	layout(offset = 84) float edge;
	layout(offset = 88) float bounds;
} pc;

void main() {
	float alpha = smoothstep(pc.bounds-pc.edge, pc.bounds+pc.edge, texture(texSampler[pc.tex.albedo],texCoord).r);
	outColor = pc.mat.color;
	outColor.a *= alpha;
	outColor.rgb *= outColor.a;
}
