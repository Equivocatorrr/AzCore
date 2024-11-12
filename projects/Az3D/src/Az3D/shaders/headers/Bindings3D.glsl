#ifndef BINDINGS_3D_GLSL
#define BINDINGS_3D_GLSL

layout(set=0, binding=3) uniform sampler2D texSampler[1];
layout(set=0, binding=4) uniform sampler2D aoSampler;
layout(set=0, binding=5) uniform sampler2D shadowMap;

#endif // BINDINGS_3D_GLSL