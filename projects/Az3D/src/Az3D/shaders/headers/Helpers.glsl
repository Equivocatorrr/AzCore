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

float linstep(float edge0, float edge1, float x) {
	return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
}

#endif // HELPERS_GLSL