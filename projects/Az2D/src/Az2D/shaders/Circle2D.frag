#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=1) uniform sampler2D texSampler[1];

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
} pc;

float square(float x) {
	return x * x;
}

void main() {
	vec4 texColor = texture(texSampler[pc.tex.albedo], texCoord);
	outColor = texColor * pc.mat.color;
	float edge = square(texCoord.x-0.5) + square(texCoord.y-0.5);
	outColor *= smoothstep(0.25, 0.25 - pc.edge, edge);
	outColor.rgb *= pc.mat.color.a;
}
