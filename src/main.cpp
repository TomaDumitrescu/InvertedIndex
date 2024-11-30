#include "main.h"

using namespace std;

// Deallocates resource
inline void clean(ifstream &fin, vector<string> *files)
{
    fin.close();
    delete files;
}

void *map(void *arg)
{
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

    int num_files, total_dim = 0;
    fin >> num_files;

    vector<int> fdim(num_files, 0);
    vector<string> *files = new vector<string>;
    char buffer[FILE_LEN] = {0};
    fin.getline(buffer, FILE_LEN);
    for (int i = 0; i < num_files; ++i) {
        fin.getline(buffer, FILE_LEN);
        files->push_back(string(buffer));

        struct stat st;
        stat(buffer, &st);
        fdim[i] = st.st_size;
        total_dim += fdim[i];
    }

    // Creating threads
    int mappers = atoi(argv[1]), reducers = atoi(argv[2]), id;
    pthread_t threads[mappers + reducers];
    void *status;
    mapper_arg_t map_args[mappers];
    reducer_arg_t red_args[reducers];

    for (id = 0; id < mappers + reducers; ++id) {
        if (id < mappers) {
            DIE(pthread_create(&threads[id], NULL, map, (void *) &map_args[id]), "pth_create");
        } else {
            DIE(pthread_create(&threads[id], NULL, reduce, (void *) &red_args[id - mappers]), "pth_create");
        }
    }

    // Waiting the threads
    for (id = 0; id < mappers + reducers; ++id) {
        DIE(pthread_join(threads[id], &status), "pthread_join");
    }

    clean(fin, files);
    return 0;
}
