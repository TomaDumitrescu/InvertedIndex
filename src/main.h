#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <pthread.h>
#include <queue>
#include <unordered_map>
#include <errno.h>
#include <set>
#include <unordered_map>
#include <functional>

using namespace std;


#define DIE(assertion, call_description)            \
    do                                              \
    {                                               \
        if (assertion)                              \
        {                                           \
            fprintf(stderr, "(%s, %d): ", __FILE__, \
                    __LINE__);                      \
            perror(call_description);               \
            exit(errno);                            \
        }                                           \
    } while (0)

#define FILE_LEN 80
#define WORD_LEN 50
#define NUM_LETTERS 26

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct mapper_arg_t {
    unordered_map<string, set<int>> *thread_dict;
    queue<int> *tasks;
    vector<string> *files;
    pthread_mutex_t *lock;
    pthread_barrier_t *barrier;
};

struct reducer_arg_t {
    unordered_map<string, set<int>> *global_maps;
    unordered_map<string, set<int>> *local_maps;
    multiset<string, function<bool(const string&, const string&)>> *global_multisets;
    pthread_mutex_t *lock;
    pthread_barrier_t *barrier;
    pthread_barrier_t *a_barrier;
    int start, start_2;
    int end, end_2;
};

#endif
