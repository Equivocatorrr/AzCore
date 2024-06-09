#ifndef HELPERS_GLSL
#define HELPERS_GLSL

float sqr(float a) {
	return a * a;
}

float normSqr(vec2 a) {
	return dot(a, a);
}

float normSqr(vec3 a) {
	return dot(a, a);
}

vec3 sqr(vec3 a) {
	return a * a;
}

vec2 normalizeFallback(vec2 vector, float epsilon, vec2 fallback) {
	float len = length(vector);
	if (len < epsilon) return fallback;
	return vector / len;
}

#endif // HELPERS_GLSL