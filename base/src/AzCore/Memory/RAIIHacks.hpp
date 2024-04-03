/*
	File: RAIIHacks.hpp
	Author: Philip Haynes
	Helper functions for hacking RAII into doing what you actually want.
*/

#ifndef AZCORE_RAII_HACKS_HPP
#define AZCORE_RAII_HACKS_HPP

#include "../basictypes.hpp"
#include <type_traits>

#define AzPlacementNew(value, ...) new(&(value)) std::remove_reference_t<decltype(value)>(__VA_ARGS__)

namespace AzCore {

// effectively like `new T[count](other)` where other was alloc'd as `new T[count * srcStride]` as well.
// Performs copy-initialization (as opposed to default initialization followed by copy-assignment).
template<typename T>
constexpr T* ArrayNewCopy(size_t count, const T* other, size_t srcStride=1) {
	// C++ has the ability to specify alignment in new calls, but MSVC throws an error if you actually do that, so we have this fuckery instead. Thanks again, Microshit.
	struct alignas(alignof(T)) FakeT {};
	T *result = (T*)(new FakeT[count * sizeof(T) / sizeof(FakeT)]);
	for (i32 i = 0; i < count; i++) {
		AzPlacementNew(result[i], other[i * srcStride]);
	}
	return result;
}

} // namespace AzCore

#endif // AZCORE_RAII_HACKS_HPP