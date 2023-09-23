/*
	File: Util.cpp
	Author: Philip Haynes
*/

#include "Util.hpp"

#include <stdio.h>

namespace AzCore {

size_t align(size_t size, size_t alignment) {
	AzAssert(IsPowerOfTwo(alignment), "alignment must be a power of 2. Maybe you want alignNonPowerOfTwo?");
	return (size + alignment-1) & ~(alignment-1);
}

size_t alignNonPowerOfTwo(size_t size, size_t alignment) {
	if (size % alignment == 0) {
		return size;
	} else {
		return (size/alignment+1)*alignment;
	}
}

} // namespace AzCore