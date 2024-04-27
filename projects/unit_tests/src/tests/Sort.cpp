/*
	File: Sort.cpp
	Author: Philip Haynes
	Making sure our sorting algorithms work as expected.
*/

#include "../UnitTests.hpp"
#include "AzCore/Sort.hpp"
#include "AzCore/Memory/Array.hpp"

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
		}\
	}

void SortTest() {
	Array<i32> listInitial = { 4, 6, 3, 9, 2, 1, 0, 5, 7, 8 };
	Array<i32> listExpect  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	Array<i32> listResult = listInitial;
	BubbleSort(listResult, 0, listResult.size);
	COMPARE_LIST(listResult, listExpect);
}

} // namespace SortTestNamespace