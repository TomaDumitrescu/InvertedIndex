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

struct mapper_arg_t {
    unordered_map<string, set<int>> *thread_dict;
    queue<int> *tasks;
    vector<string> *files;
    pthread_mutex_t *lock;
    pthread_barrier_t *barrier;
};

struct reducer_arg_t {

};

#endif
