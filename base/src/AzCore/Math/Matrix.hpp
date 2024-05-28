/*
	File: Matrix.hpp
	Author: Philip Haynes
	Dynamically-sized tensors.
*/

#ifndef AZCORE_MATRIX_HPP
#define AZCORE_MATRIX_HPP

#include "../basictypes.hpp"
#include "../Assert.hpp"
#include "../Memory/String.hpp" // For more helpful assert messages
#include "../Memory/RAIIHacks.hpp"
#include "../IO/Log.hpp"
#include "../Math/basic.hpp"
#include "../Math/vec2_t.hpp"
#include "../Math/vec3_t.hpp"
#include "../Math/vec4_t.hpp"
#include "../Math/vec5_t.hpp"
#include "../TemplateUtil.hpp"
#include "../Sort.hpp"

#include "../Simd.hpp"

#include <initializer_list>
#include <type_traits>

#define MATRIX_INFO_ARGS(obj) "Matrix<", az::TypeName<T>(), ((obj).capacity == 0 && (obj).Count() != 0 ? ">*(" : ">("), (obj).cols, ", ", (obj).rows, ")"
#define VECTOR_INFO_ARGS(obj) "Vector<", az::TypeName<T>(), ((obj).capacity == 0 && (obj).Count() != 0 ? ">*(" : ">("), (obj).count, ")"

namespace AzCore {

template<typename T>
struct Vector;

template<typename T>
struct Matrix;

template<typename T, i32 MAX_BYTES_ON_STACK=256>
struct MatrixWorkspace;

// Need to use a macro for alloca to work.
#define AZ_DECLARE_MATRIX_WORKSPACE(name, capacity, T) \
	az::MatrixWorkspace<T> name;\
	if (0 != capacity) {\
		name.Init((capacity) > az::MatrixWorkspace<T>::MAX_STACK_CAPACITY ? (T*)malloc((capacity) * sizeof( T )) : (T*)alloca((capacity) * sizeof(T)), (capacity));\
	}

} // namespace AzCore

template<typename T>
T dot(const az::Vector<T> &a, const az::Vector<T> &b);

template<typename T>
T normSqr(const az::Vector<T> &a);

template<typename T>
inline T norm(const az::Vector<T> &a) {
	return sqrt(normSqr(a));
}

namespace AzCore {

// transpose(const Matrix<T> &a): Gives you a transposed copy
// transpose(Matrix<T> *a): Gives you a transposed pointer (a col/row swapped view on the original where the original is unchanged)
template<typename T>
Matrix<T> transpose(const Matrix<T> &a);

template<typename T>
Matrix<T> transpose(Matrix<T> *a);

// Function-style Determinant
template<typename T>
inline T det(const Matrix<T> &a);

namespace Impl {

template<typename T>
using Operation = T (*)(const T &, const T &);

template<typename T>
force_inline(T) OperationMul(const T &a, const T &b) { return a * b; }
template<typename T>
force_inline(T) OperationDiv(const T &a, const T &b) { return a / b; }
template<typename T>
force_inline(T) OperationAdd(const T &a, const T &b) { return a + b; }
template<typename T>
force_inline(T) OperationSub(const T &a, const T &b) { return a - b; }
template<typename T>
force_inline(T) OperationPass(const T &a, const T &b) { return b; }

// Deferred calculation
template<typename T, Operation<T> operation, char opname, bool flipped>
struct VectorScalarOperation {
	Vector<T> vector;
	T scalar;
	explicit VectorScalarOperation(const Vector<T> &_vector, const T &_scalar) : vector(const_cast<Vector<T>*>(&_vector)), scalar(_scalar) {}
	explicit VectorScalarOperation(Vector<T> &&_vector, const T &_scalar) : vector(std::move(_vector)), scalar(_scalar) {}
	VectorScalarOperation(VectorScalarOperation &&) = delete;
	VectorScalarOperation(const VectorScalarOperation &) = delete;
	inline void VerifySizeEval(Vector<T> &dst) const {
		if constexpr (flipped) {
			AzAssert(dst.Count() == vector.Count(), Stringify("Error evaluating ", scalar, ' ', opname, ' ', VECTOR_INFO_ARGS(vector), " into ", VECTOR_INFO_ARGS(dst), ": Vectors are not the same size!"));
		} else {
			AzAssert(dst.Count() == vector.Count(), Stringify("Error evaluating ", VECTOR_INFO_ARGS(vector), ' ', opname, ' ', scalar, " into ", VECTOR_INFO_ARGS(dst), ": Vectors are not the same size!"));
		}
	}
	inline i32 RequiredCount() const {
		return vector.Count();
	}
	operator Vector<T> () {
		Vector<T> result(vector.Count());
		EvalInto(result);
		return result;
	}
	template<Operation<T> dstOp=OperationPass<T>>
	Vector<T>& EvalInto(Vector<T> &dst) {
		VerifySizeEval(dst);
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, dst.Count(), T);
		Vector<T> temp = workspace.GetVector(dst.Count());
		for (i32 i = 0; i < vector.Count(); i++) {
			if constexpr (flipped) {
				temp[i] = dstOp(dst[i], operation(scalar, vector[i]));
			} else {
				temp[i] = dstOp(dst[i], operation(vector[i], scalar));
			}
		}
		dst = temp;
		return dst;
	}
};
template<typename T, Operation<T> operation, char opname>
struct VectorVectorOperation {
	Vector<T> lhs, rhs;
	explicit VectorVectorOperation(Vector<T> &_lhs, Vector<T> &_rhs) : lhs(&_lhs), rhs(&_rhs) {
		VerifySizeDefine();
	}
	explicit VectorVectorOperation(Vector<T> &&_lhs, Vector<T> &_rhs) : lhs(std::move(_lhs)), rhs(&_rhs) {
		VerifySizeDefine();
	}
	explicit VectorVectorOperation(Vector<T> &&_lhs, Vector<T> &&_rhs) : lhs(std::move(_lhs)), rhs(std::move(_rhs)) {
		VerifySizeDefine();
	}
	explicit VectorVectorOperation(Vector<T> &_lhs, Vector<T> &&_rhs) : lhs(&_lhs), rhs(std::move(_rhs)) {
		VerifySizeDefine();
	}
	VectorVectorOperation(VectorVectorOperation &&) = delete;
	VectorVectorOperation(const VectorVectorOperation &) = delete;

	inline void VerifySizeDefine() const {
		AzAssert(lhs.Count() == rhs.Count(), Stringify("Error defining ", VECTOR_INFO_ARGS(lhs), ' ', opname, ' ', VECTOR_INFO_ARGS(rhs), ": Vectors are not the same size!"));
	}
	inline void VerifySizeEval(Vector<T> &dst) const {
		AzAssert(dst.Count() == lhs.Count(), Stringify("Error evaluating ", VECTOR_INFO_ARGS(lhs), ' ', opname, ' ', VECTOR_INFO_ARGS(rhs), " into ", VECTOR_INFO_ARGS(dst), ": Vectors are not the same size!"));
	}
	inline i32 RequiredCount() const {
		return lhs.Count();
	}
	operator Vector<T> () {
		Vector<T> result(lhs.Count());
		EvalInto(result);
		return result;
	}
	template<Operation<T> dstOp=OperationPass<T>>
	Vector<T>& EvalInto(Vector<T> &dst) const {
		VerifySizeEval(dst);
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, dst.Count(), T);
		Vector<T> temp = workspace.GetVector(dst.Count());
		for (i32 i = 0; i < lhs.Count(); i++) {
			temp[i] = dstOp(dst[i], operation(lhs[i], rhs[i]));
		}
		dst = temp;
		return dst;
	}
};

template<typename T, Operation<T> operation, char opname, bool flipped>
struct MatrixScalarOperation {
	Matrix<T> matrix;
	T scalar;
	explicit MatrixScalarOperation(Matrix<T> &_matrix, const T &_scalar) : matrix(&_matrix), scalar(_scalar) {}
	explicit MatrixScalarOperation(Matrix<T> &&_matrix, const T &_scalar) : matrix(std::move(_matrix)), scalar(_scalar) {}
	MatrixScalarOperation(MatrixScalarOperation &&) = delete;
	MatrixScalarOperation(const MatrixScalarOperation &) = delete;

	inline void VerifySizeEval(Matrix<T> &dst) const {
		if constexpr (flipped) {
			AzAssert(dst.Cols() == matrix.Cols() && dst.Rows() == matrix.Rows(), Stringify("Error evaluating ", scalar, ' ', opname, ' ', MATRIX_INFO_ARGS(matrix), " into ", MATRIX_INFO_ARGS(dst), ": Matrices are not the same size!"));
		} else {
			AzAssert(dst.Cols() == matrix.Cols() && dst.Rows() == matrix.Rows(), Stringify("Error evaluating ", MATRIX_INFO_ARGS(matrix), ' ', opname, ' ', scalar, " into ", MATRIX_INFO_ARGS(dst), ": Matrices are not the same size!"));
		}
	}
	inline i32 RequiredCols() const {
		return matrix.Cols();
	}
	inline i32 RequiredRows() const {
		return matrix.Rows();
	}
	operator Matrix<T> () {
		Matrix<T> result(matrix.Cols(), matrix.Rows());
		EvalInto(result);
		return result;
	}
	template<Operation<T> dstOp=OperationPass<T>>
	Matrix<T>& EvalInto(Matrix<T> &dst) {
		VerifySizeEval(dst);
		// It's possible for our src data to point to dst already, so we need to calculate the result before putting it into dst.
		// You'd think you could just let that be since you iterate one-by-one but matrix could be a transposed view of dst.
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, dst.Count(), T);
		Matrix<T> temp = workspace.GetMatrix(dst.Cols(), dst.Rows());
		for (i32 c = 0; c < matrix.Cols(); c++) {
			for (i32 r = 0; r < matrix.Rows(); r++) {
				if constexpr (flipped) {
					temp.Val(c,r) = dstOp(dst.Val(c,r), operation(scalar, matrix.Val(c,r)));
				} else {
					temp.Val(c,r) = dstOp(dst.Val(c,r), operation(matrix.Val(c,r), scalar));
				}
			}
		}
		dst = temp;
		return dst;
	}
};
template<typename T, bool flipped>
struct MatrixVectorMultiply {
	Matrix<T> matrix;
	Vector<T> vector;
	explicit MatrixVectorMultiply(Matrix<T> &_matrix, Vector<T> &_vector) : matrix(&_matrix), vector(&_vector) {
		VerifySizeDefine();
	}
	explicit MatrixVectorMultiply(Matrix<T> &&_matrix, Vector<T> &_vector) : matrix(std::move(_matrix)), vector(&_vector) {
		VerifySizeDefine();
	}
	explicit MatrixVectorMultiply(Matrix<T> &_matrix, Vector<T> &&_vector) : matrix(&_matrix), vector(std::move(_vector)) {
		VerifySizeDefine();
	}
	explicit MatrixVectorMultiply(Matrix<T> &&_matrix, Vector<T> &&_vector) : matrix(std::move(_matrix)), vector(std::move(_vector)) {
		VerifySizeDefine();
	}
	MatrixVectorMultiply(MatrixVectorMultiply &&) = delete;
	MatrixVectorMultiply(const MatrixVectorMultiply &) = delete;

	inline void VerifySizeDefine() const {
		if constexpr (flipped) {
			AzAssert(matrix.Rows() == vector.Count(), Stringify("Error defining ", VECTOR_INFO_ARGS(vector), " * ", MATRIX_INFO_ARGS(matrix), ": lhs.Count() must equal rhs.Rows()!"));
		} else {
			AzAssert(matrix.Cols() == vector.Count(), Stringify("Error defining ", MATRIX_INFO_ARGS(matrix), " * ", VECTOR_INFO_ARGS(vector), ": lhs.Cols() must equal rhs.Count()!"));
		}
	}
	inline void VerifySizeEval(Vector<T> &dst) const {
		if constexpr (flipped) {
			AzAssert(dst.Count() == matrix.Cols(), Stringify("Error evaluating ", VECTOR_INFO_ARGS(vector), " * ", MATRIX_INFO_ARGS(matrix), " into ", VECTOR_INFO_ARGS(dst), ": dst.Count() must equal rhs.Cols()!"));
		} else {
			AzAssert(dst.Count() == matrix.Rows(), Stringify("Error evaluating ", MATRIX_INFO_ARGS(matrix), " * ", VECTOR_INFO_ARGS(vector), " into ", VECTOR_INFO_ARGS(dst), ": dst.Count() must equal lhs.Rows()!"));
		}
	}
	inline i32 RequiredCount() const {
		if constexpr (flipped) {
			return matrix.Cols();
		} else {
			return matrix.Rows();
		}
	}
	operator Vector<T> () {
		i32 count;
		if constexpr (flipped) {
			count = matrix.Cols();
		} else {
			count = matrix.Rows();
		}
		Vector<T> result(count);
		EvalInto(result);
		return result;
	}
	template<Operation<T> dstOp=OperationPass<T>>
	Vector<T>& EvalInto(Vector<T> &dst) {
		VerifySizeEval(dst);
		// It's possible for our src data to point to dst already, so we need to calculate the result before putting it into dst.
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, dst.Count(), T);
		Vector<T> temp = workspace.GetVector(dst.Count());
		if constexpr (flipped) {
			for (i32 c = 0; c < matrix.Cols(); c++) {
				temp.Val(c) = dstOp(dst.Val(c), dot(matrix.Col(c), vector));
			}
		} else {
			for (i32 r = 0; r < matrix.Rows(); r++) {
				temp.Val(r) = dstOp(dst.Val(r), dot(vector, matrix.Row(r)));
			}
		}
		dst = temp;
		return dst;
	}
};
template<typename T>
struct MatrixMatrixMultiply {
	Matrix<T> lhs;
	Matrix<T> rhs;
	explicit MatrixMatrixMultiply(Matrix<T> &_lhs,  Matrix<T> &_rhs) : lhs(&_lhs), rhs(&_rhs) {
		VerifySizeDefine();
	}
	explicit MatrixMatrixMultiply(Matrix<T> &&_lhs,  Matrix<T> &_rhs) : lhs(std::move(_lhs)), rhs(&_rhs) {
		VerifySizeDefine();
	}
	explicit MatrixMatrixMultiply(Matrix<T> &_lhs,  Matrix<T> &&_rhs) : lhs(&_lhs), rhs(std::move(_rhs)) {
		VerifySizeDefine();
	}
	explicit MatrixMatrixMultiply(Matrix<T> &&_lhs,  Matrix<T> &&_rhs) : lhs(std::move(_lhs)), rhs(std::move(_rhs)) {
		VerifySizeDefine();
	}
	MatrixMatrixMultiply(MatrixMatrixMultiply &&) = delete;
	MatrixMatrixMultiply(const MatrixMatrixMultiply &) = delete;

	inline void VerifySizeDefine() const {
		AzAssert(lhs.Cols() == rhs.Rows(), Stringify("Error defining ", MATRIX_INFO_ARGS(lhs), " * ", MATRIX_INFO_ARGS(rhs), ": lhs.Cols() must equal rhs.Rows()!"));
	}
	inline void VerifySizeEval(Matrix<T> &dst) const {
		AzAssert(dst.Cols() == rhs.Cols() && dst.Rows() == lhs.Rows(), Stringify("Error evaluating ", MATRIX_INFO_ARGS(lhs), " * ", MATRIX_INFO_ARGS(rhs), " into ", MATRIX_INFO_ARGS(dst), ": dst.Cols() must equal rhs.Cols() and dst.Rows() must equal lhs.Rows()!"));
	}
	inline i32 RequiredCols() const {
		return rhs.Cols();
	}
	inline i32 RequiredRows() const {
		return lhs.Rows();
	}
	operator Matrix<T> () {
		Matrix<T> result(rhs.Cols(), lhs.Rows());
		EvalInto(result);
		return result;
	}
	template<Operation<T> dstOp=OperationPass<T>>
	Matrix<T>& EvalInto(Matrix<T> &dst) {
		VerifySizeEval(dst);
		// It's possible for our src data to point to dst already, so we need to calculate the result before putting it into dst.
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, dst.Count(), T);
		// Matrix<T> temp = &dst;
		Matrix<T> temp = workspace.GetMatrix(dst.Cols(), dst.Rows());
		for (i32 c = 0; c < rhs.Cols(); c++) {
			for (i32 r = 0; r < lhs.Rows(); r++) {
				temp.Val(c,r) = dstOp(dst.Val(c,r), dot(lhs.Row(r), rhs.Col(c)));
			}
		}
		dst = temp;
		return dst;
	}
};

} // namespace Impl
} // namespace AzCore

template<typename T>
az::Vector<T>& operator+=(az::Vector<T> &lhs, const az::Vector<T> &rhs);

template<typename T>
az::Vector<T>& operator-=(az::Vector<T> &lhs, const az::Vector<T> &rhs);

template<typename T, az::Impl::Operation<T> operation, char opname, bool flipped>
az::Vector<T>& operator*=(az::Vector<T> &lhs, az::Impl::VectorScalarOperation<T, operation, opname, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationMul<T>>(lhs);
	return lhs;
}
template<typename T, az::Impl::Operation<T> operation, char opname, bool flipped>
az::Vector<T>& operator/=(az::Vector<T> &lhs, az::Impl::VectorScalarOperation<T, operation, opname, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationDiv<T>>(lhs);
	return lhs;
}
template<typename T, az::Impl::Operation<T> operation, char opname, bool flipped>
az::Vector<T>& operator+=(az::Vector<T> &lhs, az::Impl::VectorScalarOperation<T, operation, opname, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationAdd<T>>(lhs);
	return lhs;
}
template<typename T, az::Impl::Operation<T> operation, char opname, bool flipped>
az::Vector<T>& operator-=(az::Vector<T> &lhs, az::Impl::VectorScalarOperation<T, operation, opname, flipped> &&rhs) {
	rhs.template EvalInto<az::Impl::OperationSub<T>>(lhs);
	return lhs;
}

template<typename T, az::Impl::Operation<T> operation, char opname>
az::Vector<T>& operator*=(az::Vector<T> &lhs, az::Impl::VectorVectorOperation<T, operation, opname> &&rhs) {
	rhs.EvalInto<az::Impl::OperationMul<T>>(lhs);
	return lhs;
}
template<typename T, az::Impl::Operation<T> operation, char opname>
az::Vector<T>& operator/=(az::Vector<T> &lhs, az::Impl::VectorVectorOperation<T, operation, opname> &&rhs) {
	rhs.EvalInto<az::Impl::OperationDiv<T>>(lhs);
	return lhs;
}
template<typename T, az::Impl::Operation<T> operation, char opname>
az::Vector<T>& operator+=(az::Vector<T> &lhs, az::Impl::VectorVectorOperation<T, operation, opname> &&rhs) {
	rhs.EvalInto<az::Impl::OperationAdd<T>>(lhs);
	return lhs;
}
template<typename T, az::Impl::Operation<T> operation, char opname>
az::Vector<T>& operator-=(az::Vector<T> &lhs, az::Impl::VectorVectorOperation<T, operation, opname> &&rhs) {
	rhs.EvalInto<az::Impl::OperationSub<T>>(lhs);
	return lhs;
}

template<typename T, bool flipped>
az::Vector<T>& operator*=(az::Vector<T> &lhs, az::Impl::MatrixVectorMultiply<T, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationMul<T>>(lhs);
	return lhs;
}
template<typename T, bool flipped>
az::Vector<T>& operator/=(az::Vector<T> &lhs, az::Impl::MatrixVectorMultiply<T, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationDiv<T>>(lhs);
	return lhs;
}
template<typename T, bool flipped>
az::Vector<T>& operator+=(az::Vector<T> &lhs, az::Impl::MatrixVectorMultiply<T, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationAdd<T>>(lhs);
	return lhs;
}
template<typename T, bool flipped>
az::Vector<T>& operator-=(az::Vector<T> &lhs, az::Impl::MatrixVectorMultiply<T, flipped> &&rhs) {
	rhs.EvalInto<az::Impl::OperationSub<T>>(lhs);
	return lhs;
}


template<typename T>
az::Vector<T>& operator+=(az::Vector<T> &lhs, const az::Vector<T> &rhs);

template<typename T>
az::Vector<T>& operator-=(az::Vector<T> &lhs, const az::Vector<T> &rhs);

template<typename T>
az::Vector<T>& operator*=(az::Vector<T> &lhs, const T &rhs);

template<typename T>
az::Vector<T>& operator/=(az::Vector<T> &lhs, const T &rhs);

template<typename T>
az::Vector<T>&& operator+=(az::Vector<T> &&lhs, const az::Vector<T> &rhs) {
	return std::move(lhs += rhs);
}

template<typename T>
az::Vector<T>&& operator-=(az::Vector<T> &&lhs, const az::Vector<T> &rhs) {
	return std::move(lhs -= rhs);
}

template<typename T>
az::Vector<T>&& operator*=(az::Vector<T> &&lhs, const T &rhs) {
	return std::move(lhs *= rhs);
}

template<typename T>
az::Vector<T>&& operator/=(az::Vector<T> &&lhs, const T &rhs) {
	return std::move(lhs /= rhs);
}

template<typename T>
az::Matrix<T>& operator*=(az::Matrix<T> &lhs, const T &rhs);

template<typename T>
az::Matrix<T>& operator/=(az::Matrix<T> &lhs, const T &rhs);

template<typename T>
az::Matrix<T>& operator+=(az::Matrix<T> &lhs, const az::Matrix<T> &rhs);

template<typename T>
az::Matrix<T>& operator-=(az::Matrix<T> &lhs, const az::Matrix<T> &rhs);

namespace AzCore {

// A memory pool for allocating Matrices and Vectors where you tell it ahead of time how much space you'll need to do all the calculations you need to do. Useful for intermediate steps in an algorithm. Note that this structure MUST be on the stack and not on the heap.
template<typename T, i32 MAX_BYTES_ON_STACK>
struct MatrixWorkspace {
	static constexpr i32 MAX_STACK_CAPACITY = MAX_BYTES_ON_STACK / sizeof(T);
	T *data;
	i32 capacity;
	i32 taken;

	MatrixWorkspace() = default;
	MatrixWorkspace(const MatrixWorkspace &) = delete;
	MatrixWorkspace(MatrixWorkspace &&) = delete;
	// DO NOT guess what capacity you need. Calculate it exactly!
	void Init(T *_data, i32 _capacity) {
		data = _data;
		capacity = _capacity;
		taken = 0;
		for (i32 i = 0; i < capacity; i++) {
			AzPlacementNew(data[i]);
		}
	}
	~MatrixWorkspace() {
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (i32 i = 0; i < capacity; i++) {
				data[i].~T();
			}
		}
		if (capacity > MAX_STACK_CAPACITY) {
			free(data);
		}
	}
	T* Get(i32 count) {
		AzAssert(taken+count <= capacity, Stringify("MatrixWorkspace ran out of storage! (Tried to get ", count, " more values when ", taken, " are taken and we have a capacity of ", capacity, "."));
		T* result = data + taken;
		taken += count;
		return result;
	}
	inline Vector<T> GetVector(i32 count) {
		return Vector<T>(Get(count), count);
	}
	inline Matrix<T> GetMatrix(i32 cols, i32 rows) {
		i32 count = cols * rows;
		return Matrix<T>(Get(count), cols, rows);
	}
	inline Vector<T> GetVectorCopy(const Vector<T> &src) {
		Vector<T> result = Vector<T>(Get(src.Count()), src.Count());
		result = src;
		return result;
	}
	inline Matrix<T> GetMatrixCopy(const Matrix<T> &src) {
		Matrix<T> result = Matrix<T>(Get(src.Count()), src.Cols(), src.Rows());
		result = src;
		return result;
	}
};

template<typename T>
struct Vector {
	using Scalar_t = T;
	T *data;
	u16 count, stride;
	u16 capacity;

	inline void AssertValid() const {
		AzAssert(data != nullptr, Stringify("Vector<", TypeName<T>(), "> is null."));
	}

	~Vector() {
		if (capacity) delete[] data;
	}
	constexpr Vector() : data(nullptr), count(0), stride(0), capacity(0) {}

	constexpr Vector(i32 _count) :
		data(_count > 0 ? new T[_count] : nullptr),
		count(_count), stride(1), capacity(_count) {}

	constexpr Vector(T *_data, i32 _count, i32 _stride=1) : data(_data), count(_count), stride(_stride), capacity(0) {}

	constexpr Vector(const Vector &other) :
		data(other.count > 0 ? ArrayNewCopy(other.count, other.data, other.stride) : nullptr),
		count(other.count), stride(1), capacity(other.count) {}

	constexpr Vector(Vector &&other) : data(other.data), count(other.count), stride(other.stride), capacity(other.capacity) {
		other.capacity = 0;
	}
	constexpr Vector(Vector *other) : data(other->data), count(other->count), stride(other->stride), capacity(0) {}

	constexpr Vector(std::initializer_list<T> list) : data(new T[list.size()]), count(list.size()), stride(1), capacity(list.size()) {
		i32 i = 0;
		for (const T &it : list) {
			data[i] = it;
			i++;
		}
	}

	constexpr Vector(i32 _count, const T &value) : data(_count ? new T[_count] : nullptr), count(_count), stride(1), capacity(_count) {
		for (i32 i = 0; i < _count; i++) {
			data[i] = value;
		}
	}

	void ResetToValue(const T &value) {
		for (i32 i = 0; i < count; i++) {
			data[i] = value;
		}
	}

	void Resize(i32 _count) {
		if (_count == count) return;
		if (count != 0) {
			AzAssert(capacity != 0, Stringify(VECTOR_INFO_ARGS(*this), ".Resize(", _count, ") error: Pointer Vectors cannot be resized!"));
		}
		MakeOwnedWithSize(_count);
	}

	// Like Resize, but ensures we're not a pointer
	void MakeOwnedWithSize(i32 _count) {
		if (capacity < _count) {
			T *newData = new T[_count];
			for (i32 i = 0; i < count; i++) {
				newData[i] = std::move(data[i]);
			}
			if (capacity) {
				delete[] data;
			}
			data = newData;
			capacity = _count;
			stride = 1;
		}
		count = _count;
	}
	// returns the old data pointer (or nullptr if we didn't need to do anything)
	// You must delete[] it yourself.
	T* MakeOwnedWithSizeDeferredDelete(i32 _count) {
		T *result = nullptr;
		if (capacity < _count) {
			T *newData = new T[_count];
			for (i32 i = 0; i < count; i++) {
				newData[i] = std::move(data[i]);
			}
			if (capacity) {
				result = data;
			}
			data = newData;
			capacity = _count;
			stride = 1;
		}
		count = _count;
		return result;
	}

	// since operator= is used for element-wise assignment, this enables us to change what this Vector represents.
	Vector& Reassign(Vector *other) {
		if (capacity != 0) {
			delete[] data;
		}
		data = other.data;
		count = other.count;
		stride = other.stride;
		capacity = 0;
		return *this;
	}
	// since operator= is used for element-wise assignment, this enables us to change what this Vector represents.
	Vector& Reassign(const Vector &other) {
		if (capacity) {
			if (capacity < other.Count()) {
				delete[] data;
				data = ArrayNewCopy(other.Count(), other.data);
				capacity = other.count;
			} else {
				for (i32 i = 0; i < other.Count(); i++) {
					data[i] = other.data[i];
				}
			}
		} else if (other.Count() != 0) {
			data = ArrayNewCopy(other.Count(), other.data);
			capacity = other.count;
		}
		count = other.count;
		return *this;
	}
	// since operator= is used for element-wise assignment, this enables us to change what this Vector represents.
	Vector& Reassign(Vector &&other) {
		if (capacity != 0) {
			delete[] data;
		}
		data = other.data;
		count = other.count;
		stride = other.stride;
		capacity = other.capacity;
		other.capacity = 0;
		return *this;
	}

	Vector& operator=(const Vector &other) {
		AzAssert(Count() == other.Count(), Stringify(VECTOR_INFO_ARGS(*this), " assigning to ", VECTOR_INFO_ARGS(other), " error: Vectors must be the same size!"));
		for (i32 i = 0; i < Count(); i++) {
			(*this)[i] = other[i];
		}
		return *this;
	}

	template<Impl::Operation<T> operation, char opname, bool flipped>
	Vector<T>& Reassign(const Impl::VectorScalarOperation<T, operation, opname, flipped> &rhs) {
		T *oldData = MakeOwnedWithSizeDeferredDelete(rhs.RequiredCount());
		rhs.EvalInto(*this);
		if (oldData) delete[] oldData;
		return *this;
	}
	template<Impl::Operation<T> operation, char opname, bool flipped>
	Vector<T>& operator=(const Impl::VectorScalarOperation<T, operation, opname, flipped> &rhs) {
		rhs.EvalInto(*this);
		return *this;
	}
	template<Impl::Operation<T> operation, char opname>
	Vector<T>& Reassign(const Impl::VectorVectorOperation<T, operation, opname> &rhs) {
		T *oldData = MakeOwnedWithSizeDeferredDelete(rhs.RequiredCount());
		rhs.EvalInto(*this);
		if (oldData) delete[] oldData;
		return *this;
	}
	template<Impl::Operation<T> operation, char opname>
	Vector<T>& operator=(const Impl::VectorVectorOperation<T, operation, opname> &rhs) {
		rhs.EvalInto(*this);
		return *this;
	}

	template<bool flipped>
	Vector<T>& Reassign(Impl::MatrixVectorMultiply<T, flipped> &&rhs) {
		T *oldData = MakeOwnedWithSizeDeferredDelete(rhs.RequiredCount());
		rhs.EvalInto(*this);
		if (oldData) delete[] oldData;
		return *this;
	}
	template<bool flipped>
	Vector<T>& operator=(Impl::MatrixVectorMultiply<T, flipped> &&rhs) {
		rhs.EvalInto(*this);
		return *this;
	}

	i32 Count() const {
		return count;
	}

	vec2_t<T>& AsVec2() const {
		AssertValid();
		AzAssert(count == 2, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec2_t<", TypeName<T>(), ">& error: Count (", count, ") is incorrect."));
		AzAssert(stride == 1, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec2_t<", TypeName<T>(), ">& error: stride (", stride, ") must be 1."));
		return *(vec2_t<T>*)data;
	};

	vec3_t<T>& AsVec3() const {
		AssertValid();
		AzAssert(count == 3, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec3_t<", TypeName<T>(), ">& error: Count (", count, ") is incorrect."));
		AzAssert(stride == 1, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec3_t<", TypeName<T>(), ">& error: stride (", stride, ") must be 1."));
		return *(vec3_t<T>*)data;
	};

	vec4_t<T>& AsVec4() const {
		AssertValid();
		AzAssert(count == 4, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec4_t<", TypeName<T>(), ">& error: Count (", count, ") is incorrect."));
		AzAssert(stride == 1, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec4_t<", TypeName<T>(), ">& error: stride (", stride, ") must be 1."));
		return *(vec4_t<T>*)data;
	};

	vec5_t<T>& AsVec5() const {
		AssertValid();
		AzAssert(count == 3, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec5_t<", TypeName<T>(), ">& error: Count (", count, ") is incorrect."));
		AzAssert(stride == 1, Stringify("Conversion of ", VECTOR_INFO_ARGS(*this), " to a vec5_t<", TypeName<T>(), ">& error: stride (", stride, ") must be 1."));
		return *(vec5_t<T>*)data;
	};

	T& Val(i32 index) {
		AssertValid();
		AzAssert(index >= 0 && index < (i32)count, Stringify("Index ", index, " is out of bounds for ", VECTOR_INFO_ARGS(*this)));
		return data[index * stride];
	}
	const T& Val(i32 index) const {
		return const_cast<Vector*>(this)->Val(index);
	}

	T& operator[](i32 index) {
		return Val(index);
	}
	const T& operator[](i32 index) const {
		return Val(index);
	}

	Vector& Normalize() {
		T mag = norm(*this);
		for (i32 i = 0; i < Count(); i++) {
			Val(i) /= mag;
		}
		return *this;
	}

	Vector& Orthogonalize(const Vector &other) {
		AzAssert(Count() == other.Count(), Stringify("Orthogonalizing ", VECTOR_INFO_ARGS(*this), " against ", VECTOR_INFO_ARGS(other), " error: Vectors must be the same size!"));
		T d = dot(*this, other);
		*this -= other * d;
		return *this;
	}
};

template<typename T>
struct Matrix {
	using Scalar_t = T;
	T *data;
	u16 cols, rows;
	u8 colStride, rowStride;
	u16 capacity;

	inline void AssertValid() const {
		AzAssert(data != nullptr, Stringify(MATRIX_INFO_ARGS(*this), " is null!"));
		AzAssert(cols > 0 && rows > 0, Stringify(MATRIX_INFO_ARGS(*this), " is not a valid matrix."));
	}

	~Matrix() {
		if (capacity) delete[] data;
	}
	constexpr Matrix() : data(nullptr), cols(0), rows(0), colStride(0), rowStride(0), capacity(0) {}

	constexpr Matrix(i32 _cols, i32 _rows) :
		data(_cols > 0 && _rows > 0 ? new T[_cols * _rows] : nullptr),
		cols(_cols), rows(_rows), colStride(_rows), rowStride(1), capacity(_cols * _rows) {}

	constexpr Matrix(T *_data, i32 _cols, i32 _rows) : data(_data), cols(_cols), rows(_rows), colStride(_rows), rowStride(1), capacity(0) {}

	constexpr Matrix(T *_data, i32 _cols, i32 _rows, i32 _colStride, i32 _rowStride) : data(_data), cols(_cols), rows(_rows), colStride(_colStride), rowStride(_rowStride), capacity(0) {}

	constexpr Matrix(const Matrix &other) :
		data(other.cols > 0 && other.rows > 0 ? ArrayNewCopy2D(other.Rows(), other.Cols(), other.data, other.rowStride, other.colStride) : nullptr),
		cols(other.cols), rows(other.rows), colStride(other.rows), rowStride(1), capacity(rows * cols) {}

	constexpr Matrix(Matrix &&other) : data(other.data), cols(other.cols), rows(other.rows), colStride(other.colStride), rowStride(other.rowStride), capacity(other.capacity) {
		other.capacity = 0;
	}
	constexpr Matrix(Matrix *other) : data(other->data), cols(other->cols), rows(other->rows), colStride(other->colStride), rowStride(other->rowStride), capacity(0) {}

	void ResetToIdentity() {
		for (i32 c = 0; c < Cols(); c++) {
			for (i32 r = 0; r < Rows(); r++) {
				Val(c,r) = c == r ? T(1) : T(0);
			}
		}
	}

	static Matrix Identity(i32 size) {
		Matrix result(size, size);
		result.ResetToIdentity();
		return result;
	}
	static Matrix Diagonal(std::initializer_list<T> init) {
		Matrix result(init.size(), init.size());
		auto it = init.begin();
		for (i32 c = 0; c < init.size(); c++) {
			for (i32 r = 0; r < init.size(); r++) {
				result[c][r] = c == r ? *(it++) : T(0);
			}
		}
		return result;
	}
	static Matrix Diagonal(const Vector<T> &vector, i32 _cols=-1, i32 _rows=-1) {
		if (_cols == -1) _cols = vector.Count();
		if (_rows == -1) _rows = vector.Count();
		AzAssert(min(_cols, _rows) <= vector.Count(), Stringify(VECTOR_INFO_ARGS(vector), " too small to fill the diagonals of a ", _cols, " x ", _rows, " matrix."));
		Matrix result(_cols, _rows);
		for (i32 c = 0; c < _cols; c++) {
			for (i32 r = 0; r < _rows; r++) {
				result.Val(c, r) = c == r ? vector[c] : T(0);
			}
		}
		return result;
	}
	static Matrix Filled(i32 _cols, i32 _rows, std::initializer_list<T> init) {
		AzAssert(_cols * _rows == (i32)init.size(), Stringify("Expected _cols * _rows to equal the initializer_list size (_cols = ", _cols, ", _rows = ", _rows, ", size = ", init.size(), ")"));
		Matrix result(_cols, _rows);
		i32 r = 0, c = 0;
		for (const T &value : init) {
			result.Val(c, r) = value;
			c++;
			if (c >= _cols) {
				c = 0;
				r++;
			}
		}
		return result;
	}
	static Matrix Filled(i32 _cols, i32 _rows, T value) {
		Matrix result(_cols, _rows);
		for (i32 i = 0; i < result.Count(); i++) {
			result.data[i] = value;
		}
		return result;
	}

	void Resize(i32 _cols, i32 _rows) {
		if (cols == _cols && rows == _rows) return;
		if (cols != 0 && rows != 0 && (cols < _cols || rows < _rows)) {
			AzAssert(capacity != 0, Stringify(MATRIX_INFO_ARGS(*this), ".Resize(", _cols, ", ", _rows, ") error: Pointer Matrices can only be shrunk!"));
		}
		MakeOwnedWithSize(_cols, _rows);
	}

	void MakeOwnedWithSize(i32 _cols, i32 _rows) {
		i32 _count = _cols * _rows;
		if (capacity < _count) {
			T *newData = new T[_count];
			for (i32 c = 0; c < min(Cols(), _cols); c++) {
				for (i32 r = 0; r < min(Rows(), _rows); r++) {
					newData[c * _rows + r] = std::move(Val(c, r));
				}
			}
			if (capacity) {
				delete[] data;
			}
			data = newData;
			capacity = _count;
		}
		cols = _cols;
		rows = _rows;
		colStride = _rows;
		rowStride = 1;
	}

	T* MakeOwnedWithSizeDeferredDelete(i32 _cols, i32 _rows) {
		T *result = nullptr;
		i32 _count = _cols * _rows;
		if (capacity < _count) {
			T *newData = new T[_count];
			for (i32 c = 0; c < min(Cols(), _cols); c++) {
				for (i32 r = 0; r < min(Rows(), _rows); r++) {
					newData[c * _rows + r] = std::move(Val(c, r));
				}
			}
			if (capacity) {
				result = data;
			}
			data = newData;
			capacity = _count;
		}
		cols = _cols;
		rows = _rows;
		colStride = _rows;
		rowStride = 1;
		return result;
	}

	void MakeOwned() {
		if (capacity == 0) {
			Reassign(*this);
		}
	}

	Matrix& Reassign(Matrix *other) {
		if (capacity) {
			delete[] data;
		}
		data = other->data;
		cols = other->cols;
		rows = other->rows;
		colStride = other->colStride;
		rowStride = other->rowStride;
		capacity = 0;
		return *this;
	}
	// Makes a new copy of other (so we're no longer pointing at anything else if we were before).
	Matrix& Reassign(const Matrix &other) {
		if (capacity) {
			if (capacity < other.Count()) {
				delete[] data;
				data = ArrayNewCopy2D(other.Rows(), other.Cols(), other.data, other.rowStride, other.colStride);
				capacity = other.Count();
			} else {
				for (i32 i = 0; i < other.Count(); i++) {
					data[i] = other.data[i];
				}
			}
		} else if (other.Count() != 0) {
			data = ArrayNewCopy2D(other.Rows(), other.Cols(), other.data, other.rowStride, other.colStride);
			capacity = other.Count();
		}
		cols = other.cols;
		rows = other.rows;
		colStride = other.rows;
		rowStride = 1;
		return *this;
	}
	Matrix& Reassign(Matrix &&other) {
		if (capacity) {
			delete[] data;
		}
		data = other.data;
		cols = other.cols;
		rows = other.rows;
		colStride = other.colStride;
		rowStride = other.rowStride;
		capacity = other.capacity;
		other.capacity = 0;
		return *this;
	}

	Matrix& operator=(const Matrix &other) {
		AzAssert(Cols() == other.Cols() && Rows() == other.Rows(), Stringify(MATRIX_INFO_ARGS(*this), " assigning to ", MATRIX_INFO_ARGS(other), " error: Matrices must be the same size! Did you mean to call Reassign()?"));
		for (i32 c = 0; c < Cols(); c++) {
			for (i32 r = 0; r < Rows(); r++) {
				Val(c, r) = other.Val(c, r);
			}
		}
		return *this;
	}

	Matrix<T>& Reassign(Impl::MatrixMatrixMultiply<T> &&rhs) {
		T *oldData = MakeOwnedWithSizeDeferredDelete(rhs.RequiredCols(), rhs.RequiredRows());
		rhs.EvalInto(*this);
		if (oldData) delete[] oldData;
		return *this;
	}
	Matrix<T>& operator=(Impl::MatrixMatrixMultiply<T> &&rhs) {
		rhs.EvalInto(*this);
		return *this;
	}

	i32 Count() const {
		return (i32)cols * (i32)rows;
	}
	i32 Cols() const {
		return cols;
	}
	i32 Rows() const {
		return rows;
	}

	Matrix<T> SubMatrix(i32 col, i32 row, i32 nCols, i32 nRows, i32 colStep=1, i32 rowStep=1) const {
		AzAssert(col + (nCols-1)*colStep < Cols(), Stringify("SubMatrix starting at col ", col, ", with ", nCols, " cols and a step of ", colStep, " extends beyond the bounds of the existing Matrix (with ", cols, " cols)."));
		AzAssert(row + (nRows-1)*rowStep < Rows(), Stringify("SubMatrix starting at row ", row, ", with ", nRows, " rows and a step of ", rowStep, " extends beyond the bounds of the existing Matrix (with ", rows, " rows)."));
		Matrix<T> result(data + row * rowStride + col * colStride, nCols, nRows, colStride * colStep, rowStride * rowStep);
		return result;
	}

	// Returns a pointer Vector which will behave like a reference
	Vector<T> Col(i32 index) const {
		AssertValid();
		AzAssert(index >= 0 && index < (i32)cols, Stringify("Column ", index, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		Vector<T> result(data + index * colStride, rows, rowStride);
		return result;
	}

	// Returns a pointer Vector which will behave like a reference
	Vector<T> Row(i32 index) const {
		AssertValid();
		AzAssert(index >= 0 && index < (i32)rows, Stringify("Row ", index, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		Vector<T> result(data + index * rowStride, cols, colStride);
		return result;
	}

	// Returns a pointer Vector which represents the diagonal
	Vector<T> Diag() const {
		AssertValid();
		Vector<T> result(data, min(cols, rows), colStride + rowStride);
		return result;
	}

	inline Vector<T> operator[](i32 index) const {
		return Col(index);
	}

	inline T& Val(i32 col, i32 row) const {
		AssertValid();
		AzAssert(col >= 0 && col < Cols(), Stringify("Column ", col, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		AzAssert(row >= 0 && row < Rows(), Stringify("Row ", row, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		return data[col * colStride + row * rowStride];
	}

	inline T& ValLessColAndRow(i32 col, i32 row, i32 colRemoved, i32 rowRemoved) const {
		AssertValid();
		AzAssert(colRemoved >= 0 && colRemoved < Cols(), Stringify("Removed Column ", colRemoved, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		AzAssert(rowRemoved >= 0 && rowRemoved < Rows(), Stringify("Removed Row ", rowRemoved, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		AzAssert(col >= 0 && col < Cols()-1, Stringify("Column ", col, " is out of bounds for ", MATRIX_INFO_ARGS(*this), " with Column ", colRemoved, " removed."));
		AzAssert(row >= 0 && row < Rows()-1, Stringify("Row ", row, " is out of bounds for ", MATRIX_INFO_ARGS(*this), " with Row ", rowRemoved, " removed."));
		col += i32(col >= colRemoved);
		row += i32(row >= rowRemoved);
		return data[col * colStride + row * rowStride];
	}

	// Flip the matrix along its diagonal in-place.
	Matrix& Transpose() {
		if (capacity == 0) {
			AzAssert(Cols() == Rows(), Stringify(MATRIX_INFO_ARGS(*this), " Transpose error: We're pointing to another Matrix, so we must be a square Matrix, because we can't reshape the target."));
		}
		// Skip the first and last indices because they never move.
		i32 start = 1;
		static thread_local ArrayWithBucket<bool, 16> touched;
		touched.ClearSoft();
		touched.Resize(Count(), false);
		T hold;
		for (i32 i = 1; i < Count()-1;) {
			i32 row = i % Rows();
			i32 col = i / Rows();
			i32 next = row * Cols() + col;
			if (next != start || i != start) {
				Swap(hold, Val(col, row));
			}
			if (touched[i]) {
				do {
					i++;
				} while (touched[i]);
				start = i;
			} else {
				touched[i] = true;
				i = next;
			}
		}
		colStride = colStride * cols / rows;
		Swap(cols, rows);
		return *this;
	}

	// Swaps the cols and rows without actually moving any data. This is less work, but only makes sense when this matrix has no pointers pointing to it as any Matrix pointing to our data will be unaffected (where Transpose() would affect them).
	Matrix& TransposeSoft() {
		Swap(colStride, rowStride);
		Swap(cols, rows);
		return *this;
	}

	// The Minor is the Determinant of the Sub-Matrix where the given row and column are removed.
	T Minor(i32 col, i32 row) const {
		AssertValid();
		AzAssert(Cols() == Rows() && Cols() > 1, Stringify(MATRIX_INFO_ARGS(*this), " error: The minor is only defined for square matrices with at least 2 rows and columns."));
		AzAssert(col < Cols() && row < Rows(), Stringify(MATRIX_INFO_ARGS(*this), " Minor error: Attempted to remove col ", col, ", row ", row, " which is out of bounds."));
		switch (Cols()-1) {
			case 1: {
				return Val(1-col,1-row);
			} break;
			case 2: {
				return ValLessColAndRow(0,0, col,row)*ValLessColAndRow(1,1, col,row)
				     - ValLessColAndRow(1,0, col,row)*ValLessColAndRow(0,1, col,row);
			} break;
			default: {
				AzAssertRel(false, "Unimplemented because it's hard :(");
				return T();
			} break;
		}
	}

	T Determinant() const {
		AssertValid();
		AzAssert(Cols() == Rows(), Stringify(MATRIX_INFO_ARGS(*this), " error: The determinant is only defined for square matrices."));
		switch (Cols()) {
			case 1: {
				return Val(0,0);
			} break;
			case 2: {
				return Val(0,0)*Val(1,1) - Val(1,0)*Val(0,1);
			} break;
			case 3: {
				return Val(0,0)*Minor(0,0) - Val(1,0)*Minor(1,0) + Val(2,0)*Minor(2,0);
			} break;
			default: {
				AzAssertRel(false, "Unimplemented because it's hard :(");
				return T();
			} break;
		}
	}

	// Get the Cofactor Matrix (all the cofactors of this matrix).
	Matrix CofactorMatrix() const {
		AzAssert(Cols() == Rows(), Stringify(MATRIX_INFO_ARGS(*this), " error: Cofactors are only defined for square matrices."));
		Matrix result(Cols(), Rows());
		for (i32 c = 0; c < Cols(); c++) {
			for (i32 r = 0; r < Rows(); r++) {
				result.Val(c, r) = Minor(c, r);
				if ((c+r)&1) {
					result.Val(c, r) = -result.Val(c, r);
				}
			}
		}
		return result;
	}

	// Get the Adjugate Matrix (the transpose of the Cofactor Matrix).
	inline Matrix Adjugate() const {
		AzAssert(Cols() == Rows(), Stringify(MATRIX_INFO_ARGS(*this), " error: The adjugate is only defined for square matrices."));
		Matrix result = CofactorMatrix();
		result.TransposeSoft();
		return result;
	}

	// You can specify a minimum magnitude for the determinant for when it's small.
	Matrix Inverse() const {
		AzAssert(Cols() == Rows(), Stringify(MATRIX_INFO_ARGS(*this), " error: The inverse is only defined for square matrices."));
		T determinant = Determinant();
		// if (abs(determinant) < detMinMag) {
		// 	determinant = detMinMag * sign(determinant);
		// }
		AzAssert(determinant != T(0), Stringify(MATRIX_INFO_ARGS(*this), " error: The determinant is 0, therefore we are not invertible:\n", *this));
		Matrix result = Adjugate();
		for (i32 c = 0; c < Cols(); c++) {
			for (i32 r = 0; r < Rows(); r++) {
				result.Val(c, r) /= determinant;
			}
		}
		return result;
	}

	/*
		Rant time:
		Most math literature refers to "orthogonal" matrices as being defined by their transpose equaling their inverse. That is, all basis vectors within the matrix are orthogonal to each other AND unit-length.
		"Orthogonal" in terms of vectors is a weaker guarantee, ensuring only that two "orthogonal" vectors are perpendicular to each other.
		"Orthonormal" is a stronger guarantee that states two vectors are orthogonal to each other AND unit-length.
		Would it not make infinitely more sense for the same pattern to hold for matrices?
		Define "orthogonal matrix" to mean all basis vectors are orthogonal to each other.
		Define "orthonormal matrix" to mean all basis vectors are orthogonal to each other AND unit-length.
		With this more consistent terminology, a matrix inverse would only equal its transpose when it is "orthonormal" and NOT when it's otherwise "orthogonal".
	*/

	// m = min(cols, rows)
	// Q is a    m x rows orthonormal matrix
	// R is a cols x m    upper-triangular matrix
	// Q * R is our original matrix
	void QRDecomposition(Matrix &Q, Matrix &R) const {
		// Uses the Gram-Schmidt process as described here: https://www.math.ucla.edu/~yanovsky/Teaching/Math151B/handouts/GramSchmidt.pdf
		i32 m = min(cols, rows);
		Q.Resize(m, Rows());
		R.Resize(Cols(), m);
	#if 0
		// Work forward orthogonalizing each basis vector against all previous basis vectors
		for (i32 c = 0; c < m; c++) {
			Vector<T> basis = Q.Col(c);
			basis = Col(c);
			// Orthogonalize against all previous orthogonal basis vectors
			for (i32 r = 0; r < c; r++) {
				Vector<T> basisPrev = Q.Col(r);
				// This also gives us all entries above the diagonal in R
				T &dstDot = R[c][r];
				dstDot = dot(basis, basisPrev);
				basis -= basisPrev * dstDot;
			}
			basis.Normalize();
			R[c][c] = dot(basis, Col(c));
			// Zero the bottom triangle of R
			for (i32 r = c+1; r < m; r++) {
				R[c][r] = T(0);
			}
		}
	#else
		// Modified Gram-Schmidt that has better numerical stability at the cost of increased memory usage
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, m * Q.Rows(), T);
		Matrix V = workspace.GetMatrix(m, Q.Rows());
		for (i32 c = 0; c < m; c++) {
			V.Col(c) = Col(c);
		}
		for (i32 c = 0; c < m; c++) {
			Vector<T> basis = Q.Col(c);
			basis = V.Col(c);
			T &mag = R[c][c];
			mag = norm(basis);
			if (mag > T(1.0e-12)) {
				basis /= mag;
			} else {
				// Contingency for weird degenerate matrices
				for (i32 i = 0; i < basis.Count(); i++) {
					basis[i] = i == c ? T(1) : T(0);
				}
				if (c > 0) {
					basis.Orthogonalize(Q.Col(c-1)).Normalize();
				}
			}
			// Orthogonalize ahead of time
			for (i32 r = c+1; r < m; r++) {
				Vector<T> basisNext = V.Col(r);
				// This also gives us all entries above the diagonal in R
				T &dstDot = R[r][c];
				dstDot = dot(basis, basisNext);
				basisNext -= basis * dstDot;
			}
			// Zero the bottom triangle of R
			for (i32 r = c+1; r < m; r++) {
				R[c][r] = T(0);
			}
		}
	#endif
		// m can be less than cols, so we have to fill in the rest of R in that case
		for (i32 c = m; c < Cols(); c++) {
			Vector<T> basis = Col(c);
			for (i32 r = 0; r < m; r++) {
				Vector<T> basisPrev = Q.Col(r);
				R[c][r] = dot(basis, basisPrev);
			}
		}
	}

	// This always works on symmetric matrices. Non-symmetric ones are a bit more tricky.
	void Eigen(Matrix &vectors, Vector<T> &values, i32 maxIterations = 1000, T epsilon = T(0.000001)) const {
		// Do a naive QR iteration for now
		AzAssert(Cols() == Rows(), Stringify(MATRIX_INFO_ARGS(*this), " error: Eigen-decomposition is only defined for square matrices."));
		i32 dims = Cols();
		vectors.Resize(dims, dims);
		vectors.ResetToIdentity();
		values.Resize(dims);
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, square(dims) * 4 + dims, T);
		T epsilonSqr = square(epsilon); // Since we compare to normSqr and not norm
		Matrix A_1 = workspace.GetMatrixCopy(*this);
		Matrix A_2 = workspace.GetMatrix(dims, dims);
		Matrix Q = workspace.GetMatrix(dims, dims);
		Matrix R = workspace.GetMatrix(dims, dims);
		Matrix *A_next = &A_2;
		Matrix *A_cur = &A_1;
		i32 i;
		for (i = 0; i < maxIterations; i++) {
			A_cur->QRDecomposition(Q, R);
			*A_next = R * Q;
			vectors = vectors * Q;
			T deltaSqr = T(0);
			T delta2Sqr = T(0);
			for (i32 c = 0; c < dims; c++) {
				for (i32 r = 0; r < dims; r++) {
					if (c != r) // Remove the diagonal
						deltaSqr += square(A_next->Val(c, r));
					delta2Sqr += square(abs(A_next->Val(c, r)) - abs(A_cur->Val(c, r)));
				}
			}
			Swap(A_cur, A_next);
			if (deltaSqr < epsilonSqr) break;
			// We're not really changing by iterating, so just bail out.
			if (delta2Sqr < epsilonSqr) break;
		}
		values = A_cur->Diag();
		Vector<T> swapStorage = workspace.GetVector(dims);
		Sort(values, 0, values.Count(), [](Vector<T> &array, i64 indexLHS, i64 indexRHS) -> bool {
			return array[indexLHS] > array[indexRHS];
		}, [&](Vector<T> &array, i64 indexLHS, i64 indexRHS, T &temp) {
			Swap(array[indexLHS], array[indexRHS], temp);
			swapStorage = vectors[indexLHS];
			vectors[indexLHS] = vectors[indexRHS];
			vectors[indexRHS] = swapStorage;
		});
	}

	// U is a min(cols, rows) x rows left singular matrix
	// S is a min(cols, rows) length vector that contains all the singular values (represents a diagonal matrix)
	// Vt is a cols x min(cols,rows) matrix, the transpose of the right singular matrix
	// U * Diagonal(S) * Vt is our original matrix
	void SingularValueDecomposition(Matrix &U, Vector<T> &S, Matrix &Vt, i32 maxIterations = 1000, T epsilon = T(0.000001)) {
		Matrix AT = transpose(this);
		if (Cols() <= Rows()) {
			Matrix AAT = *this * AT;
			AAT.Eigen(U, S, maxIterations, epsilon);
			S.count = Cols();
			for (i32 i = 0; i < S.Count(); i++) {
				if (S[i] > epsilon) {
					S[i] = sqrt(S[i]);
				} else {
					S[i] = T(0);
				}
			}
			U.cols = Cols();
			for (i32 c = 0; c < U.Cols(); c++) {
				if (c > 0) {
					U.Col(c).Orthogonalize(U.Col(c-1));
				}
				if (S[c] != T(0)) {
					U.Col(c).Normalize();
				} else {
					U.Col(c).ResetToValue(T(0));
				}
			}
			Vt.Resize(Cols(), Cols());
			for (i32 r = 0; r < Vt.Rows(); r++) {
				Vector<T> row = Vt.Row(r);
				row = AT * U.Col(r);
				if (S[r] != T(0)) {
					row /= S[r];
				}
			}
		} else {
			Matrix ATA = AT * *this;
			ATA.Eigen(Vt, S, maxIterations, epsilon);
			S.count = Rows();
			for (i32 i = 0; i < S.Count(); i++) {
				if (S[i] > epsilon) {
					S[i] = sqrt(S[i]);
				} else {
					S[i] = T(0);
				}
			}
			Vt.TransposeSoft();
			Vt.rows = Rows();
			for (i32 r = 0; r < Vt.Rows(); r++) {
				if (r > 0) {
					Vt.Row(r).Orthogonalize(Vt.Row(r-1));
				}
				if (S[r] != T(0)) {
					Vt.Row(r).Normalize();
				} else {
					Vt.Row(r).ResetToValue(T(0));
				}
			}
			U.Resize(Rows(), Rows());
			for (i32 r = 0; r < U.Rows(); r++) {
				Vector<T> row = U.Row(r);
				row = *this * Vt.Row(r);
				if (S[r] != T(0)) {
					row /= S[r];
				}
			}
		}
	}

	// Gives you the Moore-Penrose pseudoinverse (result will have inverted dimensions, like a transpose)
	Matrix PseudoInverse(i32 maxIterations = 1000, T epsilon = T(0.000001), T damping = T(0)) {
		i32 M = max(Cols(), Rows());
		i32 countNeeded = square(M) * 2 + M;
		AZ_DECLARE_MATRIX_WORKSPACE(workspace, countNeeded, T);
		Matrix U = workspace.GetMatrix(M, M);
		Vector<T> S = workspace.GetVector(M);
		Matrix Vt = workspace.GetMatrix(M, M);
		SingularValueDecomposition(U, S, Vt, maxIterations, epsilon);
		U.TransposeSoft();
		Vt.TransposeSoft();
		for (i32 i = 0; i < S.Count(); i++) {
			if (abs(S[i] + damping) > epsilon) {
				S[i] = T(1) / (S[i] + damping);
			} else {
				S[i] = T(0);
			}
			Vt.Col(i) *= S[i];
		}
		Matrix result = Vt * U;
		return result;
	}

};

} // namespace AzCore

template<typename T>
T dot(const az::Vector<T> &a, const az::Vector<T> &b) {
	a.AssertValid();
	b.AssertValid();
	AzAssert(a.Count() == b.Count(), az::Stringify("dot product of ", VECTOR_INFO_ARGS(a), " and ", VECTOR_INFO_ARGS(b), " error: Vectors must have the same number of components."));
	T result = 0;
	for (i32 i = 0; i < a.Count(); i++) {
		result += a[i] * b[i];
	}
	return result;
}

template<typename T>
T normSqr(const az::Vector<T> &a) {
	a.AssertValid();
	T result = 0;
	for (i32 i = 0; i < a.Count(); i++) {
		result += square(a[i]);
	}
	return result;
}

template<>
inline f32 dot(const az::Vector<f32> &a, const az::Vector<f32> &b) {
	a.AssertValid();
	b.AssertValid();
	using T = f32;
	AzAssert(a.Count() == b.Count(), az::Stringify("dot product of ", VECTOR_INFO_ARGS(a), " and ", VECTOR_INFO_ARGS(b), " error: Vectors must have the same number of components."));
	f32 result = 0;
	i32 i;
	for (i = 0; i <= a.Count()-4; i+=4) {
		f32x4 A = f32x4(a[i], a[i+1], a[i+2], a[i+3]);
		f32x4 B = f32x4(b[i], b[i+1], b[i+2], b[i+3]);
		A *= B;
		result += horizontalAdd(A);
	}
	for (; i < a.Count(); i++) {
		result += a[i] * b[i];
	}
	return result;
}

template<>
inline f32 normSqr(const az::Vector<f32> &a) {
	a.AssertValid();
	f32 result = 0;
	i32 i;
	for (i = 0; i <= a.Count()-4; i+=4) {
		f32x4 A = f32x4(a[i], a[i+1], a[i+2], a[i+3]);
		A *= A;
		result += horizontalAdd(A);
	}
	for (; i < a.Count(); i++) {
		result += square(a[i]);
	}
	return result;
}

namespace AzCore {

template<typename T>
az::Matrix<T> transpose(const az::Matrix<T> &a) {
	az::Matrix<T> result = a;
	result.Transpose();
	return result;
}

template<typename T>
az::Matrix<T> transpose(az::Matrix<T> *a) {
	az::Matrix<T> result = a;
	result.TransposeSoft();
	return result;
}

// Function-style Determinant
template<typename T>
inline T det(const az::Matrix<T> &a) {
	return a.Determinant();
}

} // namespace AzCore

template<typename T>
az::Vector<T>& operator+=(az::Vector<T> &lhs, const az::Vector<T> &rhs) {
	AzAssert(lhs.Count() == rhs.Count(), Stringify("Adding ", VECTOR_INFO_ARGS(lhs), " and ", VECTOR_INFO_ARGS(rhs), " error: Vector addition can only be done on same-size vectors."));
	for (i32 i = 0; i < lhs.Count(); i++) {
		lhs[i] += rhs[i];
	}
	return lhs;
}

template<typename T>
az::Vector<T>& operator-=(az::Vector<T> &lhs, const az::Vector<T> &rhs) {
	AzAssert(lhs.Count() == rhs.Count(), Stringify("Subtracting ", VECTOR_INFO_ARGS(lhs), " and ", VECTOR_INFO_ARGS(rhs), " error: Vector subtraction can only be done on same-size vectors."));
	for (i32 i = 0; i < lhs.Count(); i++) {
		lhs[i] -= rhs[i];
	}
	return lhs;
}

template<typename T>
az::Vector<T>& operator*=(az::Vector<T> &lhs, const T &rhs) {
	for (i32 i = 0; i < lhs.Count(); i++) {
		lhs[i] *= rhs;
	}
	return lhs;
}

template<typename T>
az::Vector<T>& operator/=(az::Vector<T> &lhs, const T &rhs) {
	for (i32 i = 0; i < lhs.Count(); i++) {
		lhs[i] /= rhs;
	}
	return lhs;
}

template<typename T>
az::Matrix<T>& operator*=(az::Matrix<T> &lhs, const T &rhs) {
	for (i32 c = 0; c < lhs.Cols(); c++) {
		for (i32 r = 0; r < lhs.Rows(); r++) {
			lhs.Val(c, r) *= rhs;
		}
	}
	return lhs;
}

template<typename T>
az::Matrix<T>& operator/=(az::Matrix<T> &lhs, const T &rhs) {
	for (i32 c = 0; c < lhs.Cols(); c++) {
		for (i32 r = 0; r < lhs.Rows(); r++) {
			lhs.Val(c, r) /= rhs;
		}
	}
	return lhs;
}

template<typename T>
az::Matrix<T>& operator+=(az::Matrix<T> &lhs, const az::Matrix<T> &rhs) {
	AzAssert(lhs.Cols() == rhs.Cols() && lhs.Rows() == rhs.Rows(), Stringify("Adding ", MATRIX_INFO_ARGS(lhs), " and ", MATRIX_INFO_ARGS(rhs), " error: Matrix addition can only be done on same-size matrices."));
	for (i32 r = 0; r < lhs.Rows(); r++) {
		for (i32 c = 0; c < lhs.Cols(); c++) {
			lhs.Val(c, r) += rhs.Val(c, r);
		}
	}
	return lhs;
}

template<typename T>
az::Matrix<T>& operator-=(az::Matrix<T> &lhs, const az::Matrix<T> &rhs) {
	AzAssert(lhs.Cols() == rhs.Cols() && lhs.Rows() == rhs.Rows(), Stringify("Subtracting ", MATRIX_INFO_ARGS(lhs), " and ", MATRIX_INFO_ARGS(rhs), " error: Matrix subtraction can only be done on same-size matrices."));
	for (i32 r = 0; r < lhs.Rows(); r++) {
		for (i32 c = 0; c < lhs.Cols(); c++) {
			lhs.Val(c, r) -= rhs.Val(c, r);
		}
	}
	return lhs;
}

template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<T>>
	>
>
inline az::Impl::VectorScalarOperation<T, az::Impl::OperationMul<T>, '*', false> operator*(T1 &&lhs, const T &rhs) {
	return az::Impl::VectorScalarOperation<T, az::Impl::OperationMul<T>, '*', false>(std::forward<T1>(lhs), rhs);
}
template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<T>>
	>
>
inline az::Impl::VectorScalarOperation<T, az::Impl::OperationMul<T>, '*', true> operator*(const T &lhs, T1 &&rhs) {
	return az::Impl::VectorScalarOperation<T, az::Impl::OperationMul<T>, '*', true>(std::forward<T1>(rhs), lhs);
}
template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<T>>
	>
>
inline az::Impl::VectorScalarOperation<T, az::Impl::OperationDiv<T>, '/', false> operator/(T1 &&lhs, const T &rhs) {
	return az::Impl::VectorScalarOperation<T, az::Impl::OperationDiv<T>, '/', false>(std::forward<T1>(lhs), rhs);
}
template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<T>>
	>
>
inline az::Impl::VectorScalarOperation<T, az::Impl::OperationDiv<T>, '/', true> operator/(const T &lhs, T1 &&rhs) {
	return az::Impl::VectorScalarOperation<T, az::Impl::OperationDiv<T>, '/', true>(std::forward<T1>(rhs), lhs);
}



template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationAdd<typename az::remove_cvref_t<T1>::Scalar_t>, '+'> operator+(T1 &&lhs, T2 &&rhs) {
	return az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationAdd<typename az::remove_cvref_t<T1>::Scalar_t>, '+'>(std::forward<T1>(lhs), std::forward<T2>(rhs));
}
template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationSub<typename az::remove_cvref_t<T1>::Scalar_t>, '-'> operator-(T1 &&lhs, T2 &&rhs) {
	return az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationSub<typename az::remove_cvref_t<T1>::Scalar_t>, '-'>(std::forward<T1>(lhs), std::forward<T2>(rhs));
}
template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationMul<typename az::remove_cvref_t<T1>::Scalar_t>, '*'> operator*(T1 &&lhs, T2 &&rhs) {
	return az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationMul<typename az::remove_cvref_t<T1>::Scalar_t>, '*'>(std::forward<T1>(lhs), std::forward<T2>(rhs));
}
template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationDiv<typename az::remove_cvref_t<T1>::Scalar_t>, '/'> operator/(T1 &&lhs, T2 &&rhs) {
	return az::Impl::VectorVectorOperation<typename az::remove_cvref_t<T1>::Scalar_t, az::Impl::OperationDiv<typename az::remove_cvref_t<T1>::Scalar_t>, '/'>(std::forward<T1>(lhs), std::forward<T2>(rhs));
}



template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<T>>
	>
>
inline az::Impl::MatrixScalarOperation<T, az::Impl::OperationMul<T>, '*', false> operator*(T1 &&lhs, const T &rhs) {
	return az::Impl::MatrixScalarOperation<T, az::Impl::OperationMul<T>, '*', false>(std::forward<T>(lhs), rhs);
}
template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<T>>
	>
>
inline az::Impl::MatrixScalarOperation<T, az::Impl::OperationMul<T>, '*', true> operator*(const T &lhs, T1 &&rhs) {
	return az::Impl::MatrixScalarOperation<T, az::Impl::OperationMul<T>, '*', true>(std::forward<T>(rhs), lhs);
}
template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<T>>
	>
>
inline az::Impl::MatrixScalarOperation<T, az::Impl::OperationDiv<T>, '/', false> operator/(T1 &&lhs, const T &rhs) {
	return az::Impl::MatrixScalarOperation<T, az::Impl::OperationDiv<T>, '/', false>(std::forward<T>(lhs), rhs);
}
template<
	class T1, class T,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<T>>
	>
>
inline az::Impl::MatrixScalarOperation<T, az::Impl::OperationDiv<T>, '/', true> operator/(const T &lhs, T1 &&rhs) {
	return az::Impl::MatrixScalarOperation<T, az::Impl::OperationDiv<T>, '/', true>(std::forward<T>(rhs), lhs);
}



template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::MatrixVectorMultiply<typename az::remove_cvref_t<T1>::Scalar_t, false> operator*(T1 &&lhs, T2 &&rhs) {
	return az::Impl::MatrixVectorMultiply<typename az::remove_cvref_t<T1>::Scalar_t, false>(std::forward<T1>(lhs), std::forward<T2>(rhs));
}
template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Vector<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Matrix<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::MatrixVectorMultiply<typename az::remove_cvref_t<T1>::Scalar_t, true> operator*(T1 &&lhs, T2 &&rhs) {
	return az::Impl::MatrixVectorMultiply<typename az::remove_cvref_t<T1>::Scalar_t, true>(std::forward<T2>(rhs), std::forward<T1>(lhs));
}



template<
	class T1, class T2,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<typename az::remove_cvref_t<T1>::Scalar_t>> &&
		std::is_same_v<az::remove_cvref_t<T2>, az::Matrix<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::MatrixMatrixMultiply<typename az::remove_cvref_t<T1>::Scalar_t> operator*(T1 &&lhs, T2 &&rhs) {
	return az::Impl::MatrixMatrixMultiply<typename az::remove_cvref_t<T1>::Scalar_t>(std::forward<T1>(lhs), std::forward<T2>(rhs));
}

template<
	class T1,
	typename = std::enable_if_t<
		std::is_same_v<az::remove_cvref_t<T1>, az::Matrix<typename az::remove_cvref_t<T1>::Scalar_t>>
	>
>
inline az::Impl::MatrixMatrixMultiply<typename az::remove_cvref_t<T1>::Scalar_t> operator*(az::Impl::MatrixMatrixMultiply<typename az::remove_cvref_t<T1>::Scalar_t> &&lhs, T1 &&rhs) {
	return az::Impl::MatrixMatrixMultiply<typename az::remove_cvref_t<T1>::Scalar_t>((az::remove_cvref_t<T1>)lhs, std::forward<T1>(rhs));
}

#undef MATRIX_INFO_ARGS
#undef VECTOR_INFO_ARGS

namespace AzCore {

template<typename T>
void AppendToString(String &string, const Vector<T> &vector) {
	string.Append('|');
	for (i32 i = 0; i < vector.Count(); i++) {
		string.Append(' ');
		AppendToString(string, vector[i]);
		string.Append(' ');
	}
	string.Append('|');
}

template<typename T>
void AppendToString(String &string, const Matrix<T> &matrix) {
	struct Info {
		i32 indexStart;
		i32 indexEnd;
		i32 size;
	};
	static thread_local Array<Info> infos;
	infos.ClearSoft();
	infos.Reserve(matrix.Count());
	i32 maxSize = 0;
	for (i32 r = 0; r < matrix.Rows(); r++) {
		string.Append('|');
		for (i32 c = 0; c < matrix.Cols(); c++) {
			Info info;
			string.Append(' ');
			info.indexStart = string.size;
			AppendToString(string, matrix.Val(c, r));
			info.indexEnd = string.size;
			info.size = info.indexEnd - info.indexStart;
			maxSize = max(maxSize, info.size);
			string.Append(' ');
			infos.Append(info);
		}
		string.Append("|\n");
	}
	Str spaces = "                ";
	maxSize = min(maxSize, (i32)spaces.size);
	for (i32 i = infos.size-1; i >= 0; i--) {
		auto &info = infos[i];
		if (info.size < maxSize) {
			string.Insert(info.indexEnd, spaces.SubRange(0, maxSize - info.size));
		}
	}
}

} // namespace AzCore

#endif // AZCORE_MATRIX_HPP