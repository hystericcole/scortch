//
//  main.c
//  Scortch
//
//  Created by Eric Cole on 8/8/20.
//  Copyright © 2020 Eric Cole. All rights reserved.
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "sort.h"

#ifndef LONG_MAX
#define LONG_MAX ((long)(~0UL >> 1))
#endif

#ifndef countof
#define countof(_) (sizeof(_)/sizeof(_[0]))
#endif

void sortingTest(void);

int main(int argc, const char * argv[]) {
	sortingTest();
	return 0;
}

//	MARK: - Statistics

uint64_t microsecondsSince1970() {
	struct timeval t = {};
	
	gettimeofday(&t, NULL);
	
	return (uint64_t)t.tv_sec * 1000000 + (uint64_t)t.tv_usec;
}

void sortingStatisticsReset(struct SortingStatistics *statistics) {
	statistics->invocations = 0;
	statistics->accesses = 0;
	statistics->assignments = 0;
	statistics->comparisons = 0;
	statistics->timerBegan = microsecondsSince1970();
	statistics->timerEnded = 0;
}

void sortingStatisticsBegin(struct SortingStatistics *statistics) {
	statistics->timerBegan = microsecondsSince1970();
}

void sortingStatisticsEnded(struct SortingStatistics *statistics) {
	statistics->timerEnded = microsecondsSince1970();
}

//	MARK: - Testing

unsigned isAscending(void const *array, size_t count, size_t size, Compare compare, void *context) {
	size_t index;
	
	for ( index = 1 ; index < count ; ++index ) {
		if ( compare(array + index * size, array + (index - 1) * size, context) < 0 ) {
			return 0;
		}
	}
	
	return 1;
}

unsigned isDescending(void const *array, size_t count, size_t size, Compare compare, void *context) {
	size_t index;
	
	for ( index = 1 ; index < count ; ++index ) {
		if ( compare(array + (index - 1) * size, array + index * size, context) < 0 ) {
			return 0;
		}
	}
	
	return 1;
}

//unsigned isLessStabilityTestingUnsigned(unsigned * const a, unsigned * const b, void *context) {
//	return (*a >> 8) < (*b >> 8);
//}

signed compareStabilityTestingUnsigned(unsigned * const a, unsigned * const b, void *context) {
	return (signed)(*a >> 8) - (signed)(*b >> 8);
}

//unsigned isLessUnsigned(unsigned * const a, unsigned * const b, void *context) {
//	return *a < *b;
//}

signed compareUnsigned(unsigned * const a, unsigned * const b, void *context) {
	return *a < *b ? -1 : *a > *b ? 1 : 0;
}

//unsigned isLessString(char ** const a, char ** const b, void *context) {
//	return strcmp(*a, *b) < 0;
//}

signed compareString(char ** const a, char ** const b, void *context) {
	return strcmp(*a, *b);
}

void populateRandomIntegerArray(unsigned *array, unsigned count) {
	unsigned index, limit = 2;
	
	for ( index = 0 ; index < 8 ; ++index ) {
		limit = (limit + count / limit) >> 1;
	}
	
	limit = count - limit;
	
	for ( index = 0 ; index < count ; ++index ) {
		array[index] = arc4random_uniform(limit);
	}
}

void populateIntegerArray(unsigned *array, unsigned count, unsigned length, unsigned pattern) {
	unsigned range = 2, step = 8;
	unsigned index, patternIndex;
	unsigned value, order, limit, prior = 0;
	
	if ( !pattern ) { pattern = 5; }
	if ( !length) { length = count / 2; }
	
	for ( index = 0 ; index < 8 ; ++index ) {
		range = (range + count / range) >> 1;
	}
	
	range = count - range;
	value = 0;
	
	for ( index = 0, patternIndex = 0 ; index < count ; patternIndex += 1 ) {
		order = (pattern >> (patternIndex * 3)) & 7;
		limit = index + length < count ? index + length : count;
		
		if ( !order ) {
			patternIndex = 0;
			order = pattern & 7;
		}
		
		switch ( order ) {
		case 0:	//	zero
			bzero(array, count * sizeof(unsigned));
			break;
		
		case 1:	//	equal
			if ( prior != order ) { value = 1 + arc4random_uniform(count); }
			
			for ( ; index < limit ; ++index ) { array[index] = value; }
			break;
		
		case 2:	//	ascending
			if ( prior != order ) { value = 1 + arc4random_uniform(count); }
			
			for ( ; index < limit ; ++index ) { array[index] = value; value += arc4random_uniform(step); }
			break;
		
		case 3:	//	descending
			if ( prior != order ) { value = arc4random_uniform(count) + step * (limit - index); }
			
			for ( ; index < limit ; ++index ) { array[index] = value; value -= arc4random_uniform(step); }
			break;
		
		case 4:	//	random
			for ( ; index < limit ; ++index ) { array[index] = arc4random(); }
			break;
		
		case 5:	//	random limited to count - √count
			for ( ; index < limit ; ++index ) { array[index] = 1 + arc4random_uniform(range); }
			break;
		
		case 6:	//	runless random
			if ( prior != order ) { value = ~(unsigned)0 >> 1; }
			
			for ( ; index < limit ; ++index ) {
				if ( index & 1 ) {
					value = ~arc4random_uniform(~value);
				} else {
					value = 1 + arc4random_uniform(value - 1);
				}
				
				array[index] = value;
			}
			break;
		
		case 7:	//	runless random limited to count - √count
			if ( prior != order ) { value = range >> 1; }
			
			for ( ; index < limit ; ++index ) {
				if ( index & 1 ) {
					value = range - arc4random_uniform(range - value);
				} else {
					value = 1 + arc4random_uniform(value - 1);
				}
				
				array[index] = value;
			}
			break;
		}
		
		prior = order;
	}
}

unsigned *allocateRandomIntegerArray(unsigned count) {
	unsigned *array = malloc(count * sizeof(unsigned));
	
	populateRandomIntegerArray(array, count);
	
	return array;
}

unsigned *allocateIntegerArray(unsigned count, unsigned length, unsigned pattern) {
	unsigned *array = malloc(count * sizeof(unsigned));
	
	if ( pattern ) {
		populateIntegerArray(array, count, length, pattern);
	} else {
		populateRandomIntegerArray(array, count);
	}
	
	return array;
}

void populateStabilityTestingRandomIntegerArray(unsigned *array, unsigned count) {
	unsigned index, small = count / (count > 256 ? 256 : 16) + 1;
	
	for ( index = 0 ; index < small ; ++index ) {
		array[index] = arc4random();
	}
	
	for ( index = 0 ; index < count ; ++index ) {
		array[index] = (array[index % small] & ~0x00FF) | (index / small & 0x00FF);
	}
}

unsigned *allocateStabilityTestingRandomIntegerArray(unsigned count) {
	unsigned *array = malloc(count * sizeof(unsigned));
	
	populateStabilityTestingRandomIntegerArray(array, count);
	
	return array;
}

char **allocateRandomStringArray(unsigned count, unsigned length, unsigned samePrefix) {
	void *array = malloc(count * (sizeof(char *) + length + 1));
	char *stringData = array + count * sizeof(char *);
	char **strings = array;
	char *characters = " abcdefghijklmnopqrstuvwxyz";
	unsigned characterIndex, charactersLength = 27;
	size_t index, bytes = count * (length + 1);
	
	for ( index = 0 ; index < bytes ; ++index ) {
		characterIndex = index % (length + 1);
		
		if ( characterIndex < samePrefix && characterIndex + 1 < charactersLength ) {
			characterIndex += 1;
		} else {
			characterIndex = arc4random_uniform(charactersLength);
		}
		
		stringData[index] = characters[characterIndex];
	}
	
	for ( index = 0 ; index < count ; ++index ) {
		strings[index] = stringData + index * (length + 1);
		strings[index][length] = 0;
	}
	
	return array;
}

void sortingStatisticsDisplay(char const *name, struct SortingStatistics *statistics, size_t count) {
	if ( statistics ) {
		double seconds = (double)(statistics->timerEnded - statistics->timerBegan) / 1000000.0;
		int precision = count > 1000 ? 9 - (int)floor(log10((double)count)) : 6;
		printf("%25s %9lu < %9lu = %8.*f @ %9lu () \n", name, statistics->comparisons, statistics->assignments, precision, seconds, statistics->invocations);
	}
}

void sortingComparison(void const *original, size_t count, size_t size, Compare compare, void *context, Compare stableCompare, void *stableContext) {
	long timeSum, timeBest;
	long trial, repetitions = 3;
	struct SortingStatistics s = {};
	size_t bytes = (count > 4 ? count : 4) * size;
	bytes += -bytes & 0x00FF;
	void *buffer = malloc(bytes * 2);
	void *array = buffer + bytes;
	
	if ( !buffer ) {
		printf("•• buffer %lu not allocated\n", bytes * 2);
		return;
	}
	
	if ( count < 50000 ) {
		for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
			memcpy(array, original, count * size);
			sortingStatisticsReset(&s);
			binaryInsertionSort(array, count, size, 1, buffer, &s, compare, context);
			sortingStatisticsEnded(&s);
			timeSum += s.timerEnded - s.timerBegan;
			if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
		}
		s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
		sortingStatisticsDisplay("binaryInsertionSort", &s, count);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• binaryInsertionSort not ascending\n");
		} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
			printf("•• binaryInsertionSort not stable\n");
		}
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		quickSort(array, count, size, buffer, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("~=~ quickSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• quickSort not ascending\n");
	}
	
	if ( 0 ) {
		for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
			memcpy(array, original, count * size);
			sortingStatisticsReset(&s);
			balancingQuickSort(array, count, size, 0, buffer, &s, compare, context);
			sortingStatisticsEnded(&s);
			timeSum += s.timerEnded - s.timerBegan;
			if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
		}
		s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
		sortingStatisticsDisplay("~=~ balancingQuickSort", &s, count);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• balancingQuickSort not ascending\n");
		}
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		mergeSort(array, count, size, buffer, bytes, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("mergeSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• mergeSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• mergeSort not stable\n");
	}
	
	if ( 1 ) {
		for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
			memcpy(array, original, count * size);
			sortingStatisticsReset(&s);
			juggleMergeSort(array, buffer, count, size, 0, &s, compare, context);
			sortingStatisticsEnded(&s);
			timeSum += s.timerEnded - s.timerBegan;
			if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
		}
		s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
		sortingStatisticsDisplay("juggleMergeSort", &s, count);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• juggleMergeSort not ascending\n");
		} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
			printf("•• juggleMergeSort not stable\n");
		}
	}
	
	if ( count < 50000 ) {
		for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
			memcpy(array, original, count * size);
			sortingStatisticsReset(&s);
			mergeSort(array, count, size, buffer, size, &s, compare, context);
			sortingStatisticsEnded(&s);
			timeSum += s.timerEnded - s.timerBegan;
			if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
		}
		s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
		sortingStatisticsDisplay("inPlaceMergeSort", &s, count);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• inPlaceMergeSort not ascending\n");
		} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
			printf("•• inPlaceMergeSort not stable\n");
		}
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		coleSort(array, buffer, count, size, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("coleSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• coleSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• coleSort not stable\n");
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		seriesMergeSort(array, count, size, buffer, bytes, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("seriesMergeSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• seriesMergeSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• seriesMergeSort not stable\n");
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		tumbleMergeSort(array, buffer, count, size, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("tumbleMergeSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• tumbleMergeSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• tumbleMergeSort not stable\n");
	}
	
	if ( 1 ) {
		for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
			memcpy(array, original, count * size);
			sortingStatisticsReset(&s);
			polymergeSort(array, buffer, count, size, &s, compare, context);
			sortingStatisticsEnded(&s);
			timeSum += s.timerEnded - s.timerBegan;
			if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
		}
		s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
		sortingStatisticsDisplay("polymergeSort", &s, count);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• polymergeSort not ascending\n");
		} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
			printf("•• polymergeSort not stable\n");
		}
		
		for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
			memcpy(array, original, count * size);
			sortingStatisticsReset(&s);
			bottomUpPolymergeSort(array, buffer, count, size, &s, compare, context);
			sortingStatisticsEnded(&s);
			timeSum += s.timerEnded - s.timerBegan;
			if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
		}
		s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
		sortingStatisticsDisplay("bottomUpPolymergeSort", &s, count);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• bottomUpPolymergeSort not ascending\n");
		} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
			printf("•• bottomUpPolymergeSort not stable\n");
		}
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		mergeFourSort(array, buffer, count, size, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("mergeFourSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• mergeFourSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• mergeFourSort not stable\n");
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		bottomUpMergeFourSort(array, buffer, count, size, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("bottomUpMergeFourSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• bottomUpMergeFourSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• bottomUpMergeFourSort not stable\n");
	}
	
	for ( timeBest = LONG_MAX, timeSum = 0, trial = 0 ; trial < repetitions ; ++trial ) {
		memcpy(array, original, count * size);
		sortingStatisticsReset(&s);
		heapSort(array, count, size, buffer, &s, compare, context);
		sortingStatisticsEnded(&s);
		timeSum += s.timerEnded - s.timerBegan;
		if ( s.timerEnded - s.timerBegan < timeBest ) { timeBest = s.timerEnded - s.timerBegan; }
	}
	s.timerEnded = s.timerBegan + timeBest;//timeSum / repetitions;
	sortingStatisticsDisplay("~=~ heapSort", &s, count);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• heapSort not ascending\n");
//	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
//		printf("•• heapSort not stable\n");
	}
	
	free(buffer);
}

void sortingTest() {
	void *array;
	unsigned index, count, tooth, root;
	unsigned integerArray[] = {6, 3, 5, 99, 44, 37, 9, 66, 15, 69, 85, 1, 57, 19, 22, 98, 24, 73, 11, 13, 7, 42, 17, 23};
	char const *stringArray[] = {"dog", "cat", "elk", "bat", "fox", "ape", "red", "orange", "yellow", "green", "blue", "indigo", "violet", "azure", "viridian", "cerulean", "teal", "sepia", "umber", "cerise", "sienna", "crimson", "periwinkle"};
	unsigned integerArrayCounts[] = {101, 1009, 10007, 100003, 1000003, 4000037};
	unsigned integerArrayCount = countof(integerArrayCounts);
	unsigned stringArrayCounts[] = {101, 1009, 10007, 100003, 1000003};
	unsigned stringArrayCount = countof(stringArrayCounts);
	
	printf("-- sort small known integerArray %lu\n", countof(integerArray));
	sortingComparison(integerArray, countof(integerArray), sizeof(integerArray[0]), (Compare *)compareUnsigned, NULL, NULL, NULL);
	
	printf("-- sort small known stringArray %lu\n", countof(stringArray));
	sortingComparison(stringArray, countof(stringArray), sizeof(stringArray[0]), (Compare *)compareString, NULL, NULL, NULL);
	
	for ( index = 1 ; index < 4 ; ++index ) {
		count = integerArrayCounts[index];
		array = allocateRandomIntegerArray(count);
		
		for ( root = 2, tooth = 0 ; tooth < 12 ; ++tooth ) {
			root = (root + count / root) >> 1;
		}
		
		populateIntegerArray(array, count, count, 01);
		printf("-- sort equal unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, count, 02);
		printf("-- sort ascending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, count, 03);
		printf("-- sort descending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, count, 05);
		printf("-- sort random unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, count, 07);
		printf("-- sort runless unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 05222);
		printf("-- sort lead 3/4 ascending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 02225);
		printf("-- sort tail 3/4 ascending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 05225);
		printf("-- sort inner 1/2 ascending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 02552);
		printf("-- sort outer 1/2 ascending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 05333);
		printf("-- sort lead 3/4 descending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 03335);
		printf("-- sort tail 3/4 descending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 05335);
		printf("-- sort inner 1/2 descending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, (count + 3) / 4, 05335);
		printf("-- sort outer 1/2 descending unsigned array %u [%u]\n", count, (count + 3) / 4);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		tooth = count / (root > 8 ? root >> 2 : 2);
		populateIntegerArray(array, count, tooth, 015);
		printf("-- sort alternating random and equal unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, tooth, 025);
		printf("-- sort alternating random and ascending unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, tooth, 035);
		printf("-- sort alternating random and descending unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, tooth, 032);
		printf("-- sort alternating ascending and descending unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, tooth, 03425);
		printf("-- sort alternating random equal descending ascending unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, tooth, 05352);
		printf("-- sort alternating ascending random descending random unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateIntegerArray(array, count, tooth, 05253);
		printf("-- sort alternating descending or ascending with random unsigned array %u [%u]\n", count, tooth);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateStabilityTestingRandomIntegerArray(array, count);
		printf("-- sort stability testing unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareStabilityTestingUnsigned, NULL, (Compare *)compareUnsigned, NULL);
		
		free(array);
	}
	
	for ( index = 0 ; index < stringArrayCount ; ++index ) {
		count = stringArrayCounts[index];
		array = allocateRandomStringArray(count, 20, 10);
		printf("-- sort random string array %u (n log2 n = %.0f)\n", count, log2((double)count) * (double)count);
		sortingComparison(array, count, sizeof(char *), (Compare *)compareString, NULL, NULL, NULL);
		free(array);
	}
	
	for ( index = 0 ; index < integerArrayCount ; ++index ) {
		count = integerArrayCounts[index];
		array = allocateRandomIntegerArray(count);
		printf("-- sort random unsigned array %u (n log2 n = %.0f)\n", count, log2((double)count) * (double)count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		free(array);
	}
}
