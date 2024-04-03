/*
	File: CustomMaths.cpp
	Author: Philip Haynes
	Verifying the effectiveness and error margins of custom math approximation functions.
*/

#include "AzCore/Math/Matrix.hpp"
#include "../UnitTests.hpp"
#include "../Utilities.hpp"

namespace MatrixTestNamespace {

void MatrixTest();
UT::Register matrixTest("Matrix", MatrixTest);

using Real = f32;
using Matrix = az::Matrix<Real>;
using Vector = az::Vector<Real>;

FPError<Real> fpError;

Real maxErrorWeak = 10, maxErrorFail = 100;

#define COMPARE_FP(lhs, rhs, magnitude, info) fpError.Compare(lhs, rhs, magnitude, __LINE__, info, maxErrorWeak, maxErrorFail)

#define COMPARE_VECTOR(lhs, rhs, magnitude) {\
	UTExpectEquals((lhs).Count(), (rhs).Count(), "Differently-sized!");\
	for (i32 i = 0; i < (lhs).Count(); i++) {\
		COMPARE_FP((lhs)[i], (rhs)[i], magnitude, az::Stringify("[", i, "]"));\
	}\
}

#define COMPARE_MATRIX(lhs, rhs, magnitude) {\
	UTExpectEquals((lhs).Cols(), (rhs).Cols(), "Differently-sized!");\
	UTExpectEquals((lhs).Rows(), (rhs).Rows(), "Differently-sized!");\
	if ((lhs).Cols() == (rhs).Cols() && (lhs).Rows() == (rhs).Rows()) {\
		for (i32 c = 0; c < (lhs).Cols(); c++) {\
			for (i32 r = 0; r < (lhs).Rows(); r++) {\
				COMPARE_FP((lhs)[c][r], (rhs)[c][r], magnitude, az::Stringify("[", c, ",", r, "]"));\
			}\
		}\
	}\
}

void MatrixTest() {
	Matrix initial, result, expect;
	initial = Matrix::Filled(3, 2, {
		0.0f, 2.0f, 4.0f,
		1.0f, 3.0f, 5.0f,
	});
	// Get one that points at the original data
	result = &initial;
	UTExpect(result.data == initial.data, "We didn't make an unowned copy!");
	COMPARE_MATRIX(result, initial, 1.0f);
	// This should affect initial as well
	result.Val(0, 0) = 10.0f;
	COMPARE_MATRIX(result, initial, 1.0f);
	result.Val(0, 0) = 0.0f;

	// Get a copy
	result = initial;
	UTExpect(result.data != initial.data, "We didn't make an owned copy!");
	COMPARE_MATRIX(result, initial, 1.0f);

	result.Transpose();
	expect = Matrix::Filled(2, 3, {
		0.0f, 1.0f,
		2.0f, 3.0f,
		4.0f, 5.0f,
	});
	COMPARE_MATRIX(result, expect, 1.0f);

	initial = Matrix::Filled(4, 2, {
		0.0f, 2.0f, 4.0f, 6.0f,
		1.0f, 3.0f, 5.0f, 7.0f,
	});
	result = az::transpose(initial);
	expect = Matrix::Filled(2, 4, {
		0.0f, 1.0f,
		2.0f, 3.0f,
		4.0f, 5.0f,
		6.0f, 7.0f,
	});
	COMPARE_MATRIX(result, expect, 1.0f);

	result = initial * result;
	expect = Matrix::Filled(2, 2, {
		56.0f, 68.0f,
		68.0f, 84.0f,
	});
	COMPARE_MATRIX(result, expect, 1.0f);

	Vector vecInitial, vecResult, vecExpect;

	vecInitial = {1.0f, 2.0f, 3.0f, 4.0f};

	vecResult = initial * vecInitial;

	vecExpect = {40.0f, 50.0f};
	COMPARE_VECTOR(vecResult, vecExpect, 1.0f);

	vecResult = vecInitial * az::transpose(initial);
	COMPARE_VECTOR(vecResult, vecExpect, 1.0f);

	initial = Matrix::Filled(3, 3, {
		1.0f, 4.0f, 7.0f,
		2.0f, 5.0f, 8.0f,
		3.0f, 6.0f, 9.0f,
	});
	result = initial.SubMatrix(0, 0, 2, 2);
	expect = Matrix::Filled(2, 2, {
		1.0f, 4.0f,
		2.0f, 5.0f,
	});
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Transpose();
	expect.Transpose();
	COMPARE_MATRIX(result, expect, 1.0f);

	expect = Matrix::Filled(3, 3, {
		1.0f, 2.0f, 7.0f,
		4.0f, 5.0f, 8.0f,
		3.0f, 6.0f, 9.0f,
	});
	COMPARE_MATRIX(initial, expect, 1.0f);

	result = initial.SubMatrix(1, 1, 2, 2);
	expect = Matrix::Filled(2, 2, {
		5.0f, 8.0f,
		6.0f, 9.0f,
	});
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Transpose();
	expect.Transpose();
	COMPARE_MATRIX(result, expect, 1.0f);

	expect = Matrix::Filled(3, 3, {
		1.0f, 2.0f, 7.0f,
		4.0f, 5.0f, 6.0f,
		3.0f, 8.0f, 9.0f,
	});
	COMPARE_MATRIX(initial, expect, 1.0f);

	result = initial.SubMatrix(0, 0, 2, 2, 2, 2);
	expect = Matrix::Filled(2, 2, {
		1.0f, 7.0f,
		3.0f, 9.0f,
	});
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Transpose();
	expect.Transpose();
	COMPARE_MATRIX(result, expect, 1.0f);

	expect = Matrix::Filled(3, 3, {
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f,
		7.0f, 8.0f, 9.0f,
	});
	COMPARE_MATRIX(initial, expect, 1.0f);

	fpError.Report(__LINE__);
}

} // namespace MatrixTestNamespace