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

#include <initializer_list>

namespace AzCore {

#define MATRIX_INFO_ARGS(obj) "Matrix<", az::TypeName<T>(), ">(", (obj).cols, ", ", (obj).rows, ")"
#define VECTOR_INFO_ARGS(obj) "Vector<", az::TypeName<T>(), ">(", (obj).count, ")"

template<typename T>
struct Vector {
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
	constexpr Vector(Vector *other) : data(other->data), count(other.count), stride(other.stride), capacity(0) {}

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

	Vector& operator=(const Vector &other) {
		if (capacity) {
			if (capacity < other.Count()) {
				delete[] data;
				data = ArrayNewCopy(other.Count(), other.data);
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

	Vector& operator=(Vector &&other) {
		if (capacity) {
			delete[] data;
		}
		data = other.data;
		count = other.count;
		stride = other.stride;
		capacity = other.capacity;
		other.capacity = 0;
		return *this;
	}

	i32 Count() const {
		return count;
	}

	T& operator[](i32 index) {
		AssertValid();
		AzAssert(index >= 0 && index < (i32)count, Stringify("Index ", index, " is out of bounds for ", VECTOR_INFO_ARGS(*this)));
		return data[index * stride];
	}
	const T& operator[](i32 index) const {
		return const_cast<Vector*>(this)->operator[](index);
	}
};

template<typename T>
struct Matrix {
	T *data;
	u16 cols, rows;
	u8 colStride, rowStride;
	u16 capacity;

	inline void AssertValid() const {
		AzAssert(data != nullptr, Stringify("Matrix<", TypeName<T>(), "> is null."));
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

	static Matrix Identity(i32 size) {
		Matrix result(size, size);
		for (i32 c = 0; c < size; c++) {
			for (i32 r = 0; r < size; r++) {
				result[c][r] = c == r ? T(1) : T(0);
			}
		}
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
	static Matrix Filled(i32 _cols, i32 _rows, std::initializer_list<T> init) {
		AzAssert(_cols * _rows == init.size(), Stringify("Expected _cols * _rows to equal the initializer_list size (_cols = ", _cols, ", _rows = ", _rows, ", size = ", init.size(), ")"));
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

	Matrix& operator=(const Matrix &other) {
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
	Matrix& operator=(Matrix &&other) {
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

	Vector<T> Col(i32 index) const {
		AssertValid();
		AzAssert(index >= 0 && index < (i32)cols, Stringify("Column ", index, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		Vector<T> result(data + index * colStride, rows, rowStride);
		return result;
	}

	Vector<T> Row(i32 index) const {
		AssertValid();
		AzAssert(index >= 0 && index < (i32)rows, Stringify("Row ", index, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		Vector<T> result(data + index * rowStride, cols, colStride);
		return result;
	}

	inline Vector<T> operator[](i32 index) const {
		return Col(index);
	}

	inline T& Val(i32 col, i32 row) const {
		AssertValid();
		AzAssert(col >= 0 && col < (i32)cols, Stringify("Column ", col, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
		AzAssert(row >= 0 && row < (i32)rows, Stringify("Row ", row, " is out of bounds for ", MATRIX_INFO_ARGS(*this)));
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
};

} // namespace AzCore

template<typename T>
T dot(az::Vector<T> a, az::Vector<T> b) {
	a.AssertValid();
	b.AssertValid();
	AzAssert(a.Count() == b.Count(), az::Stringify("dot product of ", VECTOR_INFO_ARGS(a), " and ", VECTOR_INFO_ARGS(b), " error: Vectors must have the same number of components."));
	T result = 0;
	for (i32 i = 0; i < a.Count(); i++) {
		result += a[i] * b[i];
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
Matrix<T>& operator+=(Matrix<T> &lhs, const Matrix<T> &rhs) {
	AzAssert(lhs.Cols() == rhs.Cols() && lhs.Rows() == rhs.Rows(), Stringify("Adding ", MATRIX_INFO_ARGS(lhs), " and ", MATRIX_INFO_ARGS(rhs), " error: Matrix addition can only be done on same-size matrices."));
	for (i32 i = 0; i < Count(); i++) {
		lhs.data[i] += rhs.data[i];
	}
	return lhs;
}

template<typename T>
Matrix<T>& operator-=(Matrix<T> &lhs, const Matrix<T> &rhs) {
	AzAssert(lhs.Cols() == rhs.Cols() && lhs.Rows() == rhs.Rows(), Stringify("Subtracting ", MATRIX_INFO_ARGS(lhs), " and ", MATRIX_INFO_ARGS(rhs), " error: Matrix subtraction can only be done on same-size matrices."));
	for (i32 i = 0; i < Count(); i++) {
		lhs.data[i] -= rhs.data[i];
	}
	return lhs;
}

template<typename T>
Matrix<T> operator*(const Matrix<T> &lhs, const Matrix<T> &rhs) {
	AzAssert(lhs.Cols() == rhs.Rows(), Stringify("Multiplying ", MATRIX_INFO_ARGS(lhs), " and ", MATRIX_INFO_ARGS(rhs), " error: lhs.Cols() must equal rhs.Rows()."));
	Matrix<T> result(rhs.Cols(), lhs.Rows());
	for (i32 c = 0; c < result.Cols(); c++) {
		for (i32 r = 0; r < result.Rows(); r++) {
			result[c][r] = dot(rhs.Col(c), lhs.Row(r));
		}
	}
	return result;
}

template<typename T>
Vector<T> operator*(const Matrix<T> &lhs, const Vector<T> &rhs) {
	AzAssert(lhs.Cols() == rhs.Count(), Stringify("Multiplying ", MATRIX_INFO_ARGS(lhs), " and ", VECTOR_INFO_ARGS(rhs), " error: lhs.Cols() must equal rhs.Count()."));
	Vector<T> result(lhs.Rows());
	for (i32 i = 0; i < result.Count(); i++) {
		result[i] = dot(rhs, lhs.Row(i));
	}
	return result;
}

template<typename T>
Vector<T> operator*(const Vector<T> &lhs, const Matrix<T> &rhs) {
	AzAssert(lhs.Count() == rhs.Rows(), Stringify("Multiplying ", VECTOR_INFO_ARGS(lhs), " and ", MATRIX_INFO_ARGS(rhs), " error: lhs.Count() must equal rhs.Rows()."));
	Vector<T> result(rhs.Cols());
	for (i32 i = 0; i < result.Count(); i++) {
		result[i] = dot(rhs.Col(i), lhs);
	}
	return result;
}

#undef MATRIX_INFO_ARGS
#undef VECTOR_INFO_ARGS

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