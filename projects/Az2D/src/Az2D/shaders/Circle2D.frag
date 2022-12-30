#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=1) uniform sampler2D texSampler[1];

layout(push_constant) uniform pushConstants {
	layout(offset = 32) vec4 color;
	layout(offset = 48) int texId;
	layout(offset = 52) float edge;
} pc;

float square(float x) {
	return x * x;
}

void main() {
	outColor = texture(texSampler[pc.texId], texCoord) * pc.color;
	float edge = square(texCoord.x-0.5) + square(texCoord.y-0.5);
	outColor.a *= smoothstep(0.25, 0.25 - pc.edge, edge);
}
