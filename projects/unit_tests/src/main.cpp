/*
	File: unit_tests.cpp
	Author: Philip Haynes
	Does exactly as advertized.
*/

#include "AzCore/IO/LogStream.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/math.hpp"

using namespace AzCore;

void MyAssert(bool condition, const char *messageOnFailure) {
	if (!condition) {
		throw std::runtime_error(messageOnFailure);
	}
}

bool equals(f32 a, f32 b) {
	const f32 epsilon = 0.000001f;
	const f32 epsilonFac[2] = { 1.0f + epsilon, 1.0f - epsilon };
	if (a == b) {
		return true;
	}
	return (a*epsilonFac[a<0] + epsilon >= b && a*epsilonFac[a>0] - epsilon <= b);
}

template<typename T>
bool equals(T a, T b) {
	return a == b;
}

bool equals(Angle32 a, Angle32 b) {
	const f32 epsilon = 0.00001f;
	if (a == b) {
		return true;
	}
	return (a-b <= epsilon);
}

inline String ToString(String a) {
	return a;
}

inline String ToString(const char* a) {
	return String(a);
}

inline String ToString(Angle32 angle) {
	return ToString(angle.value());
}

template<typename T>
void ExpectedValue(T a, T b, const char *messageOnFailure) {
	if (!equals(a, b)) {
		String err(messageOnFailure);
		err += " Actual value " + ToString(a) + " != " + ToString(b);
		throw std::runtime_error(err.data);
	}
}

// Why the fuck does it take two separate macros just to turn __LINE__ into a C string???
#define STR2(a) #a
#define STR(a) STR2(a)
#define ASSERT(condition) \
	MyAssert((condition), "Assertion failed: " #condition " in file: " __FILE__ " on line: " STR(__LINE__) )

#define EXPECTEDVALUE(a, b) \
	ExpectedValue(a, b, "Expected value " #a " to be equal to " #b " in file: " __FILE__ " on line: " STR(__LINE__) )

void Print(vec3 v, io::LogStream& cout) {
	cout << "{";
	for (u32 i = 0; i < 3; i++) {
		if (v[i] >= 0.0f)
			cout << " ";
		if (v[i] < 10.0f)
			cout << " ";
		if (v[i] < 100.0f)
			cout << " ";
		cout << v[i];
		if (i != 2)
			cout << ", ";
	}
	cout << "}";
}

void Print(mat3 m, io::LogStream& cout) {
	cout << "[" << std::fixed << std::setprecision(3);
	Print(m.Row1(), cout);
	cout << "\n ";
	Print(m.Row2(), cout);
	cout << "\n ";
	Print(m.Row3(), cout);
	cout << "]" << std::endl;
}

void Print(vec4 v, io::LogStream& cout) {
	cout << "{";
	for (u32 i = 0; i < 4; i++) {
		if (v[i] >= 0.0f)
			cout << " ";
		if (v[i] < 10.0f)
			cout << " ";
		if (v[i] < 100.0f)
			cout << " ";
		cout << v[i];
		if (i != 3)
			cout << ", ";
	}
	cout << "}";
}

void Print(mat4 m, io::LogStream& cout) {
	cout << "[" << std::fixed << std::setprecision(3);
	Print(m.Row1(), cout);
	cout << "\n ";
	Print(m.Row2(), cout);
	cout << "\n ";
	Print(m.Row3(), cout);
	cout << "\n ";
	Print(m.Row4(), cout);
	cout << "]" << std::endl;
}

bool UnitTestMat2(io::LogStream& cout) {
	cout << "Unit testing mat2...\n";
	mat2 matrix;
	matrix = mat2::Identity();
	try {
		EXPECTEDVALUE(matrix.data[0], 1.0f);
		EXPECTEDVALUE(matrix.data[1], 0.0f);
		EXPECTEDVALUE(matrix.data[2], 0.0f);
		EXPECTEDVALUE(matrix.data[3], 1.0f);
		matrix = mat2(
			1.0f, 2.0f,
			3.0f, 4.0f
		);
		vec2 rows[2] = {
			matrix.Row1(),
			matrix.Row2()
		};
		EXPECTEDVALUE(rows[0].x, 1.0f);
		EXPECTEDVALUE(rows[0].y, 2.0f);
		EXPECTEDVALUE(rows[1].x, 3.0f);
		EXPECTEDVALUE(rows[1].y, 4.0f);
		vec2 columns[2] = {
			matrix.Col1(),
			matrix.Col2()
		};
		EXPECTEDVALUE(columns[0].x, 1.0f);
		EXPECTEDVALUE(columns[0].y, 3.0f);
		EXPECTEDVALUE(columns[1].x, 2.0f);
		EXPECTEDVALUE(columns[1].y, 4.0f);
		matrix = mat2::Rotation(halfpi) * matrix;
		EXPECTEDVALUE(matrix.h.x1,-3.0f);
		EXPECTEDVALUE(matrix.h.y1,-4.0f);
		EXPECTEDVALUE(matrix.h.x2, 1.0f);
		EXPECTEDVALUE(matrix.h.y2, 2.0f);
		matrix = mat2::Scaler({2.0f, 3.0f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1,-6.0f);
		EXPECTEDVALUE(matrix.h.y1,-8.0f);
		EXPECTEDVALUE(matrix.h.x2, 3.0f);
		EXPECTEDVALUE(matrix.h.y2, 6.0f);
		matrix = matrix.Transpose();
		EXPECTEDVALUE(matrix.v.x1,-6.0f);
		EXPECTEDVALUE(matrix.v.y1,-8.0f);
		EXPECTEDVALUE(matrix.v.x2, 3.0f);
		EXPECTEDVALUE(matrix.v.y2, 6.0f);
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestMat3(io::LogStream& cout) {
	cout << "Unit testing mat3...\n";
	mat3 matrix;
	matrix = mat3::Identity();
	try {
		EXPECTEDVALUE(matrix.data[0], 1.0f);
		EXPECTEDVALUE(matrix.data[1], 0.0f);
		EXPECTEDVALUE(matrix.data[2], 0.0f);
		EXPECTEDVALUE(matrix.data[3], 0.0f);
		EXPECTEDVALUE(matrix.data[4], 1.0f);
		EXPECTEDVALUE(matrix.data[5], 0.0f);
		EXPECTEDVALUE(matrix.data[6], 0.0f);
		EXPECTEDVALUE(matrix.data[7], 0.0f);
		EXPECTEDVALUE(matrix.data[8], 1.0f);
		matrix = mat3(
			1.0f, 2.0f, 3.0f,
			4.0f, 5.0f, 6.0f,
			7.0f, 8.0f, 9.0f
		);
		vec3 rows[3] = {
			matrix.Row1(),
			matrix.Row2(),
			matrix.Row3()
		};
		EXPECTEDVALUE(rows[0].x, 1.0f);
		EXPECTEDVALUE(rows[0].y, 2.0f);
		EXPECTEDVALUE(rows[0].z, 3.0f);
		EXPECTEDVALUE(rows[1].x, 4.0f);
		EXPECTEDVALUE(rows[1].y, 5.0f);
		EXPECTEDVALUE(rows[1].z, 6.0f);
		EXPECTEDVALUE(rows[2].x, 7.0f);
		EXPECTEDVALUE(rows[2].y, 8.0f);
		EXPECTEDVALUE(rows[2].z, 9.0f);
		vec3 columns[3] = {
			matrix.Col1(),
			matrix.Col2(),
			matrix.Col3()
		};
		EXPECTEDVALUE(columns[0].x, 1.0f);
		EXPECTEDVALUE(columns[0].y, 4.0f);
		EXPECTEDVALUE(columns[0].z, 7.0f);
		EXPECTEDVALUE(columns[1].x, 2.0f);
		EXPECTEDVALUE(columns[1].y, 5.0f);
		EXPECTEDVALUE(columns[1].z, 8.0f);
		EXPECTEDVALUE(columns[2].x, 3.0f);
		EXPECTEDVALUE(columns[2].y, 6.0f);
		EXPECTEDVALUE(columns[2].z, 9.0f);
		matrix = mat3::RotationBasic(halfpi, Axis::X) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 1.0f);
		EXPECTEDVALUE(matrix.h.y1, 2.0f);
		EXPECTEDVALUE(matrix.h.z1, 3.0f);
		EXPECTEDVALUE(matrix.h.x2, -7.0f);
		EXPECTEDVALUE(matrix.h.y2, -8.0f);
		EXPECTEDVALUE(matrix.h.z2, -9.0f);
		EXPECTEDVALUE(matrix.h.x3, 4.0f);
		EXPECTEDVALUE(matrix.h.y3, 5.0f);
		EXPECTEDVALUE(matrix.h.z3, 6.0f);
		matrix = mat3::RotationBasic(halfpi, Axis::Y) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 4.0f);
		EXPECTEDVALUE(matrix.h.y1, 5.0f);
		EXPECTEDVALUE(matrix.h.z1, 6.0f);
		EXPECTEDVALUE(matrix.h.x2, -7.0f);
		EXPECTEDVALUE(matrix.h.y2, -8.0f);
		EXPECTEDVALUE(matrix.h.z2, -9.0f);
		EXPECTEDVALUE(matrix.h.x3, -1.0f);
		EXPECTEDVALUE(matrix.h.y3, -2.0f);
		EXPECTEDVALUE(matrix.h.z3, -3.0f);
		matrix = mat3::RotationBasic(halfpi, Axis::Z) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 7.0f);
		EXPECTEDVALUE(matrix.h.y1, 8.0f);
		EXPECTEDVALUE(matrix.h.z1, 9.0f);
		EXPECTEDVALUE(matrix.h.x2, 4.0f);
		EXPECTEDVALUE(matrix.h.y2, 5.0f);
		EXPECTEDVALUE(matrix.h.z2, 6.0f);
		EXPECTEDVALUE(matrix.h.x3, -1.0f);
		EXPECTEDVALUE(matrix.h.y3, -2.0f);
		EXPECTEDVALUE(matrix.h.z3, -3.0f);
		matrix = mat3::Rotation(pi, {sin(halfpi), cos(halfpi), 0.0f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 7.0f);
		EXPECTEDVALUE(matrix.h.y1, 8.0f);
		EXPECTEDVALUE(matrix.h.z1, 9.0f);
		EXPECTEDVALUE(matrix.h.x2, -4.0f);
		EXPECTEDVALUE(matrix.h.y2, -5.0f);
		EXPECTEDVALUE(matrix.h.z2, -6.0f);
		EXPECTEDVALUE(matrix.h.x3, 1.0f);
		EXPECTEDVALUE(matrix.h.y3, 2.0f);
		EXPECTEDVALUE(matrix.h.z3, 3.0f);
		matrix = mat3::Rotation(halfpi, {0.0f, 0.0f, 1.0f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 4.0f);
		EXPECTEDVALUE(matrix.h.y1, 5.0f);
		EXPECTEDVALUE(matrix.h.z1, 6.0f);
		EXPECTEDVALUE(matrix.h.x2, 7.0f);
		EXPECTEDVALUE(matrix.h.y2, 8.0f);
		EXPECTEDVALUE(matrix.h.z2, 9.0f);
		EXPECTEDVALUE(matrix.h.x3, 1.0f);
		EXPECTEDVALUE(matrix.h.y3, 2.0f);
		EXPECTEDVALUE(matrix.h.z3, 3.0f);
		matrix = mat3::Scaler({2.0f, 3.0f, 3.5f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 8.0f);
		EXPECTEDVALUE(matrix.h.y1, 10.0f);
		EXPECTEDVALUE(matrix.h.z1, 12.0f);
		EXPECTEDVALUE(matrix.h.x2, 21.0f);
		EXPECTEDVALUE(matrix.h.y2, 24.0f);
		EXPECTEDVALUE(matrix.h.z2, 27.0f);
		EXPECTEDVALUE(matrix.h.x3, 3.5f);
		EXPECTEDVALUE(matrix.h.y3, 7.0f);
		EXPECTEDVALUE(matrix.h.z3, 10.5f);
		matrix = matrix.Transpose();
		EXPECTEDVALUE(matrix.v.x1, 8.0f);
		EXPECTEDVALUE(matrix.v.y1, 10.0f);
		EXPECTEDVALUE(matrix.v.z1, 12.0f);
		EXPECTEDVALUE(matrix.v.x2, 21.0f);
		EXPECTEDVALUE(matrix.v.y2, 24.0f);
		EXPECTEDVALUE(matrix.v.z2, 27.0f);
		EXPECTEDVALUE(matrix.v.x3, 3.5f);
		EXPECTEDVALUE(matrix.v.y3, 7.0f);
		EXPECTEDVALUE(matrix.v.z3, 10.5f);
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestMat4(io::LogStream& cout) {
	cout << "Unit testing mat4\n";
	mat4 matrix;
	matrix = mat4::Identity();
	try {
		EXPECTEDVALUE(matrix.data[ 0], 1.0f);
		EXPECTEDVALUE(matrix.data[ 1], 0.0f);
		EXPECTEDVALUE(matrix.data[ 2], 0.0f);
		EXPECTEDVALUE(matrix.data[ 3], 0.0f);
		EXPECTEDVALUE(matrix.data[ 4], 0.0f);
		EXPECTEDVALUE(matrix.data[ 5], 1.0f);
		EXPECTEDVALUE(matrix.data[ 6], 0.0f);
		EXPECTEDVALUE(matrix.data[ 7], 0.0f);
		EXPECTEDVALUE(matrix.data[ 8], 0.0f);
		EXPECTEDVALUE(matrix.data[ 9], 0.0f);
		EXPECTEDVALUE(matrix.data[10], 1.0f);
		EXPECTEDVALUE(matrix.data[11], 0.0f);
		EXPECTEDVALUE(matrix.data[12], 0.0f);
		EXPECTEDVALUE(matrix.data[13], 0.0f);
		EXPECTEDVALUE(matrix.data[14], 0.0f);
		EXPECTEDVALUE(matrix.data[15], 1.0f);
		matrix = mat4(
			 1.0f,  2.0f,  3.0f,  4.0f,
			 5.0f,  6.0f,  7.0f,  8.0f,
			 9.0f, 10.0f, 11.0f, 12.0f,
			13.0f, 14.0f, 15.0f, 16.0f
		);
		vec4 rows[4] = {
			matrix.Row1(),
			matrix.Row2(),
			matrix.Row3(),
			matrix.Row4()
		};
		EXPECTEDVALUE(rows[0].x,  1.0f);
		EXPECTEDVALUE(rows[0].y,  2.0f);
		EXPECTEDVALUE(rows[0].z,  3.0f);
		EXPECTEDVALUE(rows[0].w,  4.0f);
		EXPECTEDVALUE(rows[1].x,  5.0f);
		EXPECTEDVALUE(rows[1].y,  6.0f);
		EXPECTEDVALUE(rows[1].z,  7.0f);
		EXPECTEDVALUE(rows[1].w,  8.0f);
		EXPECTEDVALUE(rows[2].x,  9.0f);
		EXPECTEDVALUE(rows[2].y, 10.0f);
		EXPECTEDVALUE(rows[2].z, 11.0f);
		EXPECTEDVALUE(rows[2].w, 12.0f);
		EXPECTEDVALUE(rows[3].x, 13.0f);
		EXPECTEDVALUE(rows[3].y, 14.0f);
		EXPECTEDVALUE(rows[3].z, 15.0f);
		EXPECTEDVALUE(rows[3].w, 16.0f);
		vec4 columns[4] = {
			matrix.Col1(),
			matrix.Col2(),
			matrix.Col3(),
			matrix.Col4()
		};
		EXPECTEDVALUE(columns[0].x,  1.0f);
		EXPECTEDVALUE(columns[0].y,  5.0f);
		EXPECTEDVALUE(columns[0].z,  9.0f);
		EXPECTEDVALUE(columns[0].w, 13.0f);
		EXPECTEDVALUE(columns[1].x,  2.0f);
		EXPECTEDVALUE(columns[1].y,  6.0f);
		EXPECTEDVALUE(columns[1].z, 10.0f);
		EXPECTEDVALUE(columns[1].w, 14.0f);
		EXPECTEDVALUE(columns[2].x,  3.0f);
		EXPECTEDVALUE(columns[2].y,  7.0f);
		EXPECTEDVALUE(columns[2].z, 11.0f);
		EXPECTEDVALUE(columns[2].w, 15.0f);
		EXPECTEDVALUE(columns[3].x,  4.0f);
		EXPECTEDVALUE(columns[3].y,  8.0f);
		EXPECTEDVALUE(columns[3].z, 12.0f);
		EXPECTEDVALUE(columns[3].w, 16.0f);
		matrix = mat4::RotationBasic(halfpi, Plane::XW) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  1.0f);
		EXPECTEDVALUE(matrix.h.y1,  2.0f);
		EXPECTEDVALUE(matrix.h.z1,  3.0f);
		EXPECTEDVALUE(matrix.h.w1,  4.0f);
		EXPECTEDVALUE(matrix.h.x2, -9.0f);
		EXPECTEDVALUE(matrix.h.y2,-10.0f);
		EXPECTEDVALUE(matrix.h.z2,-11.0f);
		EXPECTEDVALUE(matrix.h.w2,-12.0f);
		EXPECTEDVALUE(matrix.h.x3,  5.0f);
		EXPECTEDVALUE(matrix.h.y3,  6.0f);
		EXPECTEDVALUE(matrix.h.z3,  7.0f);
		EXPECTEDVALUE(matrix.h.w3,  8.0f);
		EXPECTEDVALUE(matrix.h.x4, 13.0f);
		EXPECTEDVALUE(matrix.h.y4, 14.0f);
		EXPECTEDVALUE(matrix.h.z4, 15.0f);
		EXPECTEDVALUE(matrix.h.w4, 16.0f);
		matrix = mat4::RotationBasic(halfpi, Plane::YW) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  5.0f);
		EXPECTEDVALUE(matrix.h.y1,  6.0f);
		EXPECTEDVALUE(matrix.h.z1,  7.0f);
		EXPECTEDVALUE(matrix.h.w1,  8.0f);
		EXPECTEDVALUE(matrix.h.x2, -9.0f);
		EXPECTEDVALUE(matrix.h.y2,-10.0f);
		EXPECTEDVALUE(matrix.h.z2,-11.0f);
		EXPECTEDVALUE(matrix.h.w2,-12.0f);
		EXPECTEDVALUE(matrix.h.x3, -1.0f);
		EXPECTEDVALUE(matrix.h.y3, -2.0f);
		EXPECTEDVALUE(matrix.h.z3, -3.0f);
		EXPECTEDVALUE(matrix.h.w3, -4.0f);
		EXPECTEDVALUE(matrix.h.x4, 13.0f);
		EXPECTEDVALUE(matrix.h.y4, 14.0f);
		EXPECTEDVALUE(matrix.h.z4, 15.0f);
		EXPECTEDVALUE(matrix.h.w4, 16.0f);
		matrix = mat4::RotationBasic(halfpi, Plane::ZW) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  9.0f);
		EXPECTEDVALUE(matrix.h.y1, 10.0f);
		EXPECTEDVALUE(matrix.h.z1, 11.0f);
		EXPECTEDVALUE(matrix.h.w1, 12.0f);
		EXPECTEDVALUE(matrix.h.x2,  5.0f);
		EXPECTEDVALUE(matrix.h.y2,  6.0f);
		EXPECTEDVALUE(matrix.h.z2,  7.0f);
		EXPECTEDVALUE(matrix.h.w2,  8.0f);
		EXPECTEDVALUE(matrix.h.x3, -1.0f);
		EXPECTEDVALUE(matrix.h.y3, -2.0f);
		EXPECTEDVALUE(matrix.h.z3, -3.0f);
		EXPECTEDVALUE(matrix.h.w3, -4.0f);
		EXPECTEDVALUE(matrix.h.x4, 13.0f);
		EXPECTEDVALUE(matrix.h.y4, 14.0f);
		EXPECTEDVALUE(matrix.h.z4, 15.0f);
		EXPECTEDVALUE(matrix.h.w4, 16.0f);
		matrix = mat4::RotationBasic(halfpi, Plane::XY) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  9.0f);
		EXPECTEDVALUE(matrix.h.y1, 10.0f);
		EXPECTEDVALUE(matrix.h.z1, 11.0f);
		EXPECTEDVALUE(matrix.h.w1, 12.0f);
		EXPECTEDVALUE(matrix.h.x2,  5.0f);
		EXPECTEDVALUE(matrix.h.y2,  6.0f);
		EXPECTEDVALUE(matrix.h.z2,  7.0f);
		EXPECTEDVALUE(matrix.h.w2,  8.0f);
		EXPECTEDVALUE(matrix.h.x3,-13.0f);
		EXPECTEDVALUE(matrix.h.y3,-14.0f);
		EXPECTEDVALUE(matrix.h.z3,-15.0f);
		EXPECTEDVALUE(matrix.h.w3,-16.0f);
		EXPECTEDVALUE(matrix.h.x4, -1.0f);
		EXPECTEDVALUE(matrix.h.y4, -2.0f);
		EXPECTEDVALUE(matrix.h.z4, -3.0f);
		EXPECTEDVALUE(matrix.h.w4, -4.0f);
		matrix = mat4::RotationBasic(halfpi, Plane::YZ) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  1.0f);
		EXPECTEDVALUE(matrix.h.y1,  2.0f);
		EXPECTEDVALUE(matrix.h.z1,  3.0f);
		EXPECTEDVALUE(matrix.h.w1,  4.0f);
		EXPECTEDVALUE(matrix.h.x2,  5.0f);
		EXPECTEDVALUE(matrix.h.y2,  6.0f);
		EXPECTEDVALUE(matrix.h.z2,  7.0f);
		EXPECTEDVALUE(matrix.h.w2,  8.0f);
		EXPECTEDVALUE(matrix.h.x3,-13.0f);
		EXPECTEDVALUE(matrix.h.y3,-14.0f);
		EXPECTEDVALUE(matrix.h.z3,-15.0f);
		EXPECTEDVALUE(matrix.h.w3,-16.0f);
		EXPECTEDVALUE(matrix.h.x4,  9.0f);
		EXPECTEDVALUE(matrix.h.y4, 10.0f);
		EXPECTEDVALUE(matrix.h.z4, 11.0f);
		EXPECTEDVALUE(matrix.h.w4, 12.0f);
		matrix = mat4::RotationBasic(halfpi, Plane::ZX) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  1.0f);
		EXPECTEDVALUE(matrix.h.y1,  2.0f);
		EXPECTEDVALUE(matrix.h.z1,  3.0f);
		EXPECTEDVALUE(matrix.h.w1,  4.0f);
		EXPECTEDVALUE(matrix.h.x2,  9.0f);
		EXPECTEDVALUE(matrix.h.y2, 10.0f);
		EXPECTEDVALUE(matrix.h.z2, 11.0f);
		EXPECTEDVALUE(matrix.h.w2, 12.0f);
		EXPECTEDVALUE(matrix.h.x3,-13.0f);
		EXPECTEDVALUE(matrix.h.y3,-14.0f);
		EXPECTEDVALUE(matrix.h.z3,-15.0f);
		EXPECTEDVALUE(matrix.h.w3,-16.0f);
		EXPECTEDVALUE(matrix.h.x4, -5.0f);
		EXPECTEDVALUE(matrix.h.y4, -6.0f);
		EXPECTEDVALUE(matrix.h.z4, -7.0f);
		EXPECTEDVALUE(matrix.h.w4, -8.0f);
		matrix = mat4::Rotation(pi, {sin(halfpi), cos(halfpi), 0.0f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  1.0f);
		EXPECTEDVALUE(matrix.h.y1,  2.0f);
		EXPECTEDVALUE(matrix.h.z1,  3.0f);
		EXPECTEDVALUE(matrix.h.w1,  4.0f);
		EXPECTEDVALUE(matrix.h.x2, -9.0f);
		EXPECTEDVALUE(matrix.h.y2,-10.0f);
		EXPECTEDVALUE(matrix.h.z2,-11.0f);
		EXPECTEDVALUE(matrix.h.w2,-12.0f);
		EXPECTEDVALUE(matrix.h.x3, 13.0f);
		EXPECTEDVALUE(matrix.h.y3, 14.0f);
		EXPECTEDVALUE(matrix.h.z3, 15.0f);
		EXPECTEDVALUE(matrix.h.w3, 16.0f);
		EXPECTEDVALUE(matrix.h.x4, -5.0f);
		EXPECTEDVALUE(matrix.h.y4, -6.0f);
		EXPECTEDVALUE(matrix.h.z4, -7.0f);
		EXPECTEDVALUE(matrix.h.w4, -8.0f);
		matrix = mat4::Rotation(halfpi, {0.0f, 0.0f, 1.0f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1,  9.0f);
		EXPECTEDVALUE(matrix.h.y1, 10.0f);
		EXPECTEDVALUE(matrix.h.z1, 11.0f);
		EXPECTEDVALUE(matrix.h.w1, 12.0f);
		EXPECTEDVALUE(matrix.h.x2,  1.0f);
		EXPECTEDVALUE(matrix.h.y2,  2.0f);
		EXPECTEDVALUE(matrix.h.z2,  3.0f);
		EXPECTEDVALUE(matrix.h.w2,  4.0f);
		EXPECTEDVALUE(matrix.h.x3, 13.0f);
		EXPECTEDVALUE(matrix.h.y3, 14.0f);
		EXPECTEDVALUE(matrix.h.z3, 15.0f);
		EXPECTEDVALUE(matrix.h.w3, 16.0f);
		EXPECTEDVALUE(matrix.h.x4, -5.0f);
		EXPECTEDVALUE(matrix.h.y4, -6.0f);
		EXPECTEDVALUE(matrix.h.z4, -7.0f);
		EXPECTEDVALUE(matrix.h.w4, -8.0f);
		matrix = mat4::Scaler({2.0f, 3.0f, 3.5f, 4.5f}) * matrix;
		EXPECTEDVALUE(matrix.h.x1, 18.0f);
		EXPECTEDVALUE(matrix.h.y1, 20.0f);
		EXPECTEDVALUE(matrix.h.z1, 22.0f);
		EXPECTEDVALUE(matrix.h.w1, 24.0f);
		EXPECTEDVALUE(matrix.h.x2,  3.0f);
		EXPECTEDVALUE(matrix.h.y2,  6.0f);
		EXPECTEDVALUE(matrix.h.z2,  9.0f);
		EXPECTEDVALUE(matrix.h.w2, 12.0f);
		EXPECTEDVALUE(matrix.h.x3, 45.5f);
		EXPECTEDVALUE(matrix.h.y3, 49.0f);
		EXPECTEDVALUE(matrix.h.z3, 52.5f);
		EXPECTEDVALUE(matrix.h.w3, 56.0f);
		EXPECTEDVALUE(matrix.h.x4,-22.5f);
		EXPECTEDVALUE(matrix.h.y4,-27.0f);
		EXPECTEDVALUE(matrix.h.z4,-31.5f);
		EXPECTEDVALUE(matrix.h.w4,-36.0f);
		matrix = matrix.Transpose();
		EXPECTEDVALUE(matrix.v.x1, 18.0f);
		EXPECTEDVALUE(matrix.v.y1, 20.0f);
		EXPECTEDVALUE(matrix.v.z1, 22.0f);
		EXPECTEDVALUE(matrix.v.w1, 24.0f);
		EXPECTEDVALUE(matrix.v.x2,  3.0f);
		EXPECTEDVALUE(matrix.v.y2,  6.0f);
		EXPECTEDVALUE(matrix.v.z2,  9.0f);
		EXPECTEDVALUE(matrix.v.w2, 12.0f);
		EXPECTEDVALUE(matrix.v.x3, 45.5f);
		EXPECTEDVALUE(matrix.v.y3, 49.0f);
		EXPECTEDVALUE(matrix.v.z3, 52.5f);
		EXPECTEDVALUE(matrix.v.w3, 56.0f);
		EXPECTEDVALUE(matrix.v.x4,-22.5f);
		EXPECTEDVALUE(matrix.v.y4,-27.0f);
		EXPECTEDVALUE(matrix.v.z4,-31.5f);
		EXPECTEDVALUE(matrix.v.w4,-36.0f);
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestComplex(io::LogStream& cout) {
	cout << "Unit testing complex numbers\n";
	// complex c, z;
	// for (i32 y = -40; y <= 40; y++) {
	//	 for (i32 x = -70; x <= 50; x++) {
	//		 c = z = complex(f32(x)/40.0, f32(y)/20.0);
	//		 const char val[] = " `*+%";
	//		 u32 its = 0;
	//		 for (; its < 14; its++) {
	//			 z = pow(z,4.0) + c;
	//			 if (abs(z) > 2.0)
	//				 break;
	//		 }
	//		 cout << val[its/3];
	//	 }
	//	 cout << "\n";
	// }
	// complex a(2, pi);
	// a = exp(a);
	// cout << "exp(2 + pi*i) = (" << a.real << " + " << a.imag << "i)\n";
	// a = log(a);
	// cout << "log of previous value = (" << a.real << " + " << a.imag << "i)\n";
	// cout << std::endl;
	try {
		complex c;
		for (Degrees32 degrees = -360.0f; degrees < 360.0f; degrees += 1.0f) {
			Angle32 angle = degrees;
			c = exp(complex(0.0f, angle.value()));
			EXPECTEDVALUE(c.real, cos(angle));
			EXPECTEDVALUE(c.imag, sin(angle));
			c = log(c);
			EXPECTEDVALUE(c.real, 0.0f);
			EXPECTEDVALUE(Angle32(c.imag), angle);
			// Angle32 a = c.imag;
			// f32 diff = (a-angle).value();
			// cout << "diff = " << diff << ", degrees = " << degrees.value() << ", angle = " << angle.value()
			//	  << ", c.real = " << c.real << ", c.imag = " << c.imag << std::endl;
		}

		c = {1.0f, 1.0f};
		c *= c;
		EXPECTEDVALUE(c.real, 0.0f);
		EXPECTEDVALUE(c.imag, 2.0f);
		c /= 2.0f;
		EXPECTEDVALUE(c.real, 0.0f);
		EXPECTEDVALUE(c.imag, 1.0f);
		c *= c;
		EXPECTEDVALUE(c.real, -1.0f);
		EXPECTEDVALUE(c.imag, 0.0f);
		c = {sin(pi*2.0f/3.0f), cos(pi/3.0f)};
		EXPECTEDVALUE(abs(c), 1.0f);
		c = pow(c, 2.0f);
		EXPECTEDVALUE(c.real, cos(pi/3.0f));
		EXPECTEDVALUE(c.imag, sin(pi*2.0f/3.0f));
		// cout << "c.real = " << c.real << ", c.imag = " << c.imag << std::endl;
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

void QuatVsMatrix(const quat& quaternion, const mat3& matrix) {
	mat3 quatMatrix = quaternion.ToMat3();
	EXPECTEDVALUE(quatMatrix.h.x1, matrix.h.x1);
	EXPECTEDVALUE(quatMatrix.h.y1, matrix.h.y1);
	EXPECTEDVALUE(quatMatrix.h.z1, matrix.h.z1);
	EXPECTEDVALUE(quatMatrix.h.x2, matrix.h.x2);
	EXPECTEDVALUE(quatMatrix.h.y2, matrix.h.y2);
	EXPECTEDVALUE(quatMatrix.h.z2, matrix.h.z2);
	EXPECTEDVALUE(quatMatrix.h.x3, matrix.h.x3);
	EXPECTEDVALUE(quatMatrix.h.y3, matrix.h.y3);
	EXPECTEDVALUE(quatMatrix.h.z3, matrix.h.z3);
}

bool UnitTestQuat(io::LogStream& cout) {
	cout << "Unit testing quaternions\n";
	try {
		quat quaternion;
		mat3 matrix;
		vec3 point1, point2;
		for (Degrees32 degrees = -360.0f; degrees <= 360.0f; degrees += 5.0f) {
			Radians32 radians = degrees;
			quaternion = quat::Rotation(radians.value(), {1.0f, 0.0f, 0.0f});
			matrix = mat3::Rotation(radians.value(), {1.0f, 0.0f, 0.0f});
			QuatVsMatrix(quaternion, matrix);
			point1 = {1.0f, 0.0f, 0.0f};
			point2 = matrix * point1;
			point1 = quaternion.RotatePoint(point1);
			EXPECTEDVALUE(point1.x, point2.x);
			EXPECTEDVALUE(point1.y, point2.y);
			EXPECTEDVALUE(point1.z, point2.z);
			quaternion = quat::Rotation(radians.value(), {0.0f, 1.0f, 0.0f});
			matrix = mat3::Rotation(radians.value(), {0.0f, 1.0f, 0.0f});
			QuatVsMatrix(quaternion, matrix);
			point1 = {0.0f, 1.0f, 0.0f};
			point2 = matrix * point1;
			point1 = quaternion.RotatePoint(point1);
			EXPECTEDVALUE(point1.x, point2.x);
			EXPECTEDVALUE(point1.y, point2.y);
			EXPECTEDVALUE(point1.z, point2.z);
			quaternion = quat::Rotation(radians.value(), {0.0f, 0.0f, 1.0f});
			matrix = mat3::Rotation(radians.value(), {0.0f, 0.0f, 1.0f});
			QuatVsMatrix(quaternion, matrix);
			point1 = {0.0f, 0.0f, 1.0f};
			point2 = matrix * point1;
			point1 = quaternion.RotatePoint(point1);
			EXPECTEDVALUE(point1.x, point2.x);
			EXPECTEDVALUE(point1.y, point2.y);
			EXPECTEDVALUE(point1.z, point2.z);
			quaternion = quat::Rotation(radians.value(), {0.0f, 1.0f, 1.0f});
			matrix = mat3::Rotation(radians.value(), {0.0f, 1.0f, 1.0f});
			QuatVsMatrix(quaternion, matrix);
			point1 = {1.0f, 2.0f, 3.0f};
			point2 = matrix * point1;
			point1 = quaternion.RotatePoint(point1);
			EXPECTEDVALUE(point1.x, point2.x);
			EXPECTEDVALUE(point1.y, point2.y);
			EXPECTEDVALUE(point1.z, point2.z);
			quaternion = quat::Rotation(radians.value(), {-1.0f, 1.0f, 0.0f});
			matrix = mat3::Rotation(radians.value(), {-1.0f, 1.0f, 0.0f});
			QuatVsMatrix(quaternion, matrix);
			point1 = {-1.0f, pi, 1.0f};
			point2 = matrix * point1;
			point1 = quaternion.RotatePoint(point1);
			EXPECTEDVALUE(point1.x, point2.x);
			EXPECTEDVALUE(point1.y, point2.y);
			EXPECTEDVALUE(point1.z, point2.z);
			quaternion = quat::Rotation(radians.value(), {-1.0f, 0.0f, -1.0f});
			matrix = mat3::Rotation(radians.value(), {-1.0f, 0.0f, -1.0f});
			QuatVsMatrix(quaternion, matrix);
			point1 = {1.0f, 8.0f, 3.0f};
			point2 = matrix * point1;
			point1 = quaternion.RotatePoint(point1);
			EXPECTEDVALUE(point1.x, point2.x);
			EXPECTEDVALUE(point1.y, point2.y);
			EXPECTEDVALUE(point1.z, point2.z);
		}
		// quat a(pi, 0, -1, 0);
		// a = exp(a);
		// cout << "exp(pi - j) = (" << a.w << " + " << a.x << "i + " << a.y << "j + " << a.z << "k)\n";
		// a = log(a);
		// cout << "log of previous value = (" << a.w << " + " << a.x << "i + " << a.y << "j + " << a.z << "k)\n";
		// cout << std::endl;
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestSlerp(io::LogStream& cout) {
	cout << "Unit testing slerp...\n";
	try {
		quat a(0.0f, 1.0f, 0.0f, 0.0f);
		quat b(0.0f, 0.0f, 1.0f, 0.0f);
		quat c;
		c = slerp(a,b, 0.0f);
		EXPECTEDVALUE(c.w, a.w);
		EXPECTEDVALUE(c.x, a.x);
		EXPECTEDVALUE(c.y, a.y);
		EXPECTEDVALUE(c.z, a.z);
		c = slerp(a,b, 1.0f);
		EXPECTEDVALUE(c.w, b.w);
		EXPECTEDVALUE(c.x, b.x);
		EXPECTEDVALUE(c.y, b.y);
		EXPECTEDVALUE(c.z, b.z);
		c = slerp(a,b, 0.5f);
		EXPECTEDVALUE(c.w, 0.0f);
		EXPECTEDVALUE(c.x, sin(pi/4.0f));
		EXPECTEDVALUE(c.y, sin(pi/4.0f));
		EXPECTEDVALUE(c.z, 0.0f);
		c = slerp(a,b, 1.0f/3.0f);
		EXPECTEDVALUE(c.w, 0.0f);
		EXPECTEDVALUE(c.x, cos(pi/6.0f));
		EXPECTEDVALUE(c.y, sin(pi/6.0f));
		EXPECTEDVALUE(c.z, 0.0f);
		c = slerp(a,b, 2.0f/3.0f);
		EXPECTEDVALUE(c.w, 0.0f);
		EXPECTEDVALUE(c.x, cos(tau/6.0f));
		EXPECTEDVALUE(c.y, sin(tau/6.0f));
		EXPECTEDVALUE(c.z, 0.0f);
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestRNG(io::LogStream& cout) {
	RandomNumberGenerator rng;
	cout << "Unit testing RandomNumberGenerator...\n";
	{
		u32 totalDelta = 0;
		u32 totalDone = 0;
		u32 maxDelta = 0;
		u32 totalMissed = 0;
		u16 count[500];
		for (u32 num = 4; num <= 500; num+=2) {
			for (u32 repeat = 0; repeat < 100; repeat++) {
				for (u32 i = 0; i < num; i++) {
					count[i] = 0;
					totalDone++;
				}
				for (u32 i = 0; i < num * 15; i++) {
					count[rng.Generate()%num]++;
				}
				for (u32 i = 0; i < num; i++) {
					if (count[i] == 0) {
						totalMissed++;
					}
					i32 delta = count[i];
					delta = abs(delta-10);
					totalDelta += delta;
					if ((u32)delta > maxDelta) {
						maxDelta = delta;
					}
				}
			}
		}

		f64 avgDelta = (f64)totalDelta / (f64)totalDone;

		cout << "Average delta: " << avgDelta << ", max delta: " << maxDelta
			 << ", total missed: " << totalMissed << std::endl;
		if (totalMissed > 10) {
			cout << "Failed: Too many misses!" << std::endl;
			return false;
		}
		// cout << std::dec << "After 100000 numbers generated, 0-100 has the following counts:\n{";
		// for (u32 i = 0; i < 100; i++) {
		//	 if (count[i] < 10)
		//		 cout << " ";
		//	 if (count[i] < 100)
		//		 cout << " ";
		//	 if (count[i] < 1000)
		//		 cout << " ";
		//	 cout << count[i];
		//	 if (i != 99) {
		//		 cout << ", ";
		//		 if (i%10 == 9)
		//			 cout << "\n ";
		//	 }
		// }
		// cout << "}" << std::endl;
	}
	Array<u16> count(1000000);
	for (u32 i = 0; i < 1000000; i++) {
		count[i] = 0;
	}
	cout << "After 10,000,000 numbers generated, 0-1,000,000 missed ";
	for (u32 i = 0; i < 10000000; i++) {
		count[rng.Generate()%1000000]++;
	}
	u32 total = 0;
	for (u32 i = 0; i < 1000000; i++) {
		if (count[i] == 0) {
			total++;
		}
	}
	cout << total << " indices." << std::endl;
	if (total > 100) {
		cout << "Failed: Too many misses!" << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestList(io::LogStream& cout) {
	cout << "Unit testing List<i32>...\n";
	try {
		List<i32> list = {1, 2, 3, 4};
		ListIterator<i32> it = list.begin();
		EXPECTEDVALUE(list.size(), 4);
		EXPECTEDVALUE((*it).value, 1);
		EXPECTEDVALUE((*(++it)).value, 2);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		list.Prepend(5);
		it = list.begin();
		EXPECTEDVALUE(list.size(), 5);
		EXPECTEDVALUE((*it).value, 5);
		EXPECTEDVALUE((*(++it)).value, 1);
		EXPECTEDVALUE((*(++it)).value, 2);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		it = list.begin() + 2;
		(*it).InsertNext(6);
		it = list.begin();
		EXPECTEDVALUE(list.size(), 6);
		EXPECTEDVALUE((*it).value, 5);
		EXPECTEDVALUE((*(++it)).value, 1);
		EXPECTEDVALUE((*(++it)).value, 2);
		EXPECTEDVALUE((*(++it)).value, 6);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		List<i32> secondList = list;
		it = secondList.begin();
		EXPECTEDVALUE(secondList.size(), 6);
		EXPECTEDVALUE((*it).value, 5);
		EXPECTEDVALUE((*(++it)).value, 1);
		EXPECTEDVALUE((*(++it)).value, 2);
		EXPECTEDVALUE((*(++it)).value, 6);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		List<i32> thirdList = std::move(secondList);
		it = thirdList.begin();
		EXPECTEDVALUE(secondList.size(), 0);
		EXPECTEDVALUE(thirdList.size(), 6);
		EXPECTEDVALUE((*it).value, 5);
		EXPECTEDVALUE((*(++it)).value, 1);
		EXPECTEDVALUE((*(++it)).value, 2);
		EXPECTEDVALUE((*(++it)).value, 6);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		it = list.begin() + 1;
		(*it).EraseNext();
		it = list.begin();
		EXPECTEDVALUE(list.size(), 5);
		EXPECTEDVALUE((*it).value, 5);
		EXPECTEDVALUE((*(++it)).value, 1);
		EXPECTEDVALUE((*(++it)).value, 6);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		list.EraseFirst();
		it = list.begin();
		EXPECTEDVALUE(list.size(), 4);
		EXPECTEDVALUE((*it).value, 1);
		EXPECTEDVALUE((*(++it)).value, 6);
		EXPECTEDVALUE((*(++it)).value, 3);
		EXPECTEDVALUE((*(++it)).value, 4);
		list.EraseLast();
		it = list.begin();
		EXPECTEDVALUE(list.size(), 3);
		EXPECTEDVALUE((*it).value, 1);
		EXPECTEDVALUE((*(++it)).value, 6);
		EXPECTEDVALUE((*(++it)).value, 3);
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

template<typename T>
void PrintArray(const Array<T>& array, const char* name, io::LogStream& cout) {
	cout << name << " = {";
	for (i32 i = 0; i < array.size; i++) {
		cout << array[i];
		if (i != array.size-1) {
			cout << ", ";
		}
	}
	cout << "}" << std::endl;
}

bool UnitTestArrayAndString(io::LogStream& cout) {
	cout << "Unit testing Array and String...\n";
	try {
		Array<i32> test1 = {
			1, 2, 3, 4, 5, 6, 7, 8, 9
		};
		EXPECTEDVALUE(test1.size, 9);
		EXPECTEDVALUE(test1[0], 1);
		EXPECTEDVALUE(test1[1], 2);
		EXPECTEDVALUE(test1[2], 3);
		EXPECTEDVALUE(test1[3], 4);
		EXPECTEDVALUE(test1[4], 5);
		EXPECTEDVALUE(test1[5], 6);
		EXPECTEDVALUE(test1[6], 7);
		EXPECTEDVALUE(test1[7], 8);
		EXPECTEDVALUE(test1[8], 9);
		Array<String> test2 = {
			"There once was a man who hated cheese.",
			"He was also an absolute dick.",
			"You should probably stay away from him."
		};
		EXPECTEDVALUE(test2.size, 3);
		EXPECTEDVALUE(test2[0], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[1], String("He was also an absolute dick."));
		EXPECTEDVALUE(test2[2], String("You should probably stay away from him."));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Adding values to the end of both Arrays..." << std::endl;
		test1 += 10;
		EXPECTEDVALUE(test1.size, 10);
		EXPECTEDVALUE(test1[0], 1);
		EXPECTEDVALUE(test1[1], 2);
		EXPECTEDVALUE(test1[2], 3);
		EXPECTEDVALUE(test1[3], 4);
		EXPECTEDVALUE(test1[4], 5);
		EXPECTEDVALUE(test1[5], 6);
		EXPECTEDVALUE(test1[6], 7);
		EXPECTEDVALUE(test1[7], 8);
		EXPECTEDVALUE(test1[8], 9);
		EXPECTEDVALUE(test1[9], 10);
		test2 += String("I think everything should be okay anyways.");
		EXPECTEDVALUE(test2.size, 4);
		EXPECTEDVALUE(test2[0], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[1], String("He was also an absolute dick."));
		EXPECTEDVALUE(test2[2], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[3], String("I think everything should be okay anyways."));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Adding values to the beginning of both Arrays..." << std::endl;
		test1.Insert(0,0);
		EXPECTEDVALUE(test1.size, 11);
		EXPECTEDVALUE(test1[0], 0);
		EXPECTEDVALUE(test1[1], 1);
		EXPECTEDVALUE(test1[2], 2);
		EXPECTEDVALUE(test1[3], 3);
		EXPECTEDVALUE(test1[4], 4);
		EXPECTEDVALUE(test1[5], 5);
		EXPECTEDVALUE(test1[6], 6);
		EXPECTEDVALUE(test1[7], 7);
		EXPECTEDVALUE(test1[8], 8);
		EXPECTEDVALUE(test1[9], 9);
		EXPECTEDVALUE(test1[10], 10);
		test2.Insert(0, "Once upon a time,");
		EXPECTEDVALUE(test2.size, 5);
		EXPECTEDVALUE(test2[0], String("Once upon a time,"));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("He was also an absolute dick."));
		EXPECTEDVALUE(test2[3], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[4], String("I think everything should be okay anyways."));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Adding values to the middle of both Arrays..." << std::endl;
		test1.Insert(7,67);
		EXPECTEDVALUE(test1.size, 12);
		EXPECTEDVALUE(test1[0], 0);
		EXPECTEDVALUE(test1[1], 1);
		EXPECTEDVALUE(test1[2], 2);
		EXPECTEDVALUE(test1[3], 3);
		EXPECTEDVALUE(test1[4], 4);
		EXPECTEDVALUE(test1[5], 5);
		EXPECTEDVALUE(test1[6], 6);
		EXPECTEDVALUE(test1[7], 67);
		EXPECTEDVALUE(test1[8], 7);
		EXPECTEDVALUE(test1[9], 8);
		EXPECTEDVALUE(test1[10], 9);
		EXPECTEDVALUE(test1[11], 10);
		test2.Insert(3,"And he likes to hurt people.");
		EXPECTEDVALUE(test2.size, 6);
		EXPECTEDVALUE(test2[0], String("Once upon a time,"));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("He was also an absolute dick."));
		EXPECTEDVALUE(test2[3], String("And he likes to hurt people."));
		EXPECTEDVALUE(test2[4], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[5], String("I think everything should be okay anyways."));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Erasing values from the middle of both Arrays..." << std::endl;
		test1.Erase(6);
		EXPECTEDVALUE(test1.size, 11);
		EXPECTEDVALUE(test1[0], 0);
		EXPECTEDVALUE(test1[1], 1);
		EXPECTEDVALUE(test1[2], 2);
		EXPECTEDVALUE(test1[3], 3);
		EXPECTEDVALUE(test1[4], 4);
		EXPECTEDVALUE(test1[5], 5);
		EXPECTEDVALUE(test1[6], 67);
		EXPECTEDVALUE(test1[7], 7);
		EXPECTEDVALUE(test1[8], 8);
		EXPECTEDVALUE(test1[9], 9);
		EXPECTEDVALUE(test1[10], 10);
		test1.Erase(7);
		EXPECTEDVALUE(test1.size, 10);
		EXPECTEDVALUE(test1[0], 0);
		EXPECTEDVALUE(test1[1], 1);
		EXPECTEDVALUE(test1[2], 2);
		EXPECTEDVALUE(test1[3], 3);
		EXPECTEDVALUE(test1[4], 4);
		EXPECTEDVALUE(test1[5], 5);
		EXPECTEDVALUE(test1[6], 67);
		EXPECTEDVALUE(test1[7], 8);
		EXPECTEDVALUE(test1[8], 9);
		EXPECTEDVALUE(test1[9], 10);
		test1.Erase(1, 2);
		EXPECTEDVALUE(test1.size, 8);
		EXPECTEDVALUE(test1[0], 0);
		EXPECTEDVALUE(test1[1], 3);
		EXPECTEDVALUE(test1[2], 4);
		EXPECTEDVALUE(test1[3], 5);
		EXPECTEDVALUE(test1[4], 67);
		EXPECTEDVALUE(test1[5], 8);
		EXPECTEDVALUE(test1[6], 9);
		EXPECTEDVALUE(test1[7], 10);
		test2.Erase(5);
		EXPECTEDVALUE(test2.size, 5);
		EXPECTEDVALUE(test2[0], String("Once upon a time,"));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("He was also an absolute dick."));
		EXPECTEDVALUE(test2[3], String("And he likes to hurt people."));
		EXPECTEDVALUE(test2[4], String("You should probably stay away from him."));
		test2.Erase(2, 2);
		EXPECTEDVALUE(test2.size, 3);
		EXPECTEDVALUE(test2[0], String("Once upon a time,"));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("You should probably stay away from him."));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Reversing both Arrays..." << std::endl;
		test1.Reverse();
		EXPECTEDVALUE(test1.size, 8);
		EXPECTEDVALUE(test1[0], 10);
		EXPECTEDVALUE(test1[1], 9);
		EXPECTEDVALUE(test1[2], 8);
		EXPECTEDVALUE(test1[3], 67);
		EXPECTEDVALUE(test1[4], 5);
		EXPECTEDVALUE(test1[5], 4);
		EXPECTEDVALUE(test1[6], 3);
		EXPECTEDVALUE(test1[7], 0);
		test2.Reverse();
		EXPECTEDVALUE(test2.size, 3);
		EXPECTEDVALUE(test2[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("Once upon a time,"));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Appending both Arrays with other Arrays..." << std::endl;
		test1 += {11, 12, 13, 14, 15};
		EXPECTEDVALUE(test1.size, 13);
		EXPECTEDVALUE(test1[0], 10);
		EXPECTEDVALUE(test1[1], 9);
		EXPECTEDVALUE(test1[2], 8);
		EXPECTEDVALUE(test1[3], 67);
		EXPECTEDVALUE(test1[4], 5);
		EXPECTEDVALUE(test1[5], 4);
		EXPECTEDVALUE(test1[6], 3);
		EXPECTEDVALUE(test1[7], 0);
		EXPECTEDVALUE(test1[8], 11);
		EXPECTEDVALUE(test1[9], 12);
		EXPECTEDVALUE(test1[10], 13);
		EXPECTEDVALUE(test1[11], 14);
		EXPECTEDVALUE(test1[12], 15);
		test2 += {"What was I talking about?", "This is getting pretty weird!"};
		EXPECTEDVALUE(test2.size, 5);
		EXPECTEDVALUE(test2[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("Once upon a time,"));
		EXPECTEDVALUE(test2[3], String("What was I talking about?"));
		EXPECTEDVALUE(test2[4], String("This is getting pretty weird!"));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Resizing both Arrays to 4..." << std::endl;
		test1.Resize(4);
		EXPECTEDVALUE(test1.size, 4);
		EXPECTEDVALUE(test1[0], 10);
		EXPECTEDVALUE(test1[1], 9);
		EXPECTEDVALUE(test1[2], 8);
		EXPECTEDVALUE(test1[3], 67);
		test2.Resize(4);
		EXPECTEDVALUE(test2.size, 4);
		EXPECTEDVALUE(test2[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("Once upon a time,"));
		EXPECTEDVALUE(test2[3], String("What was I talking about?"));
		// PrintArray(test1, "test1", cout);
		// PrintArray(test2, "test2", cout);
		// cout << "Using copy constructors..." << std::endl;
		Array<i32> test3(test1);
		EXPECTEDVALUE(test3.size, 4);
		EXPECTEDVALUE(test3[0], 10);
		EXPECTEDVALUE(test3[1], 9);
		EXPECTEDVALUE(test3[2], 8);
		EXPECTEDVALUE(test3[3], 67);
		Array<String> test4(test2);
		EXPECTEDVALUE(test4.size, 4);
		EXPECTEDVALUE(test4[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test4[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test4[2], String("Once upon a time,"));
		EXPECTEDVALUE(test4[3], String("What was I talking about?"));
		// PrintArray(test3, "test3", cout);
		// PrintArray(test4, "test4", cout);
		// cout << "Using copy assignment..." << std::endl;
		Array<i32> test5;
		test5 = test1;
		EXPECTEDVALUE(test5.size, 4);
		EXPECTEDVALUE(test5[0], 10);
		EXPECTEDVALUE(test5[1], 9);
		EXPECTEDVALUE(test5[2], 8);
		EXPECTEDVALUE(test5[3], 67);
		Array<String> test6;
		test6 = test2;
		EXPECTEDVALUE(test6.size, 4);
		EXPECTEDVALUE(test6[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test6[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test6[2], String("Once upon a time,"));
		EXPECTEDVALUE(test6[3], String("What was I talking about?"));
		// PrintArray(test5, "test5", cout);
		// PrintArray(test6, "test6", cout);
		// cout << "Printing them all added together with +..." << std::endl;
		Array<i32> test7 = test1 + test3 + std::move(test5);
		EXPECTEDVALUE(test5.size, 0);
		EXPECTEDVALUE(test7.size, 12);
		EXPECTEDVALUE(test7[0], 10);
		EXPECTEDVALUE(test7[1], 9);
		EXPECTEDVALUE(test7[2], 8);
		EXPECTEDVALUE(test7[3], 67);
		EXPECTEDVALUE(test7[4], 10);
		EXPECTEDVALUE(test7[5], 9);
		EXPECTEDVALUE(test7[6], 8);
		EXPECTEDVALUE(test7[7], 67);
		EXPECTEDVALUE(test7[8], 10);
		EXPECTEDVALUE(test7[9], 9);
		EXPECTEDVALUE(test7[10], 8);
		EXPECTEDVALUE(test7[11], 67);
		Array<String> test8 = test2 + test4 + std::move(test6);
		EXPECTEDVALUE(test6.size, 0);
		EXPECTEDVALUE(test8.size, 12);
		EXPECTEDVALUE(test8[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test8[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test8[2], String("Once upon a time,"));
		EXPECTEDVALUE(test8[3], String("What was I talking about?"));
		EXPECTEDVALUE(test8[4], String("You should probably stay away from him."));
		EXPECTEDVALUE(test8[5], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test8[6], String("Once upon a time,"));
		EXPECTEDVALUE(test8[7], String("What was I talking about?"));
		EXPECTEDVALUE(test8[8], String("You should probably stay away from him."));
		EXPECTEDVALUE(test8[9], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test8[10], String("Once upon a time,"));
		EXPECTEDVALUE(test8[11], String("What was I talking about?"));
		// PrintArray(test7, "test7", cout);
		// PrintArray(test8, "test8", cout);
		test1 = std::move(test7);
		EXPECTEDVALUE(test7.size, 0);
		EXPECTEDVALUE(test1.size, 12);
		EXPECTEDVALUE(test1[0], 10);
		EXPECTEDVALUE(test1[1], 9);
		EXPECTEDVALUE(test1[2], 8);
		EXPECTEDVALUE(test1[3], 67);
		EXPECTEDVALUE(test1[4], 10);
		EXPECTEDVALUE(test1[5], 9);
		EXPECTEDVALUE(test1[6], 8);
		EXPECTEDVALUE(test1[7], 67);
		EXPECTEDVALUE(test1[8], 10);
		EXPECTEDVALUE(test1[9], 9);
		EXPECTEDVALUE(test1[10], 8);
		EXPECTEDVALUE(test1[11], 67);
		test2 = std::move(test8);
		EXPECTEDVALUE(test8.size, 0);
		EXPECTEDVALUE(test2.size, 12);
		EXPECTEDVALUE(test2[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[2], String("Once upon a time,"));
		EXPECTEDVALUE(test2[3], String("What was I talking about?"));
		EXPECTEDVALUE(test2[4], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[5], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[6], String("Once upon a time,"));
		EXPECTEDVALUE(test2[7], String("What was I talking about?"));
		EXPECTEDVALUE(test2[8], String("You should probably stay away from him."));
		EXPECTEDVALUE(test2[9], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test2[10], String("Once upon a time,"));
		EXPECTEDVALUE(test2[11], String("What was I talking about?"));

		Array<i32> test9(std::move(test1));
		EXPECTEDVALUE(test1.size, 0);
		EXPECTEDVALUE(test9.size, 12);
		EXPECTEDVALUE(test9[0], 10);
		EXPECTEDVALUE(test9[1], 9);
		EXPECTEDVALUE(test9[2], 8);
		EXPECTEDVALUE(test9[3], 67);
		EXPECTEDVALUE(test9[4], 10);
		EXPECTEDVALUE(test9[5], 9);
		EXPECTEDVALUE(test9[6], 8);
		EXPECTEDVALUE(test9[7], 67);
		EXPECTEDVALUE(test9[8], 10);
		EXPECTEDVALUE(test9[9], 9);
		EXPECTEDVALUE(test9[10], 8);
		EXPECTEDVALUE(test9[11], 67);
		Array<String> test10(std::move(test2));
		EXPECTEDVALUE(test2.size, 0);
		EXPECTEDVALUE(test10.size, 12);
		EXPECTEDVALUE(test10[0], String("You should probably stay away from him."));
		EXPECTEDVALUE(test10[1], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test10[2], String("Once upon a time,"));
		EXPECTEDVALUE(test10[3], String("What was I talking about?"));
		EXPECTEDVALUE(test10[4], String("You should probably stay away from him."));
		EXPECTEDVALUE(test10[5], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test10[6], String("Once upon a time,"));
		EXPECTEDVALUE(test10[7], String("What was I talking about?"));
		EXPECTEDVALUE(test10[8], String("You should probably stay away from him."));
		EXPECTEDVALUE(test10[9], String("There once was a man who hated cheese."));
		EXPECTEDVALUE(test10[10], String("Once upon a time,"));
		EXPECTEDVALUE(test10[11], String("What was I talking about?"));
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

bool UnitTestToString(io::LogStream &cout) {
	cout << "Testing ToString." << std::endl;
	try {
		EXPECTEDVALUE(ToString(1.0f), String("1.0"));
		EXPECTEDVALUE(ToString(0.1f), String("0.1"));
		EXPECTEDVALUE(ToString(3.14159265f), String("3.1415927"));
		EXPECTEDVALUE(ToString(3.14159f, 10, 2), String("3.14"));
		EXPECTEDVALUE(ToString(63.14159f, 10, 2), String("63.14"));
		EXPECTEDVALUE(ToString(963.14159f, 10, 2), String("963.14"));
		EXPECTEDVALUE(ToString(123.456f, 10, 2), String("123.46"));
		EXPECTEDVALUE(ToString(123.456789, 10, 4), String("123.4568"));
		EXPECTEDVALUE(ToString(123.4567899, 10, 5), String("123.45679"));
		EXPECTEDVALUE(ToString(123.4567899, 10, 6), String("123.45679"));
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

void ProfileEquations(io::LogStream &cout) {
	// Get how long it takes to generate numbers
	RandomNumberGenerator rng;
	ClockTime start = Clock::now();
	f32 dontOptimizeThisOut = 0.0f;
	for (i32 i = 0; i < 100000000; i++) {
		dontOptimizeThisOut += random(-10.0f, 10.0f, &rng);
	}
	Nanoseconds rngDuration = Clock::now()-start;
	cout << "Time to generate 100000000 random numbers: " << FormatTime(rngDuration) << std::endl;
	// Get how long it takes to solve random Quadratics, Cubics, Quartics, and Quintics

	// Quadratics
	start = Clock::now();
	for (i32 i = 0; i < 1000000; i++) {
		SolveQuadratic(
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng)
		);
	}
	Nanoseconds quadraticsTime = Clock::now()-start;
	cout << "Time to solve 1000000 random quadratics: " << FormatTime(quadraticsTime)
		 << "\n\tAdjusted for rng time: " << FormatTime(quadraticsTime - rngDuration*3/100) << std::endl;

	// Cubics
	start = Clock::now();
	for (i32 i = 0; i < 1000000; i++) {
		SolveCubic(
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng)
		);
	}
	Nanoseconds cubicsTime = Clock::now()-start;
	cout << "Time to solve 1000000 random cubics: " << FormatTime(cubicsTime)
		 << "\n\tAdjusted for rng time: " << FormatTime(cubicsTime - rngDuration*4/100) << std::endl;

	// Quartics
	start = Clock::now();
	for (i32 i = 0; i < 1000000; i++) {
		SolveQuartic(
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng)
		);
	}
	Nanoseconds quarticsTime = Clock::now()-start;
	cout << "Time to solve 1000000 random quartics: " << FormatTime(quarticsTime)
		 << "\n\tAdjusted for rng time: " << FormatTime(quarticsTime - rngDuration*5/100) << std::endl;

	// Quintics
	start = Clock::now();
	for (i32 i = 0; i < 1000000; i++) {
		SolveQuintic(
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng),
			random(-10.0f, 10.0f, &rng)
		);
	}
	Nanoseconds quinticsTime = Clock::now()-start;
	cout << "Time to solve 1000000 random quintics: " << FormatTime(quinticsTime)
		 << "\n\tAdjusted for rng time: " << FormatTime(quinticsTime - rngDuration*6/100) << std::endl;

	cout << "Roots:" << std::endl;
	SolutionQuintic<f32> solution = SolveQuintic(1.7f, 6.0f, 3.0f, -6.3f, -3.0f, 2.2f);
	for (i32 i = 0; i < solution.nReal; i++) {
		cout << "\t" << solution.root[i] << std::endl;
	}
}

bool UnitTestShuffle(io::LogStream &cout) {
	cout << "Testing shuffle." << std::endl;
	try {
		HashSet<i32> set;
		i32 shuffleId = genShuffleId();
		for (i32 i = 0; i < 100; i++) {
			i32 index = shuffle(shuffleId, 100);
			if (set.Exists(index)) {
				String error = Stringify("index ", index, " was already set!");
				throw std::runtime_error(error.data);
			}
			set.Emplace(index);
		}
		i32 mustExist = shuffle(shuffleId, 100);
		if (!set.Exists(mustExist)) {
			String error = Stringify("Expected the shuffle playlist to restart, but we somehow got a number not already in the set: ", mustExist);
			throw std::runtime_error(error.data);
		}
	} catch (std::runtime_error& err) {
		cout << "Failed: " << err.what() << std::endl;
		return false;
	}
	cout << "...Success!" << std::endl;
	return true;
}

i32 main(i32 argumentCount, char** argumentValues) {
	io::LogStream cout("test.log");

	cout << "Doing unit tests..." << std::endl;

	const u32 numTests = 11;
	u32 numSuccessful = 0;

	if (UnitTestComplex(cout)) {
		numSuccessful++;
	}
	if (UnitTestQuat(cout)) {
		numSuccessful++;
	}
	if (UnitTestSlerp(cout)) {
		numSuccessful++;
	}
	if (UnitTestMat2(cout)) {
		numSuccessful++;
	}
	if (UnitTestMat3(cout)) {
		numSuccessful++;
	}
	if (UnitTestMat4(cout)) {
		numSuccessful++;
	}
	if (UnitTestList(cout)) {
		numSuccessful++;
	}
	if (UnitTestArrayAndString(cout)) {
		numSuccessful++;
	}
	if (UnitTestRNG(cout)) {
		numSuccessful++;
	}
	if (UnitTestShuffle(cout)) {
		numSuccessful++;
	}

	if (UnitTestToString(cout)) {
		numSuccessful++;
	}

	cout << "Unit tests complete: " << numSuccessful << "/" << numTests << " were successful." << std::endl;

	// ProfileEquations(cout);
}
