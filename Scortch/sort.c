//
//  sort.c
//  Scortch
//
//  Created by Eric Cole on 8/8/20.
//  Copyright © 2020 Eric Cole. All rights reserved.
//

#include <string.h>
#include "sort.h"

struct PointerCount {
	void const *p;
	size_t n;
};

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
		assignManyAt(buffer, 0, count - split, size, array + split * size, statistics);
		
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
		assignManyAt(buffer, 0, split, size, array, statistics);
		
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

//	MARK: - Four Sort

void fourSort(void *array, size_t size, void *buffer, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
		statistics->accesses += 4;
	}
	
	unsigned reverseLower = invokeIsLess(array + 1 * size, array + 0 * size, statistics, compare, context) ? 1 : 0;
	unsigned reverseUpper = invokeIsLess(array + 3 * size, array + 2 * size, statistics, compare, context) ? 1 : 0;
	
	assignAt(buffer, 0, size, array + (0 + reverseLower) * size, statistics);
	assignAt(buffer, 1, size, array + (1 - reverseLower) * size, statistics);
	assignAt(buffer, 2, size, array + (2 + reverseUpper) * size, statistics);
	assignAt(buffer, 3, size, array + (3 - reverseUpper) * size, statistics);
	
	if ( !invokeIsLess(buffer + 2 * size, buffer + 1 * size, statistics, compare, context) ) {
		if ( reverseLower ) {
			assignAt(array, 0, size, buffer + 0 * size, statistics);
			assignAt(array, 1, size, buffer + 1 * size, statistics);
		}
		
		if ( reverseUpper ) {
			assignAt(array, 2, size, buffer + 2 * size, statistics);
			assignAt(array, 3, size, buffer + 3 * size, statistics);
		}
	} else if ( invokeIsLess(buffer + 3 * size, buffer + 0 * size, statistics, compare, context) ) {
		assignAt(array, 0, size, buffer + 2 * size, statistics);
		assignAt(array, 1, size, buffer + 3 * size, statistics);
		assignAt(array, 2, size, buffer + 0 * size, statistics);
		assignAt(array, 3, size, buffer + 1 * size, statistics);
	} else {
		if ( invokeIsLess(buffer + 2 * size, buffer + 0 * size, statistics, compare, context) ) {
			assignAt(array, 0, size, buffer + 2 * size, statistics);
			assignAt(array, 1, size, buffer + 0 * size, statistics);
		} else {
			if ( reverseLower ) {
				assignAt(array, 0, size, buffer + 0 * size, statistics);
			}
			
			assignAt(array, 1, size, buffer + 2 * size, statistics);
		}
		
		if ( invokeIsLess(buffer + 3 * size, buffer + 1 * size, statistics, compare, context) ) {
			assignAt(array, 2, size, buffer + 3 * size, statistics);
			assignAt(array, 3, size, buffer + 1 * size, statistics);
		} else {
			assignAt(array, 2, size, buffer + 1 * size, statistics);
			
			if ( reverseUpper ) {
				assignAt(array, 3, size, buffer + 3 * size, statistics);
			}
		}
	}
}

void mergeIntoSorted(void const *unmerged, void *merged, size_t count, size_t split, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t i, j, n;
	
	for ( i = 0, j = split, n = 0 ; i < split && j < count ; ++n ) {
		if ( invokeIsLess(unmerged + j * size, unmerged + i * size, statistics, compare, context) ) {
			assignAt(merged, n, size, unmerged + j * size, statistics);
			j += 1;
		} else {
			assignAt(merged, n, size, unmerged + i * size, statistics);
			i += 1;
		}
	}
	
	if ( i < split ) {
		assignManyAt(merged, n, split - i, size, unmerged + i * size, statistics);
	}
	
	if ( j < count ) {
		assignManyAt(merged, n, count - j, size, unmerged + j * size, statistics);
	}
}

void mergeFourSorted(void *array, void *buffer, size_t count, size_t width, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t half = width * 2 < count ? width * 2 : count;
	size_t last = width * 3;
	
	signed lowerOrder, upperOrder, order;
	
	if ( count <= width ) {
		return;
	} else if ( !invokeIsLess(array + width * size, array + (width - 1) * size, statistics, compare, context) ) {
		lowerOrder = 1;
	} else if ( invokeIsLess(array + (half - 1) * size, array + 0 * size, statistics, compare, context) ) {
		lowerOrder = -1;
	} else {
		lowerOrder = 0;
	}
	
	if ( count <= last ) {
		upperOrder = 2;
	} else if ( !invokeIsLess(array + last * size, array + (last - 1) * size, statistics, compare, context) ) {
		upperOrder = 1;
	} else if ( invokeIsLess(array + (count - 1) * size, array + half * size, statistics, compare, context) ) {
		upperOrder = -1;
	} else {
		upperOrder = 0;
	}
	
	if ( lowerOrder && upperOrder ) {
		if ( count <= half ) {
			order = 2;
		} else if ( !invokeIsLess(array + (upperOrder > 0 ? half : last) * size, array + ((lowerOrder > 0 ? half : width) - 1) * size, statistics, compare, context) ) {
			order = 1;
		} else if ( invokeIsLess(array + ((upperOrder > 0 ? count : last) - 1) * size, array + (lowerOrder > 0 ? 0 : width) * size, statistics, compare, context) ) {
			order = -1;
		} else {
			order = 0;
		}
		
		if ( order > 0 ) {
			if ( lowerOrder < 0 ) {
				assignManyAt(buffer, 0, width, size, array, statistics);
				assignManyAt(array, 0, half - width, size, array + width * size, statistics);
				assignManyAt(array, half - width, width, size, buffer, statistics);
			}
			
			if ( upperOrder < 0 ) {
				assignManyAt(buffer, 0, width, size, array + half * size, statistics);
				assignManyAt(array, half, count - last, size, array + last * size, statistics);
				assignManyAt(array, count - width, width, size, buffer, statistics);
			}
			
			return;
		}
		
		if ( order < 0 ) {
			if ( lowerOrder < 0 ) {
				assignManyAt(buffer, 0, half - width, size, array + width * size, statistics);
				assignManyAt(buffer, half - width, width, size, array, statistics);
			} else {
				assignManyAt(buffer, 0, half, size, array, statistics);
			}
			
			if ( upperOrder < 0 ) {
				assignManyAt(array, 0, count - last, size, array + last * size, statistics);
				assignManyAt(array, count - last, width, size, array + half * size, statistics);
			} else {
				assignManyAt(array, 0, count - half, size, array + half * size, statistics);
			}
			
			assignManyAt(array, count - half, half, size, buffer, statistics);
			
			return;
		}
	}
	
	if ( lowerOrder < 0 ) {
		assignManyAt(buffer, 0, half - width, size, array + width * size, statistics);
		assignManyAt(buffer, half - width, width, size, array, statistics);
	} else if ( lowerOrder > 0 ) {
		assignManyAt(buffer, 0, half, size, array, statistics);
	} else {
		mergeIntoSorted(array, buffer, half, width, size, statistics, compare, context);
	}
	
	if ( upperOrder < 0 ) {
		assignManyAt(buffer, half, count - last, size, array + last * size, statistics);
		assignManyAt(buffer, count - width, width, size, array + half * size, statistics);
	} else if ( upperOrder > 0 ) {
		assignManyAt(buffer, half, count - half, size, array + half * size, statistics);
	} else {
		mergeIntoSorted(array + half * size, buffer + half * size, count - half, width, size, statistics, compare, context);
	}
	
	mergeIntoSorted(buffer, array, count, half, size, statistics, compare, context);
}

void mergeFourSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	size_t index, width;
	
	if ( count <= 1 ) {
		return;
	} else if ( count <= 16 ) {
		width = 4;
		
		for ( index = 0 ; index + 4 <= count ; index += 4 ) {
			fourSort(array + index * size, size, buffer, statistics, compare, context);
		}
		
		if ( index + 1 < count ) {
			binaryInsertionSort(array + index * size, count - index, size, 1, buffer, statistics, compare, context);
		}
	} else {
		width = count > 64 ? (count + 3) / 4 : 16;
		
		for ( index = 0 ; index + width <= count ; index += width ) {
			mergeFourSort(array + index * size, buffer, width, size, statistics, compare, context);
		}
		
		if ( index + 1 < count ) {
			mergeFourSort(array + index * size, buffer, count - index, size, statistics, compare, context);
		}
	}
	
	mergeFourSorted(array, buffer, count, width, size, statistics, compare, context);
}

///	Bottom up merge sort that quadruples instead of doubles at each iteration
void bottomUpMergeFourSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	size_t block, index, limit, width = 4;
	
	for ( index = 0 ; index + width <= count ; index += width ) {
		fourSort(array + index * size, size, buffer, statistics, compare, context);
	}
	
	if ( index + 1 < count ) {
		binaryInsertionSort(array + index * size, count - index, size, 1, buffer, statistics, compare, context);
	}
	
	while ( width < count ) {
		block = width * 4;
		
		for ( index = 0 ; index + width < count ; index += block ) {
			limit = count - index < block ? count - index : block;
			
			mergeFourSorted(array + index * size, buffer, limit, width, size, statistics, compare, context);
		}
		
		width = block;
	}
}

//	MARK: - Cole Sort

void coleMergeIntoSorted(void const *unmerged, void *merged, size_t count, size_t split, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t i, j, m, n, o = count - split;
	
	if ( 0 < split && split < count ) {
		//	small + large > small * log2(large)
		//	+3 to avoid edge cases for very small values
		
		if ( (o + 3) >> (o / split) <= 1 ) {
			for ( i = 0, j = split ; i < split ; ++i ) {
				m = j;
				n = count - 1;
				
				while ( m <= n ) {
					o = (m + n) / 2;
					
					if ( invokeIsLess(unmerged + i * size, unmerged + o * size, statistics, compare, context) ) {
						n = o - 1;
					} else {
						m = o + 1;
					}
				}
				
				assignManyAt(merged, i + j - split, m - j, size, unmerged + j * size, statistics);
				assignAt(merged, i + m - split, size, unmerged + i * size, statistics);
				
				j = m;
			}
			
			assignManyAt(merged, j, count - j, size, unmerged + j * size, statistics);
			return;
		}
		
		if ( (split + 3) >> (split / o) <= 1 ) {
			for ( i = 0, j = split ; j < count ; ++j ) {
				m = i + 1;
				n = split;
				
				while ( m <= n ) {
					o = (m + n) / 2 - 1;
					
					if ( invokeIsLess(unmerged + j * size, unmerged + o * size, statistics, compare, context) ) {
						n = o;
					} else {
						m = o + 2;
					}
				}
				
				m -= 1;
				
				assignManyAt(merged, i + j - split, m - i, size, unmerged + i * size, statistics);
				assignAt(merged, j + m - split, size, unmerged + j * size, statistics);
				
				i = m;
			}
			
			assignManyAt(merged, count - split + i, split - i, size, unmerged + i * size, statistics);
			return;
		}
	}
	
	for ( i = 0, j = split, n = 0 ; i < split && j < count ; ++n ) {
		if ( invokeIsLess(unmerged + j * size, unmerged + i * size, statistics, compare, context) ) {
			assignAt(merged, n, size, unmerged + j * size, statistics);
			j += 1;
		} else {
			assignAt(merged, n, size, unmerged + i * size, statistics);
			i += 1;
		}
	}
	
	if ( i < split ) {
		assignManyAt(merged, n, split - i, size, unmerged + i * size, statistics);
	}
	
	if ( j < count ) {
		assignManyAt(merged, n, count - j, size, unmerged + j * size, statistics);
	}
}

void coleMergeSorted(void *array, size_t runs[4], size_t size, void *buffer, struct SortingStatistics *statistics, Compare compare, void *context) {
	size_t a = 0, b = runs[0], c = runs[1] + b, d = runs[2] + c, e = runs[3] + d;
	
	if ( b == e ) {
		return;
	} else if ( b == c || !invokeIsLess(array + b * size, array + (b - 1) * size, statistics, compare, context) ) {
		assignManyAt(buffer, a, c - a, size, array, statistics);
	} else if ( invokeIsLess(array + (c - 1) * size, array + a * size, statistics, compare, context) ) {
		assignManyAt(buffer, a, c - b, size, array + b * size, statistics);
		assignManyAt(buffer, a + c - b, b - a, size, array, statistics);
	} else {
		coleMergeIntoSorted(array, buffer, c - a, b - a, size, statistics, compare, context);
	}
	
	if ( c == e ) {
		//
	} else if ( d == e || !invokeIsLess(array + d * size, array + (d - 1) * size, statistics, compare, context) ) {
		assignManyAt(buffer, c, e - c, size, array + c * size, statistics);
	} else if ( invokeIsLess(array + (e - 1) * size, array + c * size, statistics, compare, context) ) {
		assignManyAt(buffer, c, e - d, size, array + d * size, statistics);
		assignManyAt(buffer, c + e - d, d - c, size, array + c * size, statistics);
	} else {
		coleMergeIntoSorted(array + c * size, buffer + c * size, e - c, d - c, size, statistics, compare, context);
	}
	
	coleMergeIntoSorted(buffer, array, e, c, size, statistics, compare, context);
}

size_t coleSeek(void *array, void *buffer, size_t count, size_t size, size_t minimum, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	if ( count < 2 ) {
		return count;
	}
	
	size_t minimumRun = 8;
	size_t runs[4] = {};
	size_t run = 0, sum = 0, seek, limit, equals;
	unsigned i, isReversed;
	signed c;
	
	for ( i = 0 ; i < 4 && run + 1 < count ; ++i ) {
		isReversed = invokeIsLess(array + (run + 1) * size, array + (run + 0) * size, statistics, compare, context);
		run += 2;
		
		if ( isReversed ) {
			equals = 0;
			
			while ( 1 ) {
				if ( run < count ) {
					c = invokeCompare(array + run * size, array + (run - 1) * size, statistics, compare, context);
				} else {
					c = 1;
				}
				
				if ( c == 0 ) {
					equals += 1;
				} else if ( equals > 0 ) {
					//	preserve stability of equal elements within descending runs
					reverse(array + (run - equals - 1) * size, equals + 1, size, buffer, statistics);
					equals = 0;
				}
				
				if ( c > 0 ) {
					break;
				}
				
				run += 1;
			}
		} else {
			while ( run < count && isReversed == invokeIsLess(array + run * size, array + (run - 1) * size, statistics, compare, context) ) {
				run += 1;
			}
		}
		
		if ( isReversed ) {
			reverse(array + sum * size, run - sum, size, buffer, statistics);
		}
		
		if ( run < sum + minimumRun && run + 1 < minimum ) {
			limit = count < sum + minimumRun ? count - sum : minimumRun;
			
			binaryInsertionSort(array + sum * size, limit, size, run - sum, buffer, statistics, compare, context);
			
			run = sum + limit;
		}
		
		runs[i] = run - sum;
		sum = run;
	}
	
	if ( run + 1 == count ) {
		limit = runs[i - 1];
		
		if ( i < 4 ) {
			runs[i] = 1;
			i += 1;
		} else {
			binaryInsertionSort(array + (sum - limit) * size, limit + 1, size, limit, buffer, statistics, compare, context);
			runs[i - 1] += 1;
		}
		
		run += 1;
		sum = run;
	}
	
	coleMergeSorted(array, runs, size, buffer, statistics, compare, context);
	
	while ( run < minimum ) {
		runs[0] = run;
		runs[2] = 0;
		runs[3] = 0;
		seek = run > minimumRun * 4 ? run * 3 / 4 : run;
		
		for ( i = 1 ; i < 4 && sum < count ; ++i ) {
			limit = count - sum;
			seek = limit < seek ? limit : seek;
			
			run = coleSeek(array + sum * size, buffer, limit, size, seek, statistics, compare, context);
			sum += run;
			runs[i] = run;
		}
		
		if ( i == 3 && runs[2] < runs[0] && (runs[0] + runs[1] + 3) >> ((runs[0] + runs[1]) / runs[2]) > 1 ) {
			runs[3] = runs[2];
			runs[2] = runs[1];
			runs[1] = 0;
			i = 4;
		}
		
		coleMergeSorted(array, runs, size, buffer, statistics, compare, context);
		run = sum;
	}
	
	return sum;
}

///	Merge sort that operates on four runs at a time, starting with natural ascending or descending runs
void coleSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 16 ) {
		binaryInsertionSort(array, count, size, 1, buffer, statistics, compare, context);
		return;
	}
	
	coleSeek(array, buffer, count, size, count, statistics, compare, context);
}

//	MARK: - Tumble Marge Sort

#define kTumbleMaximumRuns 32

size_t tumbleMergeIntoSorted(void const *unmerged, void *merged, size_t runs[], unsigned runCount, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t index, total = 0;
	struct PointerCount pointers[kTumbleMaximumRuns];
	struct PointerCount temporary[1];
	unsigned i, m, n, o, valid = runCount;
	
	for ( i = 0 ; i < valid ; ++i ) {
		index = runs[i];
		pointers[i].p = unmerged + total * size;
		pointers[i].n = index;
		total += index;
	}
	
	for ( i = 1 ; i < valid ; ++i ) {
		m = 1;
		n = i;
		
		while ( m <= n ) {
			o = (m + n) / 2;
			
			if ( invokeIsLess(pointers[i].p, pointers[o - 1].p, statistics, compare, context) ) {
				n = o - 1;
			} else {
				m = o + 1;
			}
		}
		
		if ( m <= i ) {
			slideDown(pointers, i, m - 1, sizeof(struct PointerCount), temporary, NULL);
		}
	}
	
	for ( index = 0 ; index < total ; ++index ) {
		assignAt(merged, index, size, pointers[0].p, statistics);
		
		pointers[0].n -= 1;
		
		if ( pointers[0].n > 0 ) {
			pointers[0].p += size;
			
			m = 1;
			n = valid - 1;
			
			while ( m <= n ) {
				o = (m + n) / 2;
				
				if ( invokeStableIsLess(pointers[0].p, pointers[o].p, statistics, compare, context) ) {
					n = o - 1;
				} else {
					m = o + 1;
				}
			}
			
			if ( m > 1 ) {
				slideUp(pointers, 0, m - 1, sizeof(struct PointerCount), temporary, NULL);
			}
		} else {
			valid -= 1;
			
			slideUp(pointers, 0, valid, sizeof(struct PointerCount), temporary, NULL);
		}
	}
	
	return total;
}

unsigned tumbleMergeSorted(void *unmerged, size_t runs[], unsigned count, size_t size, void *merged, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count > 2 ) {
		tumbleMergeIntoSorted(unmerged, merged, runs, count, size, statistics, compare, context);
	} else if ( count > 1 ) {
		coleMergeIntoSorted(unmerged, merged, runs[0] + runs[1], runs[0], size, statistics, compare, context);
	} else {
		return 0;
	}
	
	return 1;
}

size_t tumbleMergeSeek(void *array, void *buffer, size_t count, size_t size, size_t minimum, unsigned juggling, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	if ( count < 2 ) {
		return count;
	}
	
	size_t runs[kTumbleMaximumRuns];
	size_t run = 0, sum = 0, seek, limit;
	unsigned i, isReversed, resultsInBuffer;
	
	for ( i = 0 ; i < kTumbleMaximumRuns && run + 1 < count ; ++i ) {
		isReversed = invokeIsLess(array + (run + 1) * size, array + (run + 0) * size, statistics, compare, context);
		run += 2;
		
		while ( run < count && isReversed == invokeIsLess(array + run * size, array + (run - 1) * size, statistics, compare, context) ) {
			run += 1;
		}
		
		if ( isReversed ) {
			reverse(array + sum * size, run - sum, size, buffer, statistics);
		}
		
		runs[i] = run - sum;
		sum = run;
	}
	
	if ( run + 1 == count ) {
		limit = runs[i - 1];
		
		if ( i < kTumbleMaximumRuns && limit > 8 ) {
			runs[i] = 1;
			i += 1;
		} else {
			binaryInsertionSort(array + (sum - limit) * size, limit + 1, size, limit, buffer, statistics, compare, context);
			runs[i - 1] += 1;
		}
		
		run += 1;
		sum = run;
	}
	
	resultsInBuffer = tumbleMergeSorted(array, runs, i, size, buffer, statistics, compare, context);
	
	while ( run < minimum ) {
		runs[0] = run;
		seek = run * 3 / 4;
		
		for ( i = 1 ; i < kTumbleMaximumRuns && sum < count ; ++i ) {
			limit = count - sum;
			seek = limit < seek ? limit : seek;
			
			run = tumbleMergeSeek(array + sum * size, buffer + sum * size, limit, size, seek, resultsInBuffer, statistics, compare, context);
			sum += run;
			runs[i] = run;
		}
		
		void *merged = resultsInBuffer ? array : buffer;
		void *unmerged = resultsInBuffer ? buffer : array;
		
		resultsInBuffer ^= tumbleMergeSorted(unmerged, runs, i, size, merged, statistics, compare, context);
		run = sum;
	}
	
	if ( resultsInBuffer != juggling ) {
		void *target = resultsInBuffer ? array : buffer;
		void *source = resultsInBuffer ? buffer : array;
		
		assignManyAt(target, 0, sum, size, source, statistics);
	}
	
	return sum;
}

///	Merge sort that operates on many runs at a time, starting with natural ascending or descending runs
void tumbleMergeSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 16 ) {
		binaryInsertionSort(array, count, size, 1, buffer, statistics, compare, context);
		return;
	}
	
	tumbleMergeSeek(array, buffer, count, size, count, 0, statistics, compare, context);
}

//	MARK: - Polymerge Sort

#define kPolymergeMaximumRuns 32

void polymergeFourSort(void *unsorted, void *sorted, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
		statistics->accesses += 4;
	}
	
	unsigned reverseLower = invokeIsLess(unsorted + 1 * size, unsorted + 0 * size, statistics, compare, context) ? 1 : 0;
	unsigned reverseUpper = invokeIsLess(unsorted + 3 * size, unsorted + 2 * size, statistics, compare, context) ? 1 : 0;
	
	unsigned _0 = 0 + reverseLower;
	unsigned _1 = 1 - reverseLower;
	unsigned _2 = 2 + reverseUpper;
	unsigned _3 = 3 - reverseUpper;
	unsigned t;
	
	if ( !invokeIsLess(unsorted + _2 * size, unsorted + _1 * size, statistics, compare, context) ) {
		
	} else if ( invokeIsLess(unsorted + _3 * size, unsorted + _0 * size, statistics, compare, context) ) {
		t = _0; _0 = _2; _2 = t;
		t = _1; _1 = _3; _3 = t;
	} else {
		if ( invokeIsLess(unsorted + _2 * size, unsorted + _0 * size, statistics, compare, context) ) {
			t = _2; _2 = _1; _1 = _0; _0 = t;
		} else {
			t = _2; _2 = _1; _1 = t;
		}
		
		if ( invokeIsLess(unsorted + _3 * size, unsorted + _2 * size, statistics, compare, context) ) {
			t = _2; _2 = _3; _3 = t;
		}
	}
	
	assignAt(sorted, 0, size, unsorted + _0 * size, statistics);
	assignAt(sorted, 1, size, unsorted + _1 * size, statistics);
	assignAt(sorted, 2, size, unsorted + _2 * size, statistics);
	assignAt(sorted, 3, size, unsorted + _3 * size, statistics);
}

void polymergeSorted(void const *unmerged, void *merged, size_t count, size_t width, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	size_t index = 0, blocks = (count + width - 1) / width;
	struct PointerCount pointers[kPolymergeMaximumRuns + 4];
	unsigned i, m, n, o, valid = (unsigned)blocks;
	
	for ( i = 0 ; i < valid ; ++i ) {
		pointers[i].p = unmerged + index * size;
		pointers[i].n = width;
		index += width;
	}
	
	pointers[valid - 1].n = count - (blocks - 1) * width;
	
	for ( i = 1 ; i < valid ; ++i ) {
		m = 1;
		n = i;
		
		while ( m <= n ) {
			o = (m + n) / 2;
			
			if ( invokeIsLess(pointers[i].p, pointers[o - 1].p, statistics, compare, context) ) {
				n = o - 1;
			} else {
				m = o + 1;
			}
		}
		
		if ( m <= i ) {
			slideDown(pointers, i, m - 1, sizeof(struct PointerCount), pointers + blocks, NULL);
		}
	}
	
	for ( index = 0 ; index < count ; ++index ) {
		assignAt(merged, index, size, pointers[0].p, statistics);
		
		pointers[0].n -= 1;
		
		if ( pointers[0].n > 0 ) {
			pointers[0].p += size;
			
			m = 1;
			n = valid - 1;
			
			while ( m <= n ) {
				o = (m + n) / 2;
				
				if ( invokeStableIsLess(pointers[0].p, pointers[o].p, statistics, compare, context) ) {
					n = o - 1;
				} else {
					m = o + 1;
				}
			}
			
			if ( m > 1 ) {
				slideUp(pointers, 0, m - 1, sizeof(struct PointerCount), pointers + blocks, NULL);
			}
		} else {
			valid -= 1;
			
			slideUp(pointers, 0, valid, sizeof(struct PointerCount), pointers + blocks, NULL);
		}
	}
}

void polymergeJuggle(void *array, void *buffer, size_t count, size_t size, unsigned juggling, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	if ( count <= kPolymergeMaximumRuns * 4 ) {
		size_t index, width = 4;
		
		if ( !juggling != (width < count) ) {
			for ( index = 0 ; index + width <= count ; index += width ) {
				fourSort(array + index * size, size, buffer, statistics, compare, context);
			}
			
			if ( index + 1 < count ) {
				binaryInsertionSort(array + index * size, count - index, size, 1, buffer, statistics, compare, context);
			}
			
			if ( width < count ) {
				polymergeSorted(array, buffer, count, width, size, statistics, compare, context);
			}
		} else {
			for ( index = 0 ; index + width <= count ; index += width ) {
				polymergeFourSort(array + index * size, buffer + index * size, size, statistics, compare, context);
			}
			
			if ( index < count ) {
				binaryMoveSort(array + index * size, buffer + index * size, count - index, size, statistics, compare, context);
			}
			
			if ( width < count ) {
				polymergeSorted(buffer, array, count, width, size, statistics, compare, context);
			}
		}
	} else {
		size_t index, limit, width = (count + kPolymergeMaximumRuns - 1) / kPolymergeMaximumRuns;
		
		for ( index = 0 ; index < count ; index += width ) {
			limit = count - index < width ? count - index : width;
			
			polymergeJuggle(array + index * size, buffer + index * size, limit, size, !juggling, statistics, compare, context);
		}
		
		void *unmerged = juggling ? array : buffer;
		void *merged = juggling ? buffer : array;
		
		polymergeSorted(unmerged, merged, count, width, size, statistics, compare, context);
	}
}

///	Merge sort that operates on many runs at a time, starting with runs of four
void polymergeSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 2 ) {
		return;
	} else {
		polymergeJuggle(array, buffer, count, size, 0, statistics, compare, context);
	}
}

void bottomUpPolymergeSort(void *array, void *buffer, size_t count, size_t size, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( count < 4 ) {
		binaryInsertionSort(array, count, size, 1, buffer, statistics, compare, context);
		return;
	}
	
	size_t block, limit, index, width = 4;
	void *merged, *unmerged;
	unsigned resultsInBuffer = 0;
	
	while ( width < count ) {
		width *= kPolymergeMaximumRuns;
		resultsInBuffer ^= 1;
	}
	
	width = 4;
	
	if ( resultsInBuffer ) {
		for ( index = 0 ; index + width <= count ; index += width ) {
			polymergeFourSort(array + index * size, buffer + index * size, size, statistics, compare, context);
		}
		
		if ( index < count ) {
			binaryMoveSort(array + index * size, buffer + index * size, count - index, size, statistics, compare, context);
		}
	} else {
		for ( index = 0 ; index + width <= count ; index += width ) {
			fourSort(array + index * size, size, buffer, statistics, compare, context);
		}
		
		if ( index + 1 < count ) {
			binaryInsertionSort(array + index * size, count - index, size, 1, buffer, statistics, compare, context);
		}
	}
	
	while ( width < count ) {
		block = width * kPolymergeMaximumRuns;
		merged = resultsInBuffer ? array : buffer;
		unmerged = resultsInBuffer ? buffer : array;
		
		for ( index = 0 ; index < count ; index += block ) {
			limit = count - index < block ? count - index : block;
			
			polymergeSorted(unmerged + index * size, merged + index * size, limit, width, size, statistics, compare, context);
		}
		
		width = block;
		resultsInBuffer ^= 1;
	}
	
	if ( resultsInBuffer ) {
		assignManyAt(array, 0, count, size, buffer, statistics);
	}
}

//	MARK: - Juggle Merge Sort

void juggleMergeSort(void *array, void *buffer, size_t count, size_t size, unsigned juggling, struct SortingStatistics *statistics, Compare compare, void *context) {
	if ( statistics ) {
		statistics->invocations += 1;
	}
	
	if ( count < 2 ) {
		if ( juggling && count > 0 ) {
			assignManyAt(buffer, 0, count, size, array, statistics);
		}
	} else if ( count < 32 ) {
		if ( juggling ) {
			binaryMoveSort(array, buffer, count, size, statistics, compare, context);
		} else {
			binaryInsertionSort(array, count, size, 1, buffer, statistics, compare, context);
		}
	} else {
		size_t half = count / 2;
		
		juggleMergeSort(array, buffer, half, size, !juggling, statistics, compare, context);
		juggleMergeSort(array + half * size, buffer + half * size, count - half, size, !juggling, statistics, compare, context);
		
		void *unmerged = juggling ? array : buffer;
		void *merged = juggling ? buffer : array;
		
		mergeIntoSorted(unmerged, merged, count, half, size, statistics, compare, context);
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
