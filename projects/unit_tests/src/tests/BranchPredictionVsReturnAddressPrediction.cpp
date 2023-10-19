/*
	File: BranchPredictionVsReturnAddressPrediction.cpp
	Author: Philip Haynes
	Measuring the relative cost of a branch vs a call/ret pair for conditional NOPs.
*/

#include "../UnitTests.hpp"
#include "AzCore/Time.hpp"
#include "AzCore/Thread.hpp"

// What a mouthful!
namespace BranchPredictionVsReturnAddressPredictionTestNamespace {

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

using namespace AzCore;

void BranchPredictionVsReturnAddressPredictionTest();
UT::Register branchPredictionVsReturnAddressPredictionTest("BranchPredictionVsReturnAddressPrediction", BranchPredictionVsReturnAddressPredictionTest);

typedef void (*fp_Event)(void *data);

constexpr i32 NUM_ITERATIONS = 1000000;
constexpr i32 NUM_ITERATIONS_INTERNAL = 8*2;

void EventNop(void *data) {}

void BranchPredictionVsReturnAddressPredictionTest() {
	volatile fp_Event DoNothing;
	void *data = nullptr;
	
	{ // Because __rdtsc can be inaccurate if we change cores midway through
		u16 cpu = 0;
		Thread::SetProcessorAffinity(SimpleRange<u16>(&cpu, 1));
	}
	
	DoNothing = EventNop;
	u64 start = __rdtsc();
	for (i32 i = 0; i < NUM_ITERATIONS; i++) {
		// This has to be done backwards because MSVC is broken and doesn't emit the jmp or call instructions when it's the right way around :(
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
		if (nullptr == DoNothing) { DoNothing(data); } if (nullptr == DoNothing) { DoNothing(data); }
	}
	u64 cyclesBranch = __rdtsc() - start;
	UT::ReportInfo(__LINE__, "Took ", cyclesBranch, " cycles to branch ", NUM_ITERATIONS*NUM_ITERATIONS_INTERNAL, " times (", FormatFloat((f64)cyclesBranch / (f64)(NUM_ITERATIONS*NUM_ITERATIONS_INTERNAL), 10, 3), " cycles per)");
	DoNothing = EventNop;
	start = __rdtsc();
	for (i32 i = 0; i < NUM_ITERATIONS; i++) {
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
		DoNothing(data); DoNothing(data);
	}
	u64 cyclesCallRet = __rdtsc() - start;
	UT::ReportInfo(__LINE__, "Took ", cyclesCallRet, " cycles to call an empty function ", NUM_ITERATIONS*NUM_ITERATIONS_INTERNAL, " times (", FormatFloat((f64)cyclesCallRet / (f64)(NUM_ITERATIONS*NUM_ITERATIONS_INTERNAL), 10, 3), " cycles per)");
	
	Thread::ResetProcessorAffinity();
}

} // namespace BranchPredictionVsReturnAddressPredictionTestNamespace