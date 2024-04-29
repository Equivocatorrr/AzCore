/*
	File: Sort.cpp
	Author: Philip Haynes
	Making sure our sorting algorithms work as expected.
*/

#include "../UnitTests.hpp"
#include "AzCore/Sort.hpp"
#include "AzCore/Memory/Array.hpp"
#include "AzCore/Math/RandomNumberGenerator.hpp"
#include "AzCore/Time.hpp"

namespace SortTestNamespace {

using namespace AzCore;

void SortTest();
UT::Register binarySet("Sort", SortTest);

#define COMPARE_LIST(list1, list2)\
	{\
		UTAssert((list1).size == (list2).size);\
		bool correct = true;\
		for (i32 _i = 0; _i < (list1).size; _i++) {\
			if ((list1)[_i] != (list2)[_i]) {\
				correct = false;\
				break;\
			}\
		}\
		if (!correct) {\
			UT::ReportProblem(__LINE__, true, "The lists...\n", (list1), "\n", (list2), "\n... Are not equal!");\
			UT::currentTestInfo->result = UT::Result::FAILURE;\
		}\
	}

void CheckWithListSize(i32 listSize, i32 iteration, Nanoseconds &dstBubbleTime, Nanoseconds &dstQuickTime) {
	ClockTime start, end;
	RandomNumberGenerator rng(69420 + iteration);
	i32 shuffleId = genShuffleId();
	Array<i32> listExpect(listSize);
	Array<i32> listInitial(listSize);
	start = Clock::now();
	for (i32 i = 0; i < listExpect.size; i++) {
		listExpect[i] = i;
		listInitial[i] = shuffle(shuffleId, listSize, &rng);
	}
	end = Clock::now();
	// UT::ReportInfo(__LINE__, "Took ", Nanoseconds(end - start), " to generate a shuffled list of ", listSize, " values.");
	// UT::ReportInfo(__LINE__, "Initial list: ", listInitial, "\nSorted list:  ", listExpect);
	Array<i32> listResult = listInitial;
	start = Clock::now();
	BubbleSort(listResult, 0, listResult.size);
	end = Clock::now();
	dstBubbleTime += Nanoseconds(end - start);
	// UT::ReportInfo(__LINE__, "Took ", dstBubbleTime, " to bubble sort a list of ", listSize, " values.");
	COMPARE_LIST(listResult, listExpect);
	listResult = listInitial;
	start = Clock::now();
	QuickSort(listResult, 0, listResult.size);
	end = Clock::now();
	dstQuickTime += Nanoseconds(end - start);
	// UT::ReportInfo(__LINE__, "Took ", dstQuickTime, " to quick sort a list of ", listSize, " values.");
	COMPARE_LIST(listResult, listExpect);
}

void SortTest() {
	constexpr i32 MAX_LIST_SIZE = 32;
	constexpr i32 NUM_SAMPLES_PER_LIST_SIZE = 500;
	for (i32 i = 8; i <= MAX_LIST_SIZE; i++) {
		Nanoseconds bubbleTime = Nanoseconds(0), quickTime = Nanoseconds(0);
		for (i32 j = 0; j < NUM_SAMPLES_PER_LIST_SIZE; j++) {
			CheckWithListSize(i, j, bubbleTime, quickTime);
		}
		bubbleTime /= NUM_SAMPLES_PER_LIST_SIZE;
		quickTime /= NUM_SAMPLES_PER_LIST_SIZE;
		UT::ReportInfo(__LINE__, "Took on average ", quickTime, " to quick sort and ", bubbleTime, " to bubble sort a list of ", i, " values.");
	}
	// UTAssert(quickIsFaster);
	// TODO: Maybe refine this with multiple samples for more reliable output.
	// UT::ReportInfo(__LINE__, "QuickSort is always faster than bubble sort with a list size of ", crossoverPoint);
}

} // namespace SortTestNamespace