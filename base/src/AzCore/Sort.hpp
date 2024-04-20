/*
	File: Sort.hpp
	Author: Philip Haynes
	Actually just making a generic sort function that does the smart thing.
*/

#ifndef AZCORE_SORT_HPP
#define AZCORE_SORT_HPP

#include "basictypes.hpp"
#include "Memory/Util.hpp"

namespace AzCore {

template<typename T>
force_inline(bool)
lessThanGeneric(T &array, i64 indexLHS, i64 indexRHS) {
	return array[indexLHS] < array[indexRHS];
}

template<typename T>
force_inline(void)
swapGeneric(T &array, i64 indexLHS, i64 indexRHS) {
	Swap(array[indexLHS], array[indexRHS]);
}

//
template<typename Array_t, typename LessThanFunc_t, typename SwapFunc_t>
void BubbleSort(Array_t &array, i64 indexStart, i64 indexEnd, const LessThanFunc_t &lessThan=lessThanGeneric<Array_t>, const SwapFunc_t &swap=swapGeneric<Array_t>) {
	for (i64 n = indexEnd; n > 1;) {
		i64 nNew = 0;
		for (i64 i = 1; i < n; i++) {
			if (lessThan(array, i, i-1)) {
				swap(array, i, i-1);
				nNew = i;
			}
		}
		n = nNew;
	}
}

} // namespace AzCore

#endif // AZCORE_SORT_HPP