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
	Stable in-place sort with time complexity:
		Best:  O(n)
		Avg:   O(n^2)
		Worst: O(n^2)
	Quite cheap for small arrays. If you know your array will be less than a couple dozen items, this will probably be the fastest sorting algorithm anyway.
*/
template<typename Array_t, typename LessThanFunc_t=fp_lessThanGeneric_t<Array_t>, typename SwapFunc_t=fp_swapGeneric_t<Array_t>>
void BubbleSort(Array_t &array, i64 indexStart, i64 indexEnd, const LessThanFunc_t &lessThan=lessThanGeneric<Array_t>, const SwapFunc_t &swap=swapGeneric<Array_t, false>, array_subscript_t<Array_t, i64> *temp=nullptr) {
	MAYBE_CONSTRUCT_TEMP(temp);
	for (i64 n = indexEnd; n > indexStart+1;) {
		i64 nNew = 0;
		for (i64 i = indexStart+1; i < n; i++) {
			if (lessThan(array, i, i-1)) {
				swap(array, i, i-1, *temp);
				nNew = i;
			}
		}
		n = nNew;
	}
	MAYBE_DESTRUCT_TEMP(temp);
}

/*
	Unstable in-place sort with time complexity:
		Best:  O(n*log(n))
		Avg:   O(n*log(n))
		Worst: O(n^2)
	Will automatically choose a simpler sorting function at small sizes, so you can kinda just use this for everything unstable if you want.
*/
template<typename Array_t, typename LessThanFunc_t=fp_lessThanGeneric_t<Array_t>, typename SwapFunc_t=fp_swapGeneric_t<Array_t>>
void QuickSort(Array_t &array, i64 indexStart, i64 indexEnd, const LessThanFunc_t &lessThan=lessThanGeneric<Array_t>, const SwapFunc_t &swap=swapGeneric<Array_t, false>, az::remove_cvref_t<decltype(array[(i64)0])> *temp=nullptr) {
	MAYBE_CONSTRUCT_TEMP(temp);
	i64 count = indexEnd - indexStart;
	// Through some experimentation, 16 seems to be a good cutoff. Give or take 8ish, the timing results are roughly the same.
	if (count <= 16) {
		BubbleSort(array, indexStart, indexEnd, lessThan, swap, temp);
		goto done;
	}
	i64 indexPivot;
	{
		// Choose pivot
		i64 mid = indexStart + count / 2;
		i64 lo = indexStart;
		i64 hi = indexEnd-1;
		if (lessThan(array, mid, lo))
			Swap(lo, mid); // lo < mid
		if (lessThan(array, hi, lo))
			Swap(lo, hi);  // lo < hi & mid
		if (lessThan(array, hi, mid))
			Swap(mid, hi); // lo < mid < hi
		indexPivot = mid;
	}
	{
		i64 indexLeft = indexStart-1;
		i64 indexRight = indexEnd;
		while (true) {
			do {
				indexLeft++;
			} while (indexLeft < indexEnd && lessThan(array, indexLeft, indexPivot));
			do {
				indexRight--;
			} while (indexRight >= indexStart && lessThan(array, indexPivot, indexRight));
			if (indexLeft >= indexRight) break;
			if (indexLeft == indexPivot) {
				indexPivot = indexRight;
			} else if (indexRight == indexPivot) {
				indexPivot = indexLeft;
			}
			swap(array, indexLeft, indexRight, *temp);
		}
		if (indexRight == indexStart-1 || indexLeft == indexEnd) goto done;
		i64 indexSplit = indexRight+1;
		if (indexSplit == indexEnd) goto done;
		QuickSort(array, indexStart, indexSplit, lessThan, swap, temp);
		QuickSort(array, indexSplit, indexEnd, lessThan, swap, temp);
	}
done:
	MAYBE_DESTRUCT_TEMP(temp);
}

/*
	Unstable sort (just calls QuickSort, which should do the smart thing anyway).
*/
template<typename Array_t, typename LessThanFunc_t=fp_lessThanGeneric_t<Array_t>, typename SwapFunc_t=fp_swapGeneric_t<Array_t>>
inline void Sort(Array_t &array, i64 indexStart, i64 indexEnd, const LessThanFunc_t &lessThan=lessThanGeneric<Array_t>, const SwapFunc_t &swap=swapGeneric<Array_t, false>, az::remove_cvref_t<decltype(array[(i64)0])> *temp=nullptr) {
	QuickSort(array, indexStart, indexEnd, lessThan, swap, temp);
}

/*
	Stable sort (just calls BubbleSort, which is the only currently-implemented stable sorting algorithm even if its scaling isn't great for large arrays)
	TODO: Implement a better-scaling stable sort function such as merge sort
	You might also consider passing a lessThan function to Sort that handles all of the ordering requirements in one go.
*/
template<typename Array_t, typename LessThanFunc_t=fp_lessThanGeneric_t<Array_t>, typename SwapFunc_t=fp_swapGeneric_t<Array_t>>
inline void SortStable(Array_t &array, i64 indexStart, i64 indexEnd, const LessThanFunc_t &lessThan=lessThanGeneric<Array_t>, const SwapFunc_t &swap=swapGeneric<Array_t, false>, az::remove_cvref_t<decltype(array[(i64)0])> *temp=nullptr) {
	BubbleSort(array, indexStart, indexEnd, lessThan, swap, temp);
}


#undef MAYBE_CONSTRUCT_TEMP
#undef MAYBE_DESTRUCT_TEMP

} // namespace AzCore

#endif // AZCORE_SORT_HPP