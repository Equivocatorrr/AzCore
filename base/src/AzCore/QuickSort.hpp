/*
	File: QuickSort.hpp
	Author: Philip Haynes
	Does what it says on the tin.
*/

#ifndef AZCORE_QUICKSORT_HPP

#include "basictypes.hpp"
#include "memory.hpp"

namespace AzCore {

template<typename T, typename LessThanFunc_t>
void QuickSort(SimpleRange<T> toSort, const LessThanFunc_t &lessThan) {
	if (toSort.size < 2) return; // Nothing to be done
	if (toSort.size == 2) {
		if (lessThan(toSort[1], toSort[0])) {
			Swap(toSort[0], toSort[1]);
		}
		return;
	}
	T pivot;
	{
		// Choose pivot
		i64 mid = toSort.size/2;
		i64 lo = 0;
		i64 hi = toSort.size-1;
		if (lessThan(toSort[mid], toSort[lo]))
			Swap(lo, mid); // lo < mid
		if (lessThan(toSort[hi], toSort[lo]))
			Swap(lo, hi);  // lo < hi & mid
		if (lessThan(toSort[hi], toSort[mid]))
			Swap(mid, hi); // lo < mid < hi
		pivot = toSort[mid];
	}
	i64 left = -1;
	i64 right = toSort.size;
	while (true) {
		do {
			left++;
		} while (lessThan(toSort[left], pivot));
		do {
			right--;
		} while (lessThan(pivot, toSort[right]));
		if (left >= right) break;
		Swap(toSort[left], toSort[right]);
	}
	i64 split = right+1;
	QuickSort(toSort.SubRange(0, split), lessThan);
	QuickSort(toSort.SubRange(split, toSort.size-split), lessThan);
}

template <typename T, i32 allocTail, typename LessThanFunc_t>
inline void QuickSort(Array<T, allocTail> &toSort, const LessThanFunc_t &lessThan) {
	QuickSort<T>(SimpleRange<T>(toSort), lessThan);
}

template <typename T, i32 noAllocCount, i32 allocTail, typename LessThanFunc_t>
inline void QuickSort(ArrayWithBucket<T, noAllocCount, allocTail> &toSort,
                      const LessThanFunc_t &lessThan) {
	QuickSort<T>(SimpleRange<T>(toSort), lessThan);
}

template <typename T>
inline void QuickSort(SimpleRange<T> toSort) {
	QuickSort(toSort, [](T lhs, T rhs) { return lhs < rhs; });
}
template <typename T, i32 allocTail>
inline void QuickSort(Array<T, allocTail> &toSort) {
	QuickSort(toSort, [](T lhs, T rhs) { return lhs < rhs; });
}
template <typename T, i32 noAllocCount, i32 allocTail>
inline void QuickSort(ArrayWithBucket<T, noAllocCount, allocTail> &toSort) {
	QuickSort(toSort, [](T lhs, T rhs) { return lhs < rhs; });
}

} // namespace AzCore

#endif // AZCORE_QUICKSORT_HPP
