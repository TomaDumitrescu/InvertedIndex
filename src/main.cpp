#include "main.h"

// Each thread will have each dictionary to put pairs <string, int>
void *map_op(void *arg)
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

    pthread_barrier_wait(m_arg->barrier);
    return NULL;
}

void *reduce_op(void *arg)
{
    reducer_arg_t *r_arg = (reducer_arg_t *)arg;

    // Waiting the mappers
    pthread_barrier_wait(r_arg->barrier);
    for (int i = r_arg->start; i < r_arg->end; ++i) {
        // RW over the global dictionary by multiple threads
        pthread_mutex_lock(r_arg->lock);
        for (const auto& item: r_arg->local_maps[i]) {
            r_arg->global_maps[(int)(item.first[0]) - 97][item.first].insert(item.second.begin(),
                                                                            item.second.end());
        }
        pthread_mutex_unlock(r_arg->lock);
    }

    // Waiting the grouping
    pthread_barrier_wait(r_arg->a_barrier);

    for (int i = r_arg->start_2; i < r_arg->end_2; ++i) {
        unordered_map<string, set<int>> current_umap = r_arg->global_maps[i];
        unordered_map<string, set<int>>::iterator item = current_umap.begin();
        r_arg->global_multisets[i] = multiset<string, function<bool(const string&, const string&)>>
                ([&current_umap](const string& o, const string& oth) {
                    const auto& oset = current_umap.at(o);
                    const auto& othset = current_umap.at(oth);

                    // Sorting descending by set size
                    if (oset.size() != othset.size()) {
                        return oset.size() > othset.size();
                    }

                    // Sorting alphabetically ascending
                    return o < oth;
                }
            );

        while (item != current_umap.end()) {
            r_arg->global_multisets[i].insert((*item).first);
            ++item;
        }
    }

    return NULL;
}

inline void print_data(unordered_map<string, set<int>> *global_maps,
                       multiset<string, function<bool(const string&, const string&)>> *global_multisets)
{
    char file[6];
    file[1] = '.', file[2] = 't', file[3] = 'x', file[4] = 't', file[5] = '\0';
    for (int i = 0; i < NUM_LETTERS; ++i) {
        file[0] = (char)(i + 97);
        ofstream out(file);

        for (const auto& word: global_multisets[i]) {
            const auto& id_list = global_maps[i][word];
            out << word << ":[";
            for (auto id = id_list.begin(); id != id_list.end(); ++id) {
                if (id != id_list.begin()) {
                    out << " ";
                }
                out << *id;
            }
            out << "]\n";
        }

        out.close();
    }
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

    // Reducers division
    for (id = 0; id < reducers; ++id) {
        red_args[id].start = id * 1.0 * mappers / reducers;
        red_args[id].start_2 = id * 26.0 / reducers;
        red_args[id].end = min<int>((id + 1) * 1.0 * mappers / reducers, mappers);
        red_args[id].end_2 = min<int>((id + 1) * 26.0 / reducers, 26.0);
    }

    pthread_mutex_t lock;
    DIE(pthread_mutex_init(&lock, NULL) != 0, "Mutex initialization failed!\n");

    pthread_barrier_t barrier, a_barrier;
    pthread_barrier_init(&barrier, NULL, mappers + reducers);
    pthread_barrier_init(&a_barrier, NULL, reducers);

    unordered_map<string, set<int>> *thread_maps = new unordered_map<string, set<int>>[mappers]();
    unordered_map<string, set<int>> *global_maps = new unordered_map<string, set<int>>[NUM_LETTERS]();
    multiset<string, function<bool(const string&, const string&)>> *global_multisets =
        new multiset<string, function<bool(const string&, const string&)>>[NUM_LETTERS]();
    for (id = 0; id < mappers + reducers; ++id) {
        if (id < mappers) {
            map_args[id].thread_dict = &thread_maps[id], map_args[id].tasks = &tasks;
            map_args[id].files = files, map_args[id].lock = &lock, map_args[id].barrier = &barrier;
            DIE(pthread_create(&threads[id], NULL, map_op, (void *) &map_args[id]), "pth_create");
        } else {
            red_args[id - mappers].barrier = &barrier, red_args[id - mappers].lock = &lock;
            red_args[id - mappers].local_maps = thread_maps, red_args[id - mappers].a_barrier = &a_barrier;
            red_args[id - mappers].global_maps = global_maps;
            red_args[id - mappers].global_multisets = global_multisets;
            DIE(pthread_create(&threads[id], NULL, reduce_op, (void *) &red_args[id - mappers]), "pth_create");
        }
    }

    // Waiting the threads
    for (id = 0; id < mappers + reducers; ++id) {
        DIE(pthread_join(threads[id], &status), "pthread_join");
    }

    // Sequentially printing the data
    print_data(global_maps, global_multisets);

    // Cleaning dynamical allocated resources
    fin.close();
    delete files;
    delete[] thread_maps;
    delete[] global_maps;
    delete[] global_multisets;
    pthread_barrier_destroy(&barrier);
    pthread_barrier_destroy(&a_barrier);
    pthread_mutex_destroy(&lock);
    return 0;
}
