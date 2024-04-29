#ifndef BONES_BUFFER_GLSL
#define BONES_BUFFER_GLSL

layout(std140, set=0, binding=2) readonly buffer BonesBuffer {
	mat4 bones[];
} bonesBuffer;

#endif // BONES_BUFFER_GLSL