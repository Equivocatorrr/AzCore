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
	vec2 size = textureSize(texSampler[pc.texId], 0);
	vec2 pixelPos = texCoord*size;
	vec2 pixel = floor(pixelPos - 0.5) + 1.0;
	vec2 subPixel = fract(pixelPos - 0.5) - 0.5;
	vec2 sharpTexCoord = (pixel + clamp(subPixel / fwidth(pixelPos), -0.5, 0.5)) / size;
	outColor = texture(texSampler[pc.texId], sharpTexCoord) * pc.mat.color;
	outColor.rgb *= pc.mat.color.a;
}
