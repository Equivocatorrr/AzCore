#include "AzCore/IO/Log.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/Memory/BigInt.hpp"
#include "AzCore/Math/RandomNumberGenerator.hpp"

using namespace AzCore;

io::Log cout("checks.log");

u64 StringToU64(String str) {
	u64 number = 0;
	u64 exponent = 1;
	for (i32 i = str.size-1; i >= 0; i--) {
		number += u64(str[i]-'0') * exponent;
		exponent *= 10;
	}
	return number;
}

u64 totalPersistenceChecks = 0;
u64 remainingPersistenceChecks = 0;
u32 biggestSecondIterationNumberDigits = 0;

u32 bestPersistence = 0;
String bestPersistenceNum;

u32 checksCount = 0;

Mutex persistenceMutex;
Array<String> persistenceNumbers[5]{};

struct DigitCounts {
	u32 counts[10];
};
u32 rearrangementChecks = 0;
u64 totalRearrangementChecks = 0;
u64 remainingRearrangementChecks = 0;
ClockTime startTime;

Milliseconds totalTimeTaken(0);

Array<String> successfulFactorizations{};

const i32 minimumDigits = 4;
// const i32 minimumDigits = 237;
const i32 minimumPermutationDigits = 17;
const i32 maximumDigits = 32;
// const i32 maximumDigits = 237;
const i32 numThreads = 8;

Mutex threadControlMutex;
u32 activeThreads = 0;
u32 completedThreads = 0;
u32 remainingThreads = 0;

String DurationString(u64 ms) {
	String out;
	const u32 weeks = ms / 604800000;
	const u32 days = (ms / 86400000) % 7;
	const u32 hours = (ms / 3600000) % 24;
	const u32 minutes = (ms / 60000) % 60;
	const u32 seconds = (ms / 1000) % 60;
	const u32 milliseconds = ms % 1000;
	if (weeks > 0) {
		out += ToString(weeks) + " weeks ";
	}
	if (days > 0) {
		out += ToString(days) + " days ";
	}
	if (hours > 0) {
		out += ToString(hours) + "h ";
	}
	if (minutes > 0) {
		out += ToString(minutes) + "m ";
	}
	if (seconds > 0) {
		out += ToString(seconds) + "s ";
	}
	if (milliseconds > 0) {
		out += ToString(milliseconds) + "ms ";
	}
	return out;
}

u32 persistence(BigInt number, u32 iteration=0) {
	if (number < 10) {
		return iteration;
	}
	String numString = number.Digits();
	if (iteration == 1 && (u32)numString.size > biggestSecondIterationNumberDigits) {
		biggestSecondIterationNumberDigits = numString.size;
	}
	BigInt newNumber(1);
	u32 cache = 1;
	for (char& c : numString) {
		if (c == '0') {
			return iteration+1;
		}
		cache *= u32(c-'0');
		if (cache > 477218588) {
			newNumber *= cache;
			cache = 1;
		}
	}
	// if (iteration == 3) {
	//	 cout.Lock();
	//	 cout.PrintLn("Fourth iteration number: ", number.ToString(), " with numString ", numString, " multiplied = ", newNumber.ToString());
	//	 cout.Unlock();
	// }
	if (cache != 1) {
		newNumber *= cache;
	}
	return persistence(newNumber, iteration+1);
}

u32 persistence(String number, u32 iteration=0) {
	if (number.size <= 1) {
		return iteration;
	}
	BigInt newNumber(1);
	u32 cache = 1;
	for (char& c : number) {
		if (c == '0') {
			return iteration+1;
		}
		cache *= u32(c-'0');
		if (cache > 477218588) {
			newNumber *= cache;
			cache = 1;
		}
	}
	if (cache != 1) {
		newNumber *= cache;
	}
	return persistence(newNumber, iteration+1);
}

bool BetterPersistence(u32 per, String num) {
	if (per > bestPersistence) {
		return true;
	}
	if (per == bestPersistence) {
		if (num.size < bestPersistenceNum.size) {
			return true;
		} else if (num.size == bestPersistenceNum.size) {
			for (i32 i = 0; i < num.size; i++) {
				if (num[i] < bestPersistenceNum[i]) {
					return true;
				} else if (num[i] > bestPersistenceNum[i]) {
					return false;
				}
			}
		}
	}
	return false;
}

void CheckPersistence(u32 minDigits, u32 maxDigits, u32 currentDigit=0, u32 totalDigits=0, String numStr=String()) {
	const char digits[] = {'2', '3', '4', '5', '6', '7', '8', '9'};
	if (totalDigits > maxDigits) {
		return;
	}
	if (totalDigits >= minDigits) {
		u32 per = persistence(numStr);
		persistenceMutex.Lock();
		totalPersistenceChecks++;
		remainingPersistenceChecks--;
		if (checksCount++ >= 10000000 / (minimumDigits+maximumDigits) || per > 10) {
			const Milliseconds delta = std::chrono::duration_cast<Milliseconds>(Clock::now() - startTime);
			cout.Lock();
			cout.PrintLn("Per: ", per, " for num: ", numStr, "\nTotal Persistence Checks So Far: ", totalPersistenceChecks, "\n", DurationString(delta.count()), "elapsed. Estimated ", DurationString(delta.count() * remainingPersistenceChecks / totalPersistenceChecks), "remaining.\n");
			cout.Unlock();
			checksCount = 1;
		}
		persistenceMutex.Unlock();
		if (per >= 8) {
			persistenceMutex.Lock();
			persistenceNumbers[per-8].Append(numStr);
			persistenceMutex.Unlock();
		}
		if (BetterPersistence(per, numStr)) {
			bestPersistence = per;
			bestPersistenceNum = numStr;
			if (per > 2) {
				cout.Lock();
				cout.PrintLn("New Best Persistence (of ", per, ") number found: ", numStr, "\nTotal Persistence Checks So Far: ", totalPersistenceChecks, "\n");
				cout.Unlock();
			}
		}
	}
	for (u32 i = currentDigit; i < 8; i++) {
		if (totalDigits >= 2 && (i == 4 || i < 3))
			continue;
		CheckPersistence(minDigits, maxDigits, i, totalDigits+1, numStr+digits[i]);
	}
}

void GetRequiredPersistenceChecks(u32 minDigits, u32 maxDigits, u32 currentDigit = 0, u32 totalDigits = 0) {
	if (totalDigits > maxDigits) {
		return;
	}
	if (totalDigits >= minDigits) {
		persistenceMutex.Lock();
		remainingPersistenceChecks++;
		persistenceMutex.Unlock();
	}
	for (u32 i = currentDigit; i < 8; i++) {
		if (totalDigits >= 2 && (i == 4 || i < 3))
			continue;
		GetRequiredPersistenceChecks(minDigits, maxDigits, i, totalDigits+1);
	}
}

Array<u128> GetPrimeFactors(BigInt number) {
	#include "tenThousandPrimes.cpp"
	Array<u128> factors{};
	bool fullStop = false;
	for (i32 i = 0; i < primes.size && !fullStop; i++) {
		bool keepGoing = true;
		while (keepGoing) {
			BigInt quotient, remainder;
			BigInt::QuotientAndRemainder(number, BigInt(primes[i]), &quotient, &remainder);
			if (quotient < primes[i]) {
				fullStop = true;
				keepGoing = false;
			}
			if (remainder == 0) {
				number = quotient;
				factors.Append(primes[i]);
			} else {
				keepGoing = false;
			}
		}
	}
	if (number > 1) {
		u64 leftOver = number.words[0];
		if (number.words.size > 1) {
			leftOver += (u64)number.words[1] << 32;
		}
		factors.Append(leftOver);
	}
	return factors;
}

bool GetSingleDigitFactors(BigInt number, String *dstFactors) {
	for (u32 i = 9; i >= 2; i--) {
		bool keepGoing = true;
		while (keepGoing) {
			BigInt quotient;
			u64 remainder;
			BigInt::QuotientAndRemainder(number, i, &quotient, &remainder);
			if (quotient < i) {
				keepGoing = false;
			}
			if (remainder == 0u) {
				number = quotient;
				*dstFactors += char(i+'0');
			} else {
				keepGoing = false;
			}
		}
	}
	return (number == 1);
}

bool CheckAllRearrangements(const DigitCounts& digits, String number=String()) {
	bool success = false;
	bool constructed = true;
	for (u32 i = 0; i < 10; i++) {
		if (digits.counts[i] > 0) {
			constructed = false;
			DigitCounts temp = digits;
			temp.counts[i]--;
			if (CheckAllRearrangements(temp, number+char(i+'0'))) {
				success = true;
			}
		}
	}
	if (constructed) {
		String factors;
		if (GetSingleDigitFactors(BigInt(number), &factors)) {
			cout.Lock();
			successfulFactorizations.Append(factors);
			cout.Print("\n\n\n\nWe found one!!! It's ", number, " and it has the factors: ");
			for (i32 i = 0; i < factors.size; i++) {
				cout.Print(factors[i], " ");
			}
			cout.PrintLn("\n\n\n");
			cout.Unlock();
			success = true;
		}
		persistenceMutex.Lock();
		totalRearrangementChecks++;
		remainingRearrangementChecks--;
		if (rearrangementChecks++ >= 1000000 || factors.size > 10) {
			cout.Lock();
			cout.Print("\nChecked number: ", number, " which has the factors: ");
			for (i32 i = 0; i < factors.size; i++) {
				cout.Print(factors[i], " ");
			}
			totalTimeTaken = std::chrono::duration_cast<Milliseconds>(Clock::now() - startTime);
			cout.PrintLn("\n", totalRearrangementChecks, " total checks, ", remainingRearrangementChecks, " remaining.\n", DurationString(totalTimeTaken.count()), " passed so far, estimated ", DurationString(totalTimeTaken.count() * remainingRearrangementChecks / totalRearrangementChecks), "remaining.\n");
			cout.Unlock();
			rearrangementChecks = 1;
		}
		persistenceMutex.Unlock();
	}
	return success;
}

bool CheckAllRearrangements(const String& number) {
	DigitCounts digits; // 0 through 9
	for (u32 i = 0; i < 10; i++) {
		digits.counts[i] = 0;
	}
	for (const char& c : number) {
		digits.counts[c-'0']++;
	}
	return CheckAllRearrangements(digits);
}

void ThreadProc(u32 i) {
	CheckPersistence(i+minimumDigits, i+minimumDigits);
	threadControlMutex.Lock();
	activeThreads--;
	completedThreads++;
	remainingThreads--;
	cout.Lock();
	cout.PrintLn("\nThread ", i, " completed.\n", remainingThreads, " remaining, ", completedThreads, " completed.\n\n");
	cout.Unlock();
	threadControlMutex.Unlock();
}

void ThreadProc2(const String& number) {
	CheckAllRearrangements(number);
	threadControlMutex.Lock();
	activeThreads--;
	completedThreads++;
	remainingThreads--;
	cout.Lock();
	cout.PrintLn("\n\nThread for number ", number, " has completed.\n", remainingThreads, " remaining, ", completedThreads, " completed.\n\n");
	cout.Unlock();
	threadControlMutex.Unlock();
}

BigInt factorial(u32 a) {
	BigInt answer(a);
	while (a-- > 2) {
		answer *= a;
	}
	return answer;
}

u64 pow(const u64& base, const u64& exponent) {
	u64 answer = 1;
	for (u32 i = 0; i < exponent; i++) {
		answer *= base;
	}
	return answer;
}

void BigIntTest() {
	BigInt test(BucketArray<u64, BIGINT_BUCKET_SIZE>({0, 1}));
	BigInt test2(2);
	cout.PrintLn("test = ", test.HexString(), " and test2 = ", test2.HexString());
	cout.PrintLn("test * test2 = ", (test * test2).HexString());
	cout.PrintLn("test / test2 = ", (test / test2).HexString());
	cout.PrintLn("test % test2 = ", (test % test2).HexString());
	cout.PrintLn("test + test2 = ", (test + test2).HexString());
	cout.PrintLn("test - test2 = ", (test - test2).HexString());
	cout.PrintLn("test2 << 32 = ", (test2 << 32).HexString());
	cout.PrintLn("test >> 32 = ", (test >> 32).HexString());
	cout.PrintLn("test2 << 64 = ", (test2 << 64).HexString());
	cout.PrintLn("test >> 64 = ", (test >> 64).HexString());
}

void CheckNumbersForHighPersistence() {
	remainingPersistenceChecks = 0;
	RandomNumberGenerator rng;
	Array<i32> randomizedDigits;
	randomizedDigits.Reserve(maximumDigits-minimumDigits+1);
	for (i32 i = minimumDigits; i <= maximumDigits; i++) {
		if (randomizedDigits.size != 0) {
			i32 spot = rng.Generate() % randomizedDigits.size;
			spot %= (rng.Generate() % randomizedDigits.size) + 1;
			randomizedDigits.Insert(spot, i-minimumDigits);
		} else {
			randomizedDigits.Append(i-minimumDigits);
		}
		GetRequiredPersistenceChecks(i, i);
	}
	cout.PrintLn("A total of ", remainingPersistenceChecks, " persistence checks will be made.\n");
	completedThreads = 0;
	remainingThreads = randomizedDigits.size;
	startTime = Clock::now();
	for (i32 i = 0; i <= maximumDigits-minimumDigits; i++) {
		while (true) {
			if (activeThreads < numThreads) {
				cout.PrintLn("\nStarting thread ", randomizedDigits[i], "\n");
				Thread(ThreadProc, randomizedDigits[i]).Detach();
				threadControlMutex.Lock();
				activeThreads++;
				threadControlMutex.Unlock();
				break;
			} else {
				Thread::Sleep(Milliseconds(100));
			}
		}
	}
	while (true) {
		if (activeThreads > 0) {
			Thread::Sleep(Milliseconds(100));
		} else {
			break;
		}
	}

	cout.PrintLn("\n\nBest Number: ", bestPersistenceNum, "\nTotal steps: ", bestPersistence, "\nTotal numbers checked: ", totalPersistenceChecks, "\nBiggest 2nd Iteration Number has ", biggestSecondIterationNumberDigits, " digits.\n");
	for (i32 x = 8; x <= 12; x++) {
		cout.PrintLn("\n\nList of numbers found with persistence of ", x, ":");
		for (i32 i = 0; i < persistenceNumbers[x-8].size; i++) {
			cout.PrintLn(persistenceNumbers[x-8][i]);
			// cout << "Factors: ";
			// Array<u64> factors = GetPrimeFactors(persistenceNumbers[x-8][i]);
			// for (const u64& factor : factors) {
			//	 cout << factor << " ";
			// }
			// cout << "\n\n";
		}
		if (persistenceNumbers[x-8].size == 0) {
			cout.PrintLn("None.");
		}
		cout.Print("\n");
	}
}

void CheckHighPersistenceNumbersForSingleDigitFactorability() {
	CheckNumbersForHighPersistence();

	cout.PrintLn("Adding variations of all persistence 11 numbers, now including leading ones.");
	Array<String> allPersistenceElevenNumbers(persistenceNumbers[3]);
	for (i32 i = 0; i < persistenceNumbers[3].size; i++) {
		String lead = "1";
		for (i32 x = (i32)persistenceNumbers[3][i].size; x < maximumDigits; x++) {
			allPersistenceElevenNumbers.Append(lead + persistenceNumbers[3][i]);
			lead += "1";
		}
	}

	cout.PrintLn("\nTrimming persistence 11 numbers with fewer than ", minimumPermutationDigits, " digits.\n");

	for (i32 i = 0; i < allPersistenceElevenNumbers.size; i++) {
		if (allPersistenceElevenNumbers[i].size < minimumPermutationDigits) {
			allPersistenceElevenNumbers.Erase(i--);
		}
	}

	cout.PrintLn("Calculating how many permutations we will have to run...");
	remainingRearrangementChecks = 0;
	for (i32 i = 0; i < allPersistenceElevenNumbers.size; i++) {
		DigitCounts digits;
		for (u32 j = 0; j < 10; j++) {
			digits.counts[j] = 0;
		}
		for (const char& c : allPersistenceElevenNumbers[i]) {
			digits.counts[c-'0']++;
		}
		BigInt permutationCount = factorial(allPersistenceElevenNumbers[i].size);
		for (u32 j = 0; j < 10; j++) {
			if (digits.counts[j] > 1) {
				permutationCount /= factorial(digits.counts[j]);
			}
		}
		if (permutationCount.words.size > 2) {
			cout.PrintLn("Number ", allPersistenceElevenNumbers[i], " has too many permutations: ", ToString(permutationCount));
			return;
		}
		cout.PrintLn("Number ", allPersistenceElevenNumbers[i], " has ", ToString(permutationCount), " permutations.");
		remainingRearrangementChecks += permutationCount.words[0];
		if (permutationCount.words.size == 2) {
			remainingRearrangementChecks += (u64)permutationCount.words[1] << 32;
		}
	}

	remainingThreads = allPersistenceElevenNumbers.size;
	completedThreads = 0;

	startTime = Clock::now();

	for (i32 i = 0; i < allPersistenceElevenNumbers.size; i++) {
		while (true) {
			if (activeThreads < numThreads) {
				cout.PrintLn("Launching thread for number ", allPersistenceElevenNumbers[i], "...");
				Thread(ThreadProc2, allPersistenceElevenNumbers[i]).Detach();
				threadControlMutex.Lock();
				activeThreads++;
				threadControlMutex.Unlock();
				break;
			} else {
				Thread::Sleep(Milliseconds(100));
			}
		}
	}
	cout.PrintLn("\n");
	while (true) {
		if (activeThreads > 0) {
			Thread::Sleep(Milliseconds(100));
		} else {
			break;
		}
	}

	totalTimeTaken = std::chrono::duration_cast<Milliseconds>(Clock::now() - startTime);

	cout.PrintLn("\n\n\nAll permutations checked.\nTotal time taken: ", DurationString(totalTimeTaken.count()));

	cout.PrintLn("\nResults show ", successfulFactorizations.size, " factorizations successfully found to have a greater persistence:");
	for (i32 i = 0; i < successfulFactorizations.size; i++) {
		cout.PrintLn(successfulFactorizations[i]);
	}

	cout.Print("\n");
}

i32 main(i32 argumentCount, char** argumentValues) {
	BigIntTest();
	CheckNumbersForHighPersistence();
}
