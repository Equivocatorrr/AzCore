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

layout(push_constant) uniform pushConstants {
	layout(offset = 48) Material mat;
	layout(offset = 72) int texId;
} pc;

void main() {
	vec4 texColor = texture(texSampler[pc.texId], texCoord);
	outColor = texColor * pc.mat.color;
	outColor.rgb *= pc.mat.color.a;
}
