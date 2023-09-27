/*
	File: Result.hpp
	Author: Philip Haynes
	A discriminated union for a valid return type or an error type.
*/

#ifndef AZCORE_RESULT_HPP
#define AZCORE_RESULT_HPP

#include "../IO/Log.hpp"

namespace AzCore {

// Can be used for void success/error types
struct VoidResult_t {};

template <typename Success_t, typename Error_t>
struct Result {
	union {
		Success_t value;
		Error_t error;
	};
	bool isError;
	Result() = delete;
	Result(Success_t _value) : value(_value), isError(false) {}
	Result(Error_t _error) : error(_error), isError(true) {}
	~Result() {
		if (isError) {
			error.~Error_t();
		} else {
			value.~Success_t();
		}
	}
	#define AzUnwrap() Unwrap(__FILE__, __LINE__)
	constexpr Success_t&& Unwrap(const char *file, i32 line) {
		if (isError) {
			io::cerr.PrintLn(file, ":", line, " unwrap failure with error: ", error);
			PrintBacktrace();
			abort();
		}
		return std::move(value);
	}
	inline Success_t&& UnwrapOr(Success_t &&alternate) {
		return std::move(isError ? alternate : value);
	}
};

} // namespace AzCore

#endif // AZCORE_RESULT_HPP