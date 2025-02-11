# Copyright 2024 ~ Dumitrescu Toma-Ioan

## Description:

The project calculates the inverted index using parallel computation for a number
of documents, using MapReduce paradigm, a classical design for fast-search engines.
After running the program, from {(file1, words1), ..., (filen, wordsn)}, the set
{(word1, files1), ..., (wordn, filesn)} is calculated. The algorithms focus on
minimizing time complexity, with the cost of spatial complexity.

## Implementation

1) Reading and initializing data:
- the file names are loaded in memory for time efficiency (another possibility is to have
an array of offsets, but the purpose of this algorithm is the speed)
- creating mappers and reducer threads, converting char* to int with atoi
- reducers divisions:

(i) grouping operation: diving the threads statically by equally distribute index intervals
from 0, 1, ..., mappers - 1

(ii) sorting: based on statically dividing the interval for a - z letters equally between
threads, so a start-end based on thread id logic

- mappers divisions: dynamically, a queue with file ids is initialized
- creating the threads and intiazing their arguments (generally with pointers for shared access)
- join the threads
- sequentially printing data to files, since doing this in parallel will cause overhead
- cleaning data and destroying pthread variables (mutexes and barriers)

2) Map:
Each mapper thread has its own dictionary, and will put pairs <string, set<int>> there when
processing files. Since the mappers have their work dynamically divided, a mapper thread will
lock the task queue (this way no other thread will pop faster so that the queu becomes empty
before the current thread notices), extract a file id or just exit the file processing loop if
there are no tasks left, together with unlocking the queue. From the files array, the queue
takes the string using that id from the queue. Basically, the file contents is read word by word,
for each word transform it to a standardized word (only lowercase english letters), and then verify
if its length is greater than 0 (because a word like "12" will be transformed to ""). Add the
standardized word to the thread dictionary. File reading is done via buffered ifstream. After
file processing loop, the thread calls barrier_wait(barrier), to notify barrier that the mapper
thread reached there.

Considerations:
- This operation is called map, because it applies a function over every element from the file list,
so the algorithm can be transled to: map((f process(f)) file_list).
- Mutex only over the critical part (extracting the task from the queue).

3) Reduce:
The reducer calls barrier_wait(barrier), so when the mapper finish, the number of threads waiting
at the barrier is mappers + reducers, so the barrier raises. For each assigned unordered_map (with
mapper origins), merge the pairs <string, set<int>> to the array of dictionaries (unordered_map)
for each a-z letter. Lock the critical section (write on the array of maps) for each mapper_dictionary,
then put the pair in the corresponding dictionary (O(1)). For example, if while processing the
mapper_dictionary, the pair ("car", {1, 2}), add this pair (or update the pair) in global_maps[2],
because 'c' letter corresponds to index 2. This operation does grouping and aggregating the partial
lists at the same time. This approach is time efficient, because of O(1) operations and because of
mutex locking of a letter-dictionary. If there was a global dictionary to put data in, there was only
one resource to lock/unlock, possibly leading to thread starvation. After processing dictionaries,
the reducers call barrier_wait(aggregate_barrier) to prepare for the sorting algorithm (because a-z
dictionaries should be complete for a correct sort). For this part, the (i) division was used. Now,
after the aggregate_barrier counts reducers threads, the second phase begins. For the sorting part,
the (ii) division is used. There are 26 (a-z) shared multisets that receive keys from the a-z
dictionaries with a custom comparator: criterium1: descending by set_size, criterium2: alphabetically
ascending. So, a reducer thread, for a group of letters, will take each letter l (based on (ii)), and
add all elements from the l dictionary to l multiset. There is no need of mutexes, since the intersection
between letter intervals is void. The complexity for this multiset-like sorting is O(N log N), where
N is the number of pairs in the l-dictionary. This was the sorting step after grouping in reduce operation.

Considerations:
- This operation is called reduce, because it combines a list of elements into a single result
(the array of sorted a-z dictionaries), the operation being translated to:
(reduce partial_dicts array_of_sorted_dicts)
- Mutex on a bigger part to avoid lots of context switches and thread starvation, this highly
increasing the scalability

4) Printing data in [a-z].txt files:
This is done in parallel, after sorting in the reducer operation. For every letter ? from
range red_start2, red_end_2, the file "?.txt" is created. The multiset ? is traversed to get the keys,
so that the pairs (string, set<int>) are 2-criterium sorted, and for each key from the multiset, get
the id sets from global_maps[? - ascii_int('a')], where ascii_int('a') = 97.

## Optimizations:
1) dynamical thread work allocation for mappers instead of statically division by the file
dimensions (taken eventually using sys/stats.h in O(1))
2) preferred stack allocation instead of heap
3) built-in data structures like vector, unordered_map, queue
4) parallel file writing using reducers
5) passing arrays of dictionaries or multisets as pointers to avoid unnecessary copy
6) minimize function calls, so where a function is reasonably long, change a function
call with actual code; also inline for print_data to eliminate overhead
7) instead of checking whether strlen(word) > 0, just verify word[0] != '\0'
8) integer operations instead of char ascii code operations
9) unordered_map instead of map for the a-z dictionaries to have O(1) time operations
10) mutexes on medium critical sections to avoid frequent context-switches
avoid function calls

## Testing:

After running with docker 10 times, the average score was 84/84, so maximum score appeared
for scalability and correctness every time.

## Bibliography:
https://static.googleusercontent.com/media/research.google.com/en//archive/mapreduce-osdi04.pdf
