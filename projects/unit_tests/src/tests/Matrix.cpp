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
	if ((lhs).Count() != (rhs).Count()) {\
		UT::ReportProblem(__LINE__, true, "Differently-sized!");\
	} else {\
		for (i32 i = 0; i < (lhs).Count(); i++) {\
			COMPARE_FP((lhs)[i], (rhs)[i], magnitude, az::Stringify("[", i, "]"));\
		}\
	}\
}

#define COMPARE_VECTOR_INVERTIBLE(lhs, rhs, magnitude) {\
	if ((lhs).Count() != (rhs).Count()) {\
		UT::ReportProblem(__LINE__, true, "Differently-sized!");\
	} else {\
		if (normSqr(Vector((lhs) - (rhs))) < normSqr(Vector((lhs) + (rhs)))) {\
			for (i32 i = 0; i < (lhs).Count(); i++) {\
				COMPARE_FP((lhs)[i], (rhs)[i], magnitude, az::Stringify("[", i, "]"));\
			}\
		} else {\
			for (i32 i = 0; i < (lhs).Count(); i++) {\
				COMPARE_FP((lhs)[i], -(rhs)[i], magnitude, az::Stringify("[", i, "]"));\
			}\
		}\
	}\
}

#define COMPARE_MATRIX(lhs, rhs, magnitude) {\
	if ((lhs).Cols() != (rhs).Cols()) {\
		UT::ReportProblem(__LINE__, true, "Differently-sized!");\
	} else if ((lhs).Rows() != (rhs).Rows()) {\
		UT::ReportProblem(__LINE__, true, "Differently-sized!");\
	} else {\
		if ((lhs).Cols() == (rhs).Cols() && (lhs).Rows() == (rhs).Rows()) {\
			for (i32 c = 0; c < (lhs).Cols(); c++) {\
				for (i32 r = 0; r < (lhs).Rows(); r++) {\
					COMPARE_FP((lhs).Val(c,r), (rhs).Val(c,r), magnitude, az::Stringify("[", c, ",", r, "]"));\
				}\
			}\
		}\
	}\
}

void MatrixTest() {
	Matrix initial, result, expect;
	initial.Reassign(Matrix::Filled(3, 2, {
		0.0f, 2.0f, 4.0f,
		1.0f, 3.0f, 5.0f,
	}));

	// Get one that points at the original data
	result.Reassign(&initial);
	UTExpect(result.data == initial.data, "We didn't make an unowned copy!");
	COMPARE_MATRIX(result, initial, 1.0f);
	// This should affect initial as well
	result.Val(0, 0) = 10.0f;
	COMPARE_MATRIX(result, initial, 1.0f);
	result.Val(0, 0) = 0.0f;

	// Get a copy
	result.Reassign(initial);
	UTExpect(result.data != initial.data, "We didn't make an owned copy!");
	COMPARE_MATRIX(result, initial, 1.0f);

	result.Transpose();
	expect.Reassign(Matrix::Filled(2, 3, {
		0.0f, 1.0f,
		2.0f, 3.0f,
		4.0f, 5.0f,
	}));
	COMPARE_MATRIX(result, expect, 1.0f);

	initial.Reassign(Matrix::Filled(4, 2, {
		0.0f, 2.0f, 4.0f, 6.0f,
		1.0f, 3.0f, 5.0f, 7.0f,
	}));
	result.Reassign(az::transpose(&initial));
	expect.Reassign(Matrix::Filled(2, 4, {
		0.0f, 1.0f,
		2.0f, 3.0f,
		4.0f, 5.0f,
		6.0f, 7.0f,
	}));
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Reassign(initial * result);
	expect.Reassign(Matrix::Filled(2, 2, {
		56.0f, 68.0f,
		68.0f, 84.0f,
	}));
	COMPARE_MATRIX(result, expect, 1.0f);

	Vector vecInitial, vecResult, vecExpect;

	vecInitial.Reassign({1.0f, 2.0f, 3.0f, 4.0f});

	vecResult.Reassign(initial * vecInitial);

	vecExpect.Reassign({40.0f, 50.0f});
	COMPARE_VECTOR(vecResult, vecExpect, 1.0f);

	vecResult.Reassign(vecInitial * az::transpose(initial));
	COMPARE_VECTOR(vecResult, vecExpect, 1.0f);

	initial.Reassign(Matrix::Filled(3, 3, {
		1.0f, 4.0f, 7.0f,
		2.0f, 5.0f, 8.0f,
		3.0f, 6.0f, 9.0f,
	}));
	result.Reassign(initial.SubMatrix(0, 0, 2, 2));
	expect.Reassign(Matrix::Filled(2, 2, {
		1.0f, 4.0f,
		2.0f, 5.0f,
	}));
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Transpose();
	expect.Transpose();
	COMPARE_MATRIX(result, expect, 1.0f);

	expect.Reassign(Matrix::Filled(3, 3, {
		1.0f, 2.0f, 7.0f,
		4.0f, 5.0f, 8.0f,
		3.0f, 6.0f, 9.0f,
	}));
	COMPARE_MATRIX(initial, expect, 1.0f);

	result.Reassign(initial.SubMatrix(1, 1, 2, 2));
	expect.Reassign(Matrix::Filled(2, 2, {
		5.0f, 8.0f,
		6.0f, 9.0f,
	}));
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Transpose();
	expect.Transpose();
	COMPARE_MATRIX(result, expect, 1.0f);

	expect.Reassign(Matrix::Filled(3, 3, {
		1.0f, 2.0f, 7.0f,
		4.0f, 5.0f, 6.0f,
		3.0f, 8.0f, 9.0f,
	}));
	COMPARE_MATRIX(initial, expect, 1.0f);

	result.Reassign(initial.SubMatrix(0, 0, 2, 2, 2, 2));
	expect.Reassign(Matrix::Filled(2, 2, {
		1.0f, 7.0f,
		3.0f, 9.0f,
	}));
	COMPARE_MATRIX(result, expect, 1.0f);

	result.Transpose();
	expect.Transpose();
	COMPARE_MATRIX(result, expect, 1.0f);

	expect.Reassign(Matrix::Filled(3, 3, {
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f,
		7.0f, 8.0f, 9.0f,
	}));
	COMPARE_MATRIX(initial, expect, 1.0f);

	Matrix resultQ, resultR, expectQ, expectR;
	initial.Reassign(Matrix::Filled(2, 3, {
		1.0f, 2.0f,
		3.0f, 4.0f,
		5.0f, 6.0f,
	}));
	expectQ.Reassign(Matrix::Filled(2, 3, {
		0.169030850945703f, 0.897085227145060f,
		0.507092552837110f, 0.276026223736942f,
		0.845154254728517f,-0.345032779671177f,
	}));
	expectR.Reassign(Matrix::Filled(2, 2, {
		5.9160797830996160f, 7.437357441610946f,
		0.0000000000000000f, 0.828078671210825f,
	}));
	initial.QRDecomposition(resultQ, resultR);
	COMPARE_MATRIX(expectQ, resultQ, 1.0f);
	COMPARE_MATRIX(expectR, resultR, 1.0f);
	result.Reassign(resultQ * resultR);
	COMPARE_MATRIX(initial, result, 1.0f);

	initial.Reassign(Matrix::Filled(3, 2, {
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f,
	}));
	expectQ.Reassign(Matrix::Filled(2, 2, {
		0.242535625036333f, 0.970142500145332f,
		0.970142500145332f,-0.242535625036333f,
	}));
	expectR.Reassign(Matrix::Filled(3, 2, {
		4.123105625617661f, 5.335783750799325f, 6.548461875980990f,
		0.000000000000000f, 0.727606875108999f, 1.455213750217998f,
	}));
	initial.QRDecomposition(resultQ, resultR);
	COMPARE_MATRIX(expectQ, resultQ, 1.0f);
	COMPARE_MATRIX(expectR, resultR, 1.0f);
	result.Reassign(resultQ * resultR);
	COMPARE_MATRIX(initial, result, 1.0f);

	initial.Reassign(Matrix::Filled(3, 3, {
		1.0f, 2.0f, 3.0f,
		3.0f, 2.0f, 1.0f,
		2.0f, 1.0f, 3.0f,
	}));
	expectQ.Reassign(Matrix::Filled(3, 3, {
		0.267261241912424f,  0.943456353049726f,  0.196116135138184f,
		0.801783725737273f, -0.104828483672192f, -0.588348405414552f,
		0.534522483824849f, -0.314485451016575f,  0.784464540552736f,
	}));
	expectR.Reassign(Matrix::Filled(3, 3, {
		3.741657386773941f, 2.672612419124244f, 3.207134902949093f,
		0.000000000000000f, 1.362770287738494f, 1.782084222427261f,
		0.000000000000000f, 0.000000000000000f, 2.353393621658208f,
	}));
	initial.QRDecomposition(resultQ, resultR);
	COMPARE_MATRIX(expectQ, resultQ, 1.0f);
	COMPARE_MATRIX(expectR, resultR, 1.0f);
	result.Reassign(resultQ * resultR);
	COMPARE_MATRIX(initial, result, 1.0f);

	initial.Reassign(Matrix::Filled(3, 3, {
		1.0f, 2.0f, 3.0f,
		2.0f, 4.0f, 5.0f,
		3.0f, 5.0f, 6.0f,
	}));

	Matrix expectVectors = Matrix::Filled(3, 3, {
		0.445041867912629f,  1.801937735804838f, -1.246979603717467f,
		0.801937735804838f, -2.246979603717467f, -0.554958132087371f,
		1.000000000000000f,  1.000000000000000f,  1.000000000000000f,
	});
	expectVectors.Col(0).Normalize();
	expectVectors.Col(1).Normalize();
	expectVectors.Col(2).Normalize();
	UT::ReportInfo(__LINE__, "expected eigenvectors:\n", expectVectors);
	Vector expectValues = Vector({11.344814282762078f, 0.170915188827179f, -0.515729471589257f});

	Matrix resultVectors;
	Vector resultValues;
	initial.Eigen(resultVectors, resultValues, 1000, 0.00000001f);
	COMPARE_VECTOR_INVERTIBLE(resultVectors.Col(0), expectVectors.Col(0), 1.0f);
	COMPARE_VECTOR_INVERTIBLE(resultVectors.Col(1), expectVectors.Col(1), 1.0f);
	COMPARE_VECTOR_INVERTIBLE(resultVectors.Col(2), expectVectors.Col(2), 1.0f);
	COMPARE_VECTOR(resultValues, expectValues, 1.0f);

	initial.Reassign(Matrix::Filled(2, 4, {
		1.0f, 2.0f,
		3.0f, 4.0f,
		5.0f, 6.0f,
		7.0f, 8.0f,
	}));
	Matrix expectU, expectVt;
	expectU.Reassign(Matrix::Filled(2, 4, {
		0.152483233310201f,-0.822647472225659f,
		0.349918371807964f,-0.421375287684581f,
		0.547353510305727f,-0.020103103143503f,
		0.744788648803490f, 0.381169081397575f,
	}));
	expectVt.Reassign(Matrix::Filled(2, 2, {
		0.641423027995072f, 0.767187395072177f,
		0.767187395072177f,-0.641423027995072f,
	}));
	Vector expectS = Vector({14.269095499261483f, 0.626828232417541f});
	Matrix resultU, resultVt;
	Vector resultS;
	initial.SingularValueDecomposition(resultU, resultS, resultVt, 1000, 0.00000001f);

	COMPARE_MATRIX(resultU, expectU, 1.0f);
	COMPARE_VECTOR(resultS, expectS, 1.0f);
	COMPARE_MATRIX(resultVt, expectVt, 1.0f);
	result.Reassign(resultU * Matrix::Diagonal(resultS) * resultVt);
	COMPARE_MATRIX(result, initial, 1.0f);

	// UT::ReportInfo(__LINE__, "initial:\n", initial, "resultU:\n", resultU, "resultS: ", resultS, "\nresultVt:\n", resultVt, "result:\n", result);

	initial.Reassign(Matrix::Filled(4, 2, {
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
	}));
	expectU.Reassign(Matrix::Filled(2, 2, {
		0.376168234428141f, 0.926551379798884f,
		0.926551379798884f,-0.376168234428141f,
	}));
	expectVt.Reassign(Matrix::Filled(4, 2, {
		 0.352061692489013f, 0.443625782589520f, 0.535189872690028f, 0.626753962790535f,
		-0.758981267675146f,-0.321241599145932f, 0.116498069383282f, 0.554237737912496f,
	}));
	expectS.Reassign(Vector({14.227407412633742f, 1.25732983537911f}));

	initial.SingularValueDecomposition(resultU, resultS, resultVt, 1000, 0.00000001f);

	COMPARE_MATRIX(resultU, expectU, 1.0f);
	COMPARE_VECTOR(resultS, expectS, 1.0f);
	COMPARE_MATRIX(resultVt, expectVt, 1.0f);
	result.Reassign(resultU * Matrix::Diagonal(resultS) * resultVt);
	COMPARE_MATRIX(result, initial, 1.0f);

	// UT::ReportInfo(__LINE__, "initial:\n", initial, "resultU:\n", resultU, "resultS: ", resultS, "\nresultVt:\n", resultVt, "result:\n", result);

	fpError.Report(__LINE__);
}

} // namespace MatrixTestNamespace