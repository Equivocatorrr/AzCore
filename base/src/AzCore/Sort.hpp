/*
	File: Sort.hpp
	Author: Philip Haynes
	Actually just making a generic sort function that does the smart thing.
*/

#ifndef AZCORE_SORT_HPP
#define AZCORE_SORT_HPP

#include "basictypes.hpp"
#include "Memory/Util.hpp"
#include "Memory/RAIIHacks.hpp"
#include "TemplateUtil.hpp"

namespace AzCore {

template<typename T>
using fp_lessThanGeneric_t = bool (*)(T &, i64, i64);

template<typename Array_t, typename T=array_subscript_t<Array_t, i64>>
using fp_swapGeneric_t = void (*)(Array_t &, i64, i64, T &);

template<typename T>
bool lessThanGeneric(T &array, i64 indexLHS, i64 indexRHS) {
	return array[indexLHS] < array[indexRHS];
}

template<typename T, bool swapByValue=false>
void swapGeneric(T &array, i64 indexLHS, i64 indexRHS, array_subscript_t<T, i64> &temp) {
	if constexpr (swapByValue) {
		SwapByValue(array[indexLHS], array[indexRHS], temp);
	} else {
		Swap(array[indexLHS], array[indexRHS], temp);
	}
}

#define MAYBE_CONSTRUCT_TEMP(temp) \
	using _Temp_t = az::remove_cvref_t<decltype(*temp)>;\
	struct alignas(alignof(_Temp_t)) FakeTemp_t {};\
	FakeTemp_t myTempStorage;\
	bool weOwnTemp;\
	if (temp == nullptr) {\
		temp = (_Temp_t*)&myTempStorage;\
		AzPlacementNew(*temp);\
		weOwnTemp = true;\
	} else {\
		weOwnTemp = false;\
	}
#define MAYBE_DESTRUCT_TEMP(temp) \
	if (weOwnTemp) {\
		temp->~_Temp_t();\
	}

/*
	Stable sort with time complexity:
		Best:  O(n)
		Avg:   O(n^2)
		Worst: O(n^2)
*/
template<typename Array_t, typename LessThanFunc_t=fp_lessThanGeneric_t<Array_t>, typename SwapFunc_t=fp_swapGeneric_t<Array_t>>
void BubbleSort(Array_t &array, i64 indexStart, i64 indexEnd, const LessThanFunc_t &lessThan=lessThanGeneric<Array_t>, const SwapFunc_t &swap=swapGeneric<Array_t, false>, array_subscript_t<Array_t, i64> *temp=nullptr) {
	MAYBE_CONSTRUCT_TEMP(temp);
	for (i64 n = indexEnd; n > 1;) {
		i64 nNew = 0;
		for (i64 i = 1; i < n; i++) {
			if (lessThan(array, i, i-1)) {
				swap(array, i, i-1, *temp);
				nNew = i;
			}
		}
		n = nNew;
	}
	MAYBE_DESTRUCT_TEMP(temp);
}

#undef MAYBE_CONSTRUCT_TEMP
#undef MAYBE_DESTRUCT_TEMP

} // namespace AzCore

#endif // AZCORE_SORT_HPP