//
//  sort.h
//  Scortch
//
//  Created by Eric Cole on 8/8/20.
//  Copyright © 2020 Eric Cole. All rights reserved.
//

#ifndef sort_h
#define sort_h

typedef unsigned IsLess(void const *, void const *, void *);
typedef signed Compare(void const *, void const *, void *);

struct SortingStatistics {
	long invocations;
	long accesses;
	long assignments;
	long comparisons;
	
	long timerBegan;
	long timerEnded;
};

void reverse(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics);

void binaryInsertionSort(void *array, size_t count, size_t size, size_t sorted, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context);
void binaryMoveSort(void *unsorted, void *sorted, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);

void mergeSort(void *array, size_t count, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context);
void insertionMergeSort(void *array, size_t count, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context);
void bottomUpMergeSort(void *array, size_t count, size_t size, size_t width, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context);

void seriesMergeSort(void *array, size_t count, size_t size, void *buffer, size_t space, struct SortingStatistics *statistics, Compare compare, void *context);

void mergeFourSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);
void bottomUpMergeFourSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);

void coleSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);

void tumbleMergeSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);
void polymergeSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);
void bottomUpPolymergeSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context);
void juggleMergeSort(void *array, void *buffer, size_t count, size_t size, unsigned juggling, struct SortingStatistics *statistics, Compare compare, void *context);

void heapSort(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context);
void quickSort(void *array, size_t count, size_t size, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context);
void balancingQuickSort(void *array, size_t count, size_t size, unsigned imbalances, void *temporary, struct SortingStatistics *statistics, Compare compare, void *context);

#endif /* sort_h */
