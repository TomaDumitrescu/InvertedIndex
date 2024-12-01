#include "main.h"

// Deallocates resource
inline void clean(ifstream &fin, vector<string> *files, unordered_map<string, set<int>> *thread_maps)
{
    fin.close();
    delete files;
    delete[] thread_maps;
}

// Each thread will have each dictionary to put pairs <string, int>
void *map(void *arg)
{
    mapper_arg_t *m_arg = (mapper_arg_t *)arg;
    int fid = -1, len, cnt;
    char buffer[WORD_LEN] = {0}, word[WORD_LEN] = {0};
    while (true) {
        // Avoid RW race conditions
        pthread_mutex_lock(m_arg->lock);
            if (m_arg->tasks->size() == 0) {
                fid = -1;
            } else {
                fid = m_arg->tasks->front();
                m_arg->tasks->pop();
            }
        pthread_mutex_unlock(m_arg->lock);

        if (fid == -1) {
            break;
        }

        // Process the file
        ifstream in((*m_arg->files)[fid].c_str());

        while (in >> buffer) {
            len = strlen(buffer), cnt = 0;
            // Standardizing the word
            for (int i = 0; i < len; i++) {
                if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                    word[cnt++] = buffer[i];
                } else if (buffer[i] >= 'A' && buffer[i] <= 'Z') {
                    word[cnt++] = (char)(buffer[i] - 'A' + 'a');
                }
            }

            word[cnt] = '\0';

            if (word[0] == '\0') {
                continue;
            }

            (*m_arg->thread_dict)[string(word)].insert(fid + 1);
            memset(buffer, '\0', len + 1);
        }

        in.close();
    }

    // Useful barrier for the reducers
    //pthread_barrier_wait(m_arg->barrier);
    return NULL;
}

void *reduce(void *arg)
{
    return NULL;
}

int main(int argc, char **argv)
{
    // Loading data
    ifstream fin(argv[3]);

    int num_files;
    fin >> num_files;

    vector<string> *files = new vector<string>;
    queue<int> tasks;
    char buffer[FILE_LEN] = {0};
    fin.getline(buffer, FILE_LEN);
    for (int i = 0; i < num_files; ++i) {
        fin.getline(buffer, FILE_LEN);
        files->push_back(string(buffer));
        tasks.push(i);
    }

    // Creating threads
    int mappers = atoi(argv[1]), reducers = atoi(argv[2]), id;
    pthread_t threads[mappers + reducers];
    void *status;
    mapper_arg_t map_args[mappers];
    reducer_arg_t red_args[reducers];

    pthread_mutex_t lock;
    DIE(pthread_mutex_init(&lock, NULL) != 0, "Mutex initialization failed!\n");

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, mappers + reducers);

    unordered_map<string, set<int>> *thread_maps = new unordered_map<string, set<int>>[mappers]();
    for (id = 0; id < mappers + reducers; ++id) {
        if (id < mappers) {
            map_args[id].thread_dict = &thread_maps[id], map_args[id].tasks = &tasks;
            map_args[id].files = files, map_args[id].lock = &lock;
            map_args[id].barrier = &barrier;
            DIE(pthread_create(&threads[id], NULL, map, (void *) &map_args[id]), "pth_create");
        } else {
            DIE(pthread_create(&threads[id], NULL, reduce, (void *) &red_args[id - mappers]), "pth_create");
        }
    }

    // Waiting the threads
    for (id = 0; id < mappers + reducers; ++id) {
        DIE(pthread_join(threads[id], &status), "pthread_join");
    }

    // Cleaning dynamical allocated resources
    clean(fin, files, thread_maps);
    return 0;
}
