#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inColor;
layout(location=1) in vec2 inPosition;
layout(location=2) in float inDepth;

layout(location=0) out vec4 outColor;
layout(location=1) out float outDepth;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    outColor = inColor;
    outDepth = inDepth;
}
