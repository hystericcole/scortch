# scortch
Sorting algorithms in C

## This project was inspired by scandum / quadsort.


### binaryInsertionSort (stable, in place)
Like classic insertion sort, but does a binary search for the insertion location instead of swapping as it searches.

### binaryMoveSort (stable)
Like classic insertion sort, but does a binary search for the insertion position and
puts the sorted results into a separate buffer.

### mergeSort (stable, optionally in place)
Classic merge sort, splitting the array recursively until each array is implicitly sorted by having only 1 element then merging adjacent sorted segments as recursion unwinds.

### insertionMergeSort (stable, optionally in place)
Like classic merge sort but recursions stops at a small threshold size where segments are sorted using insertion sort.

### bottomUpMergeSort (stable, optionally in place)
Like classic merge sort but using iteration instead of recursion.

### seriesMergeSort (stable, optionally in place)
Similar to merge sort but seeks existing runs of ascending or descending elements instead
of recursing until segments have 1 element.

### mergeFourSort (stable)
Similar to merge sort but recursion stops at 16 and groups of 4 elements are sorted then merges 4 segments at a time as recursion unwinds.

### bottomUpMergeFourSort (stable)
Like mergeFourSort but uses iteration instead of recursion.

### polymergeSort (stable)
Similar to merge sort but recursion stops at 96 and groups of 4 elements are sorted then merges 32 segments at a time as recursion unwinds.  Fastest merge sort variation for random data.

### bottomUpPolymergeSort (stable)
Like polymergeSort but uses iteration instead of recursion.

### tumbleMergeSort (stable)
Seeks then merges many existing runs in a tumbling cascade of recursion.

### juggleMergeSort (stable)
Like insertion merge sort but at odd levels of recursion the merged results are left in the buffer for the next recursion level to merge back into the array.

### coleSort (stable)
Similar to merge sort but uses natural ascending or descending runs when found and merges four segments at a time instead of two.  Has alternate merging techniques for imbalanced runs.

Converting this method to use < instead of <=> will give up one optimization for handling descending runs that contain equal elements.  Without the optimization, equal elements will end descending runs to preserve stability.

### heapSort (unstable, in place)
Classic heap sort where elements are organized into a tree structure then pulled from the tree in order.

### quickSort (unstable, in place)
Classic quick sort using median of three partition scheme and using binary insertion sort once a partition is reduced to 16 or fewer elements.

### balancingQuickSort (unstable, in place)
Like quick sort but falls back to slower but more consistent heap sort when partition imbalances are detected.


## Considerations

* The sorting algorithms all take a statistics object to facilitate more accurate comparison but this will have a slight negative impact on wall clock timings.
* The sorting algorithms all move and swap data using mem* functions so an easy optimization is to hard code the size of elements.
* The sorting algorithms are all designed to operate on an array in memory from a single thread.
* The sorting algorithms use a compare method that returns <=> 0 however most are written in such a way that they would work with a < method.
* Most of the merge sort variants require a buffer of the same size as the data being sorted, but a few will dynamically adapt to the space provided by sacrificing speed.
