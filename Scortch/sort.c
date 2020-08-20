//
//  sort.c
//  Scortch
//
//  Created by Eric Cole on 8/8/20.
//  Copyright © 2020 Eric Cole. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "sort.h"

#ifndef LONG_MAX
#define LONG_MAX ((long)(~0UL >> 1))
#endif

#ifndef countof
#define countof(_) (sizeof(_)/sizeof(_[0]))
#endif

typedef unsigned IsLess(void const *, void const *, void *);
typedef signed Compare(void const *, void const *, void *);

struct PointerCount {
	void const *p;
	size_t n;
};

//	MARK: - Statistics

struct SortingStatistics {
	long invocations;
	long accesses;
	long assignments;
	long comparisons;
	
	long timerBegan;
	long timerEnded;
};

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

//	MARK: - Utility

unsigned invokeIsLess(void const * a, void const * b, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->accesses += 2;
		statistics->comparisons += 1;
	}
	
	return compare(a, b, context) < 0;
}

signed invokeCompare(void const * a, void const * b, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->accesses += 2;
		statistics->comparisons += 1;
	}
	
	return compare(a, b, context);
}

unsigned invokeStableIsLess(void const * a, void const * b, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->accesses += 2;
		statistics->comparisons += 1;
	}
	
	signed c = compare(a, b, context);
	
	return c ? c < 0 : a < b;
}

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

//	MARK: - Data Motion

void accessAt(void *array, size_t from, size_t size, void * to, struct SortingStatistics *statistics) {
	if ( statistics ) {
		statistics->accesses += 1;
	}
	
	memmove(to, array + from * size, size);
}

void assignAt(void *array, size_t to, size_t size, void const * from, struct SortingStatistics *statistics) {
	if ( statistics ) {
		statistics->accesses += 1;
		statistics->assignments += 1;
	}
	
	memmove(array + to * size, from, size);
}

void assignManyAt(void *array, size_t to, size_t count, size_t size, void const * from, struct SortingStatistics *statistics) {
	if ( statistics ) {
		statistics->accesses += count;
		statistics->assignments += count;
	}
	
	memmove(array + to * size, from, count * size);
}

void swapAt(void *array, size_t from, size_t to, size_t size, void *temporary, struct SortingStatistics *statistics) {
	if ( statistics ) {
		statistics->accesses += 2;
		statistics->assignments += 2;
	}
	
	memcpy(temporary, array + from * size, size);
	memmove(array + from * size, array + to * size, size);
	memcpy(array + to * size, temporary, size);
}

void swapManyAt(void *array, size_t from, size_t to, size_t count, size_t size, void *buffer, struct SortingStatistics *statistics) {
	if ( statistics ) {
		statistics->accesses += 2 * count;
		statistics->assignments += 2 * count;
	}
	
	memcpy(buffer, array + from * size, count * size);
	memmove(array + from * size, array + to * size, count * size);
	memcpy(array + to * size, buffer, count * size);
}

void reverse(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics) {
	long i, end = count - 1, half = count / 2;
	
	for ( i = 0 ; i < half ; ++i ) {
		swapAt(array, i, end - i, size, temporary, statistics);
	}
}

void slideDown(void *array, size_t from, size_t to, size_t size, void *temporary, struct SortingStatistics *statistics) {
	if ( from <= to ) {
		return;
	}
	
	if ( statistics ) {
		statistics->accesses += from - to + 1;
		statistics->assignments += from - to + 1;
	}
	
	memcpy(temporary, array + from * size, size);
	memmove(array + (to + 1) * size, array + to * size, (from - to) * size);
	memcpy(array + to * size, temporary, size);
}

void slideUp(void *array, size_t from, size_t to, size_t size, void *temporary, struct SortingStatistics *statistics) {
	if ( to <= from ) {
		return;
	}
	
	if ( statistics ) {
		statistics->accesses += to - from + 1;
		statistics->assignments += to - from + 1;
	}
	
	memcpy(temporary, array + from * size, size);
	memmove(array + from * size, array + (from + 1) * size, (to - from) * size);
	memcpy(array + to * size, temporary, size);
}

//	MARK: - Binary Insertion Sort

///	Insertion sort that uses a binary search to place each element
void binaryInsertionSort(void *array, size_t count, size_t size, size_t sorted, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t i, m, n, o;
	
	for ( i = sorted ; i < count ; ++i ) {
		m = 1;
		n = i;
		
		while ( m <= n ) {
			o = (m + n) / 2 - 1;
			
			if ( invokeIsLess(array + i * size, array + o * size, statistics, compare, context) ) {
				n = o;
			} else {
				m = o + 2;
			}
		}
		
		if ( m <= i ) {
			slideDown(array, i, m - 1, size, temporary, statistics);
		}
	}
}

void binaryMoveSort(void *unsorted, void *sorted, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t i, m, n, o;
	
	for ( i = 0 ; i < count ; ++i ) {
		m = 1;
		n = i;
		
		while ( m <= n ) {
			o = (m + n) / 2 - 1;
			
			if ( invokeIsLess(unsorted + i * size, sorted + o * size, statistics, compare, context) ) {
				n = o;
			} else {
				m = o + 2;
			}
		}
		
		if ( m <= i ) {
			assignManyAt(sorted, m, i - m + 1, size, sorted + (m - 1) * size, statistics);
		}
		
		assignAt(sorted, m - 1, size, unsorted + i * size, statistics);
	}
}

//	MARK: - Merge Sort

void inPlaceMergeSorted(void *array, size_t count, size_t split, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	size_t merged = 0, swapped = 0;
	
	/*
		a : 0 ... merged		elements that have been merged
		b : merged ... split	unmerged elements from before split
		c : split ... swapped	swapped elements from before split (a < c < b)
		d : swapped ... count	unmerged elements from after split
	*/
	
	while ( 1 ) {
		while ( merged < split && swapped < 1 ) {
			if ( invokeIsLess(array + split * size, array + merged * size, statistics, compare, context) ) {
				swapAt(array, merged, split, size, temporary, statistics);
				swapped += 1;
			}
			
			merged += 1;
		}
		
		while ( merged < split && split + swapped < count ) {
			if ( invokeIsLess(array + (split + swapped) * size, array + split * size, statistics, compare, context) ) {
				swapAt(array, merged, split + swapped, size, temporary, statistics);
				swapped += 1;
			} else {
				swapAt(array, merged, split, size, temporary, statistics);
				slideUp(array, split, split + swapped - 1, size, temporary, statistics);
			}
			
			merged += 1;
		}
		
		if ( merged < split ) {
			//	elements above split are swapped and less than unmerged elements before split
			
			while ( 1 ) {
				if ( split - merged < count - split ) {
					if ( (split - merged) * 2 < count - split ) {
						break;
					}
					
					swapManyAt(array, merged, count - split + merged, split - merged, size, temporary, statistics);
					
					count -= split - merged;
				} else {
					if ( (count - split) * 2 < split - merged ) {
						break;
					}
					
					swapManyAt(array, merged, split, count - split, size, temporary, statistics);
					
					merged += count - split;
				}
			}
			
			if ( merged < split && split < count ) {
				reverse(array + merged * size, split - merged, size, temporary, statistics);
				reverse(array + split * size, count - split, size, temporary, statistics);
				reverse(array + merged * size, count - merged, size, temporary, statistics);
			}
			
			break;
		}
		
		if ( swapped > 0 && split + swapped < count ) {
			//	elements below split are merged and elements above split are ready to merge
			array += split * size;
			count -= split;
			split = swapped;
			swapped = 0;
			merged = 0;
		} else {
			break;
		}
	}
}

void mergeSorted(void *array, size_t count, size_t split, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	if ( space < split * size && space < (count - split) * size ) {
		inPlaceMergeSorted(array, count, split, size, buffer, statistics, compare, context);
		return;
	}
	
	size_t i, j, n;
	
	if ( split * size > space ) {
		memmove(buffer, array + split * size, (count - split) * size);
		
		for ( i = 0, j = split, n = 0 ; n < count ; ++n ) {
			if ( i < split && invokeIsLess(buffer + (count - j - 1) * size, array + (split - i - 1) * size, statistics, compare, context) ) {
				assignAt(array, count - n - 1, size, array + (split - i - 1) * size, statistics);
				i += 1;
			} else {
				assignAt(array, count - n - 1, size, buffer + (count - j - 1) * size, statistics);
				j += 1;
			}
		}
	} else {
		memmove(buffer, array, split * size);
		
		for ( i = 0, j = split, n = 0 ; n < count && i < split ; ++n ) {
			if ( j < count && invokeIsLess(array + j * size, buffer + i * size, statistics, compare, context) ) {
				assignAt(array, n, size, array + j * size, statistics);
				j += 1;
			} else {
				assignAt(array, n, size, buffer + i * size, statistics);
				i += 1;
			}
		}
	}
}

void mergeSort(void *array, size_t count, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	}
	
	size_t half = count / 2;
	mergeSort(array, half, size, buffer, space, statistics, compare, context);
	mergeSort(array + half * size, count - half, size, buffer, space, statistics, compare, context);
	mergeSorted(array, count, half, size, buffer, space, statistics, compare, context);
}

/// Merge sort that uses insertion sort when the pieces are small instead of descending to single element arrays
void insertionMergeSort(void *array, size_t count, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	} else if ( count < 32 ) {
		binaryInsertionSort(array, count, size, 1, buffer, statistics, compare, context);
	} else {
		size_t half = count / 2;
		insertionMergeSort(array, half, size, buffer, space, statistics, compare, context);
		insertionMergeSort(array + half * size, count - half, size, buffer, space, statistics, compare, context);
		mergeSorted(array, count, half, size, buffer, space, statistics, compare, context);
	}
}

/// Merge sort that doubles the width at each iteration instead of halving at each descent
void bottomUpMergeSort(void *array, size_t count, size_t size, size_t width, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context) {
	size_t index;
	
	if ( width > 1 ) {
		index = 0;
		
		while ( index + width <= count ) {
			binaryInsertionSort(array + index * size, width, size, 1, buffer, statistics, compare, context);
			
			index += width;
		}
		
		if ( index + 1 < count ) {
			binaryInsertionSort(array + index * size, count - index, size, 1, buffer, statistics, compare, context);
		}
	} else {
		width = 1;
	}
	
	while ( width < count ) {
		index = 0;
		
		while ( index + width * 2 <= count ) {
			mergeSorted(array + index * size, width * 2, width, size, buffer, space, statistics, compare, context);
			
			index += width * 2;
		}
		
		if ( index + width < count ) {
			mergeSorted(array + index * size, count - index, width, size, buffer, space, statistics, compare, context);
		}
		
		width *= 2;
	}
}

//	MARK: - Series Merge Sort

size_t seriesMergeSeek(void *array, size_t count, size_t size, size_t minimum, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	if ( minimum < 2 ) {
		return minimum;
	}
	
	unsigned isReversed = invokeIsLess(array + 1 * size, array + 0 * size, statistics, compare, context);
	size_t length, run = 2;
	
	while ( run < count && isReversed == invokeIsLess(array + run * size, array + (run - 1) * size, statistics, compare, context) ) {
		run += 1;
	}
	
	if ( isReversed ) {
		reverse(array, run, size, buffer, statistics);
	}
	
	if ( run < minimum && minimum < 16 ) {
		binaryInsertionSort(array, minimum, size, run, buffer, statistics, compare, context);
		
		run = minimum;
	}
	
	while ( run + 1 < minimum ) {
		length = seriesMergeSeek(array + run * size, count - run, size, minimum - run < run ? minimum - run : run, buffer, space, statistics, compare, context);
		
		mergeSorted(array, run + length, run, size, buffer, space, statistics, compare, context);
		
		run += length;
	}
	
	return run;
}

///	Insertion merge sort that honors natural ascending and descending series in the array
void seriesMergeSort(void *array, size_t count, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	}
	
	size_t length, sorted = 0;
	size_t minimum = count < 64 ? count / 2 : 32;
	
	while ( sorted < count ) {
		length = seriesMergeSeek(array + sorted * size, count - sorted, size, minimum, buffer, space, statistics, compare, context);
		
		if ( sorted > 0 ) {
			mergeSorted(array, sorted + length, sorted, size, buffer, space, statistics, compare, context);
		}
		
		sorted += length;
		minimum = count - sorted < sorted ? count - sorted : sorted;
	}
}

//	MARK: - Heap Sort

void heapSift(void *array, size_t start, size_t end, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t i = start, j = start;
	size_t a, b;
	unsigned d = 0;
	
	while ( j * 2 + 2 <= end ) {
		if ( invokeIsLess(array + (j * 2 + 1) * size, array + (j * 2 + 2) * size, statistics, compare, context) ) {
			j = j * 2 + 2;
		} else {
			j = j * 2 + 1;
		}
		d += 1;
	}
	
	while ( j * 2 + 1 <= end ) {
		j = j * 2 + 1;
		d += 1;
	}
	
	while ( invokeIsLess(array + j * size, array + i * size, statistics, compare, context) ) {
		j = (j - 1) / 2;
		d -= 1;
	}
	
	if ( d > 0 ) {
		accessAt(array, i, size, temporary, statistics);
		
		while ( d --> 0 ) {
			a = (j + 1 - (1 << d)) >> d;
			b = (a - 1) >> 1;
			assignAt(array, b, size, array + a * size, statistics);
		}
		
		assignAt(array, j, size, temporary, statistics);
	}
}

///	Heap Sort organizes elements into a heap then extracts elements from the heap in sorted order
void heapSort(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	}
	
	size_t start = count / 2;
	
	while ( start > 0 ) {
		start -= 1;
		
		heapSift(array, start, count - 1, size, temporary, statistics, compare, context);
	}
	
	size_t end = count - 1;
	
	while ( end > 0 ) {
		swapAt(array, 0, end, size, temporary, statistics);
		
		end -= 1;
		
		heapSift(array, 0, end, size, temporary, statistics, compare, context);
	}
}

//	MARK: - Quick Sort

size_t quickPartition(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t pivot = count / 2;
	size_t lower = 0, upper = count - 1;
	
	if ( invokeIsLess(array + pivot * size, array + lower * size, statistics, compare, context) ) {
		swapAt(array, pivot, lower, size, temporary, statistics);
	}
	
	if ( invokeIsLess(array + upper * size, array + pivot * size, statistics, compare, context) ) {
		swapAt(array, pivot, upper, size, temporary, statistics);
		
		if ( invokeIsLess(array + pivot * size, array + lower * size, statistics, compare, context) ) {
			swapAt(array, pivot, lower, size, temporary, statistics);
		}
	}
	
	for ( ;; ) {
		do {
			lower += 1;
		} while ( invokeIsLess(array + lower * size, array + pivot * size, statistics, compare, context) );
		
		do {
			upper -= 1;
		} while ( invokeIsLess(array + pivot * size, array + upper * size, statistics, compare, context) );
		
		if ( lower < upper ) {
			swapAt(array, lower, upper, size, temporary, statistics);
			
			if ( pivot == lower ) {
				pivot = upper;
			} else if ( pivot == upper ) {
				pivot = lower;
			}
		} else {
			break;
		}
	}
	
	return 1 + upper;
}

///	Quick sort identifies a pivot element then swaps elements on the wrong side of the pivot
void quickSort(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	} else if ( count < 16 ) {
		binaryInsertionSort(array, count, size, 1, temporary, statistics, compare, context);
	} else {
		size_t pivot = quickPartition(array, count, size, temporary, statistics, compare, context);
		
		quickSort(array, pivot, size, temporary, statistics, compare, context);
		quickSort(array + pivot * size, count - pivot, size, temporary, statistics, compare, context);
	}
}

/// Self balancing quick sort that use an alternate sorting algorithm on affected regions after several pivots in a row are imbalanced
void balancingQuickSort(void *array, size_t count, size_t size, unsigned imbalances, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	} else if ( count < 16 ) {	//	higher threshold will decrease comparisons and increase assignments
		binaryInsertionSort(array, count, size, 1, temporary, statistics, compare, context);
	} else if ( imbalances > 4 ) {
		heapSort(array, count, size, temporary, statistics, compare, context);
	} else {
		size_t pivot = quickPartition(array, count, size, temporary, statistics, compare, context);
		size_t ratio = 12;
		
		imbalances = count > ratio * (count - pivot - 1 < pivot ? count - pivot - 1 : pivot) ? imbalances + 1 : 0;
		
		balancingQuickSort(array, pivot, size, imbalances, temporary, statistics, compare, context);
		balancingQuickSort(array + pivot * size, count - pivot, size, imbalances, temporary, statistics, compare, context);
	}
}

//	MARK: - Testing

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
	unsigned index;
	
	for ( index = 0 ; index < count ; ++index ) {
		array[index] = arc4random();
	}
}

unsigned *allocateRandomIntegerArray(unsigned count) {
	unsigned *array = malloc(count * sizeof(unsigned));
	
	populateRandomIntegerArray(array, count);
	
	return array;
}

void populateStabilityTestingRandomIntegerArray(unsigned *array, unsigned count) {
	unsigned index, small = count / (count > 256 ? 256 : 16) + 1;
	
	populateRandomIntegerArray(array, small);
	
	for ( index = 0 ; index < count ; ++index ) {
		array[index] = (array[index % small] & ~0x00FF) | (index / small & 0x00FF);
	}
}

unsigned *allocateStabilityTestingRandomIntegerArray(unsigned count) {
	unsigned *array = malloc(count * sizeof(unsigned));
	
	populateStabilityTestingRandomIntegerArray(array, count);
	
	return array;
}

char **allocateRandomStringArray(unsigned count, unsigned length) {
	void *array = malloc(count * (sizeof(char *) + length + 1));
	char *stringData = array + count * sizeof(char *);
	char **strings = array;
	char *characters = " abcdefghijklmnopqrstuvwxyz";
	unsigned charactersLength = 27;
	size_t index, bytes = count * (length + 1);
	
	for ( index = 0 ; index < bytes ; ++index ) {
		stringData[index] = characters[arc4random_uniform(charactersLength)];
	}
	
	for ( index = 0 ; index < count ; ++index ) {
		strings[index] = stringData + index * (length + 1);
		strings[index][length] = 0;
	}
	
	return array;
}

void sortingStatisticsDisplay(char const *name, struct SortingStatistics *statistics) {
	if ( statistics ) {
		double seconds = (double)(statistics->timerEnded - statistics->timerBegan) / 1000000.0;
		printf("%25s %9lu < %9lu = %8.4f @ %9lu () \n", name, statistics->comparisons, statistics->assignments, seconds, statistics->invocations);
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
		sortingStatisticsDisplay("binaryInsertionSort", &s);
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
	sortingStatisticsDisplay("~=~ quickSort", &s);
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
		sortingStatisticsDisplay("~=~ balancingQuickSort", &s);
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
	sortingStatisticsDisplay("mergeSort", &s);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• mergeSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• mergeSort not stable\n");
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
		sortingStatisticsDisplay("inPlaceMergeSort", &s);
		if ( !isAscending(array, count, size, compare, context) ) {
			printf("•• inPlaceMergeSort not ascending\n");
		} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
			printf("•• inPlaceMergeSort not stable\n");
		}
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
	sortingStatisticsDisplay("seriesMergeSort", &s);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• seriesMergeSort not ascending\n");
	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
		printf("•• seriesMergeSort not stable\n");
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
	sortingStatisticsDisplay("~=~ heapSort", &s);
	if ( !isAscending(array, count, size, compare, context) ) {
		printf("•• heapSort not ascending\n");
//	} else if ( stableCompare && !isAscending(array, count, size, stableCompare, stableContext) ) {
//		printf("•• heapSort not stable\n");
	}
	
	free(buffer);
}

void sortingTest() {
	void *array;
	unsigned index, count, tooth, teeth = 50;
	unsigned integerArray[] = {6, 3, 5, 99, 44, 37, 9, 66, 15, 69, 85, 1, 57, 19, 22, 98, 24, 73, 11, 13, 7, 42, 17, 23};
	char const *stringArray[] = {"dog", "cat", "elk", "bat", "fox", "ape", "red", "orange", "yellow", "green", "blue", "indigo", "violet", "azure", "viridian", "cerulean", "teal", "sepia", "umber", "cerise", "sienna", "crimson", "periwinkle"};
	unsigned integerArrayCounts[] = {101, 1009, 10007, 100003, 1000003, 4000037};
	unsigned integerArrayCount = countof(integerArrayCounts);
	unsigned stringArrayCounts[] = {101, 1009, 10007, 100003, 1000003};
	unsigned stringArrayCount = countof(stringArrayCounts);
	unsigned scratch;
	
	printf("-- sort small known integerArray %lu\n", countof(integerArray));
	sortingComparison(integerArray, countof(integerArray), sizeof(integerArray[0]), (Compare *)compareUnsigned, NULL, NULL, NULL);
	
	printf("-- sort small known stringArray %lu\n", countof(stringArray));
	sortingComparison(stringArray, countof(stringArray), sizeof(stringArray[0]), (Compare *)compareString, NULL, NULL, NULL);
	
	for ( index = 3 ; index < 4 ; ++index ) {
		count = integerArrayCounts[index];
		array = allocateRandomIntegerArray(count);
		quickSort(array, count, sizeof(unsigned), &scratch, NULL, (Compare *)compareUnsigned, NULL);
		printf("-- sort ascending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		reverse(array, count, sizeof(unsigned), &scratch, NULL);
		printf("-- sort descending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		if ( count > teeth * 4 ) {
			for ( tooth = 0 ; tooth < teeth ; ++tooth ) {
				reverse(array + tooth * sizeof(unsigned) * count / teeth, count / 2 / teeth, sizeof(unsigned), &scratch, NULL);
			}
			
			printf("-- sort oscillating unsigned array %u\n", count);
			sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
			
			reverse(array, count, sizeof(unsigned), &scratch, NULL);
			printf("-- sort reversed oscillating unsigned array %u\n", count);
			sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
			
			reverse(array, count, sizeof(unsigned), &scratch, NULL);
			for ( tooth = 0 ; tooth < teeth ; ++tooth ) {
				populateRandomIntegerArray(array + tooth * sizeof(unsigned) * count / teeth, count / 2 / teeth);
			}
			
			printf("-- sort alternating random and ascending unsigned array %u\n", count);
			sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
			
			reverse(array, count, sizeof(unsigned), &scratch, NULL);
			printf("-- sort alternating random and descending unsigned array %u\n", count);
			sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		}
		
		tooth = count / 4;
		quickSort(array, count, sizeof(unsigned), &scratch, NULL, (Compare *)compareUnsigned, NULL);
		populateRandomIntegerArray(array + (count - tooth), tooth);
		printf("-- sort lead 3/4 ascending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateRandomIntegerArray(array, tooth);
		printf("-- sort middle 1/2 ascending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		quickSort(array, count - tooth, sizeof(unsigned), &scratch, NULL, (Compare *)compareUnsigned, NULL);
		printf("-- sort tail 3/4 ascending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		reverse(array, count, sizeof(unsigned), &scratch, NULL);
		printf("-- sort lead 3/4 descending unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		
		populateStabilityTestingRandomIntegerArray(array, count);
		printf("-- sort stability testing unsigned array %u\n", count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareStabilityTestingUnsigned, NULL, (Compare *)compareUnsigned, NULL);
		
		free(array);
	}
	
	for ( index = 0 ; index < stringArrayCount ; ++index ) {
		count = stringArrayCounts[index];
		array = allocateRandomStringArray(count, 20);
		printf("-- sort random string array %u (n ln n = %.0f)\n", count, log((double)count) * (double)count);
		sortingComparison(array, count, sizeof(char *), (Compare *)compareString, NULL, NULL, NULL);
		free(array);
	}
	
	for ( index = 0 ; index < integerArrayCount ; ++index ) {
		count = integerArrayCounts[index];
		array = allocateRandomIntegerArray(count);
		printf("-- sort random unsigned array %u (n ln n = %.0f)\n", count, log((double)count) * (double)count);
		sortingComparison(array, count, sizeof(unsigned), (Compare *)compareUnsigned, NULL, NULL, NULL);
		free(array);
	}
}
