/*
	File: Result.hpp
	Author: Philip Haynes
	A discriminated union for a valid return type or an error type.
*/

#ifndef AZCORE_RESULT_HPP
#define AZCORE_RESULT_HPP

#include "../IO/Log.hpp"
#include "../Utility/RAIIHacks.hpp"
#include "../Memory/None.hpp"

namespace AzCore {

template <typename Success_t, typename Error_t>
struct Result {
	union {
		Success_t value;
		Error_t error;
	};
	bool isError;
	Result() = delete;
	Result(const Success_t &_value) : value(_value), isError(false) {}
	Result(Success_t &&_value) : value(std::move(_value)), isError(false) {}
	Result(const Error_t &_error) : error(_error), isError(true) {}
	Result(Error_t &&_error) : error(std::move(_error)), isError(true) {}
	Result(const Result &other) : isError(other.isError) {
		if (isError) {
			AzPlacementNew(error, other.error);
		} else {
			AzPlacementNew(value, other.value);
		}
	}
	Result(Result &&other) : isError(other.isError) {
		if (isError) {
			AzPlacementNew(error, std::move(other.error));
		} else {
			AzPlacementNew(value, std::move(other.value));
		}
	}
	Result& operator=(const Result &other) {
		if (isError != other.isError) {
			this->~Result();
			AzPlacementNew(*this, other);
		} else {
			if (isError) {
				error = other.error;
			} else {
				value = other.value;
			}
		}
		return *this;
	}
	Result& operator=(Result &&other) {
		if (isError != other.isError) {
			this->~Result();
			AzPlacementNew(*this, std::move(other));
		} else {
			if (isError) {
				error = std::move(other.error);
			} else {
				value = std::move(other.value);
			}
		}
		return *this;
	}
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
			PrintBacktrace(io::cerr);
			exit(1);
		}
		return std::move(value);
	}
	inline Success_t&& UnwrapOr(Success_t &&alternate) {
		return std::move(isError ? alternate : value);
	}
};

} // namespace AzCore

#endif // AZCORE_RESULT_HPP