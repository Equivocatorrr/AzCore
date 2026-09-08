#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inColor;

layout(location=0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
	float alpha;
} constants;

void main() {
	outColor = inColor;
	outColor.rgb *= inColor.a;
	outColor *= constants.alpha;
}
