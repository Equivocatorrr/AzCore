#ifndef HELPERS_GLSL
#define HELPERS_GLSL

float sqr(float a) {
	return a * a;
}

float normSqr(vec3 a) {
	return dot(a, a);
}

vec3 sqr(vec3 a) {
	return a * a;
}

#endif // HELPERS_GLSL