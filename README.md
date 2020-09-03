# scortch
Sorting algorithms in C

## ColeSort

Introducing ColeSort, a faster merge sort.  Also introducing many failed experiments.

ColeSort takes advantage of existing runs in the data being sorted.  Doing so introduces imbalance in the merges that can result in excessive comparisons.  ColeSort ameliorates this by using an alternate merge strategy on imbalanced merges.  ColeSort borrows from QuadSort the technique of merging four runs at a time.

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
Similar to merge sort but recursion stops at 96 and groups of 4 elements are sorted then merges 32 segments at a time as recursion unwinds.

### bottomUpPolymergeSort (stable)
Like polymergeSort but uses iteration instead of recursion.

### tumbleMergeSort (stable)
Seeks then merges many existing runs in a tumbling cascade of recursion.

### juggleMergeSort (stable)
Like insertion merge sort but at odd levels of recursion the merged results are left in the buffer for the next recursion level to merge back into the array.

### coleSort (stable)
Similar to merge sort but uses natural ascending or descending runs when found and merges four segments at a time instead of two.  Has alternate merging techniques for imbalanced runs.

Converting this method to use < instead of <=> will give up one optimization for handling descending runs that contain equal elements.  Without the optimization, equal elements will end descending runs to preserve stability.

### quadSort (stable)
Similar to merge sort with optimized four element sort that merges four segments at a time instead of two.  Ported from scandum/quadsort repository with some optimizations removed.

### heapSort (unstable, in place)
Classic heap sort where elements are organized into a tree structure then pulled from the tree in order.

### quickSort (unstable, in place)
Classic quick sort using median of three partition scheme and using binary insertion sort once a partition is reduced to 16 or fewer elements.

### balancingQuickSort (unstable, in place)
Like quick sort but falls back to slower but more consistent heap sort when partition imbalances are detected.


## Notes

- All the algorithms use a compare method that returns <=> 0 but almost all are written to work with a compare method that returns true for less than and false otherwise.
- All merge variants require a buffer the same size as the data being sorted.  Some can adapt to a smaller array by using a slower in place merge.


## Tests

Current test suite includes the following test arrays:

- Known unsigned integer (length 24)
- Known string (length 23)
- Unsigned integers with chosen characteristics (length 1024, 10007, 100003)
  - equal
  - ascending
  - descending
  - random
  - runless (random with no runs above length 2)
  - 3/4 ascending, 1/4 random
  - 1/4 random, 3/4 ascending
  - 1/4 random, 1/2 ascending, 1/4 random
  - 1/4 ascending, 1/2 random, 1/4 ascending
  - 3/4 descending, 1/4 random
  - 1/4 random, 3/4 descending
  - 1/4 random, 1/2 descending, 1/4 random
  - 1/4 descending, 1/2 random, 1/4 descending
  - alternating random, equal segments of length n/⌊√n/4⌋
  - alternating random, ascending segments of length n/⌊√n/4⌋
  - alternating random, descending segments of length n/⌊√n/4⌋
  - alternating ascending, descending segments of length n/⌊√n/4⌋
  - alternating random, equal, descending, ascending segments of length n/⌊√n/4⌋
  - alternating ascending, random, descending, random segments of length n/⌊√n/4⌋
  - alternating descending, random, ascending, random segments of length n/⌊√n/4⌋
  - random with stability testing values
- Random strings with similar prefixes (length 102, 1024, 10007, 100003, 1000003)
- Random unsigned integers (length 102, 1024, 10007, 100003, 1000003, 4000037)

For each test, the same array is given to every algorithm 3 times and the best time is shown for each algorithm.  Other statistics are used from the last invocation of each algorithm.

Most content is randomly generated for each test but a seed can be programmatically set to repeat the same tests across multiple runs.

Tests measure the following statistics:
- invocations (of recursive and significant functions)
- comparisons (of elements)
- assignments (of elements to array or buffer, excluding temporary)
- writes (to array or buffer, coalescing sequential writes)
- accesses (of elements in array or buffer)


## Results

The logic to gather other statistics may affect the timing.  It is assumed to be negligible.  The size of array elements is a parameter and all element movement uses memmove and memcpy.  It is assumed that using hard coded sizes and types could improve performance.  The gettimeofday method used for timing is not intended for that purpose and not well suited to small intervals.  Allocating the array used by merge sort variants is not included in the timing.

Unsigned integers are used for simplicity in testing, but the timing is unlikely to reflect real world sorting.  The number of comparisons is a good metric, but it ignores algorithm complexity and data movement that will tip the scales for most algorithms.  I generally consider timings within a few percent of each other as equal, then use comparison count as a tie breaker.

Any useful sort algorithm must be able to sort random data.  What makes a sort algorithm interesting is taking advantage of existing patterns in the data without adversely affecting the case of random data or unrecognized patterns.

There are around 80 tests applied to a dozen sort algorithms, so this is just a sample.


    -- sort ascending unsigned array 10007
            ~=~ quickSort    125965 <       482 =  0.00094 @      2047 () 
                mergeSort     64652 <    129304 =  0.00122 @     10006 () 
                 coleSort     10006 <         0 =  0.00008 @         1 () 
    -- sort descending unsigned array 10007
            ~=~ quickSort    125954 <     10488 =  0.00097 @      2047 () 
                mergeSort     69597 <    197597 =  0.00187 @     10006 () 
                 coleSort     10006 <     12222 =  0.00018 @         1 () 
    -- sort random unsigned array 10007
            ~=~ quickSort    140362 <     76637 =  0.00228 @      2103 () 
                mergeSort    120522 <    190795 =  0.00284 @     10006 () 
                 coleSort    126047 <    139309 =  0.00245 @      2815 () 
    -- sort runless unsigned array 10007
            ~=~ quickSort    143407 <     76543 =  0.00229 @      2123 () 
                mergeSort    120639 <    190748 =  0.00275 @     10006 () 
                 coleSort    125626 <    138461 =  0.00244 @      2815 () 
    -- sort random string array 1024 (n log2 n = 10240)
            ~=~ quickSort     10477 <      6440 = 0.000335 @       219 () 
                mergeSort      8925 <     14696 = 0.000311 @      1023 () 
                 coleSort      9167 <     10555 = 0.000284 @       288 () 
    -- sort random string array 1000003 (n log2 n = 19931633)
            ~=~ quickSort  22102568 <  11045986 =    0.921 @    212129 () 
                mergeSort  18674055 <  29165334 =    0.777 @   1000002 () 
                 coleSort  18931254 <  20336450 =    0.696 @    281231 () 
    -- sort random unsigned array 1024 (n log2 n = 10240)
            ~=~ quickSort     11048 <      6049 = 0.000279 @       213 () 
                mergeSort      8934 <     14681 = 0.000345 @      1023 () 
                 coleSort      9195 <     10471 = 0.000284 @       288 () 
    -- sort random unsigned array 4000037 (n log2 n = 87727139)
            ~=~ quickSort  98155990 <  46997165 =    1.448 @    841753 () 
                mergeSort  82698465 < 128660870 =    1.821 @   4000036 () 
                 coleSort  83830215 <  89402624 =    1.588 @   1124922 () 


## Contact

eric x cole gmail com
