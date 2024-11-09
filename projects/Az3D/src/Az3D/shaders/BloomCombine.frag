#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D image;

void main() {
	outColor = texture(image, inTexCoord);
	outColor.a = 0.5;
}
