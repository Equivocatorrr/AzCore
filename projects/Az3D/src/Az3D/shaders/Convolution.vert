#version 460
#extension GL_ARB_separate_shader_objects : enable

const vec4 vertices[4] = vec4[4](
	vec4(-1.0, -1.0, 0.0, 1.0),
	vec4( 1.0, -1.0, 0.0, 1.0),
	vec4( 1.0,  1.0, 0.0, 1.0),
	vec4(-1.0,  1.0, 0.0, 1.0)
);

const vec2 uvs[4] = vec2[4](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
);

layout(location=0) out vec2 outUV;

void main() {
	gl_Position = vertices[gl_VertexIndex];
	outUV = uvs[gl_VertexIndex];
}
