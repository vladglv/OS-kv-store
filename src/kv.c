#include "kv.h"

store_t* gp_Store = NULL;
int g_fd = -1;
int cache_valid = MY_FALSE;
size_t cache_idx = 0;
char* cache_data[BUCKET_SIZE + 1] = {NULL};
char* cache_key = NULL;

int sm_create(int* fd, char* name, int flags, mode_t perms, int clear) {
    if(fd == NULL || name == NULL)
        return MY_ERR_ARG;

    *fd = shm_open(name, flags, perms);
    if(*fd == -1)
        return MY_ERR;

    if(clear == MY_TRUE) {
        int r = ftruncate(*fd, sizeof(store_t));
        if(r == -1)
            return MY_ERR;
    }

    return MY_OK;
}

int sm_exists(char* name) {
    int fd;
    int r = sm_create(&fd, name, O_RDWR, S_IRWXU, MY_FALSE);
    if(r == MY_OK)
        assert(close(fd) == 0);
    return r;
}

int sm_attach(int fd, store_t** shm) {
    if(shm == NULL)
        return MY_ERR_ARG;

    *shm = (store_t*)mmap(
        NULL, sizeof(store_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(*shm == MAP_FAILED) {
        *shm = NULL;
        return MY_ERR;
    }

    return MY_OK;
}

int sm_detach(store_t** shm) {
    if(shm == NULL)
        return MY_ERR_ARG;

    int r = munmap(*shm, sizeof(store_t));
    if(r == -1)
        return MY_ERR;

    *shm = NULL;

    return MY_OK;
}

int sm_close(char* name) {
    if(name == NULL)
        return MY_ERR_ARG;

    assert(close(g_fd) == 0);

    int r = shm_unlink(name);
    if(r == -1)
        return MY_ERR;

    return MY_OK;
}

int kv_store_create(char* name) {
    if(gp_Store != NULL)
        return 0;

    int r = sm_exists(name);
    int flag = O_CREAT | O_RDWR;
    int clear = MY_TRUE;

    if(r == MY_OK) {
        flag = O_RDWR;
        clear = MY_FALSE;
    }

    r = sm_create(&g_fd, name, flag, S_IRWXU, clear);
    if(r != MY_OK)
        return -1;

    r = sm_attach(g_fd, &gp_Store);
    if(r != MY_OK)
        return -1;

    if(clear == MY_TRUE) {
        assert(sem_init(&gp_Store->protect, 1, 1) == 0);

        memset(gp_Store->name, 0, sizeof(gp_Store->name));
        strncpy(gp_Store->name, name, sizeof(gp_Store->name));
        gp_Store->name[sizeof(gp_Store->name) - 1] = '\0';
        gp_Store->clients = 0;

        for(size_t i = 0; i < BUCKET_COUNT; i++) {
            bucket_t* p = &gp_Store->buckets[i];

            p->cache_valid = MY_FALSE;

            cache_idx = 0;
            cache_valid = MY_FALSE;
            cache_key = NULL;
            memset(cache_data, 0, sizeof(cache_data));

            assert(sem_init(&p->protect, 1, 1) == 0);
            memset(p->keys, 0, sizeof(p->keys));
            memset(p->values, 0, sizeof(p->values));
        }
    }

    assert(sem_wait(&gp_Store->protect) == 0);

    // - Sanity check
    struct stat s;
    r = fstat(g_fd, &s);
    if(r == 0)
        assert(s.st_size == sizeof(store_t));

    gp_Store->clients++;
    assert(sem_post(&gp_Store->protect) == 0);

    return 0;
}

void kv_store_print(void) {
    if(gp_Store == NULL)
        return;

    assert(sem_wait(&gp_Store->protect) == 0);

    for(size_t i = 0; i < BUCKET_COUNT; i++) {
        for(size_t j = 0; j < BUCKET_SIZE; j++) {
            bucket_t* p = &gp_Store->buckets[i];
            assert(sem_wait(&p->protect) == 0);
            printf("Bucket %zu | Element %zu | Key='%s' | Value='%s'\n",
                   i,
                   j,
                   p->keys[j],
                   p->values[j]);
            assert(sem_post(&p->protect) == 0);
        }
    }

    assert(sem_post(&gp_Store->protect) == 0);
}

int kv_delete_db(void) {
    if(gp_Store == NULL)
        return -1;

    assert(sem_wait(&gp_Store->protect) == 0);

    char n[sizeof(gp_Store->name)];
    strncpy(n, gp_Store->name, sizeof(gp_Store->name));

    assert(sem_post(&gp_Store->protect) == 0);

    return kv_store_destroy(n);
}

int kv_store_destroy(char* name) {
    if(gp_Store == NULL || name == NULL)
        return -1;

    cache_idx = 0;
    cache_valid = MY_FALSE;
    cache_key = NULL;
    memset(cache_data, 0, sizeof(cache_data));

    assert(sem_wait(&gp_Store->protect) == 0);
    if(strncmp(gp_Store->name, name, sizeof(gp_Store->name) - 1) != 0) {
        assert(sem_post(&gp_Store->protect) == 0);
        return -1;
    }

    int cl = --gp_Store->clients;
    assert(sem_post(&gp_Store->protect) == 0);

    if(cl < 1) {
        for(size_t i = 0; i < BUCKET_COUNT; i++)
            assert(sem_destroy(&gp_Store->buckets[i].protect) == 0);
        assert(sem_destroy(&gp_Store->protect) == 0);
    }

    int r = sm_detach(&gp_Store);
    if(r != MY_OK)
        return -1;

    gp_Store = NULL;

    if(cl < 1) {
        r = sm_close(name);
        if(r != MY_OK)
            return -1;
    }

    return 0;
}

int kv_store_write(char* key, char* value) {
    if(key == NULL || value == NULL || gp_Store == NULL || strlen(key) < 1)
        return -1;

    int i = hash_func(key, BUCKET_COUNT);
    bucket_t* p = &gp_Store->buckets[i];

    assert(sem_wait(&p->protect) == 0);

    p->cache_valid = MY_FALSE;
    cache_idx = 0;
    cache_key = NULL;
    cache_valid = MY_FALSE;
    for(size_t j = 0; j < BUCKET_SIZE + 1; j++)
        cache_data[j] = NULL;

    char keys[BUCKET_SIZE][KEY_COUNT];
    char values[BUCKET_SIZE][KEY_SIZE];

    for(size_t k = 0; k < BUCKET_SIZE; k++) {
        strncpy(keys[k], p->keys[k], KEY_COUNT);
        strncpy(values[k], p->values[k], KEY_SIZE);
    }

    for(size_t k = 0; k < BUCKET_SIZE - 1; k++) {
        strncpy(p->keys[k + 1], keys[k], KEY_COUNT);
        strncpy(p->values[k + 1], values[k], KEY_SIZE);
    }

    strncpy(p->keys[0], key, KEY_COUNT);
    strncpy(p->values[0], value, KEY_SIZE);

    p->keys[0][KEY_COUNT - 1] = '\0';
    p->values[0][KEY_SIZE - 1] = '\0';

    assert(sem_post(&p->protect) == 0);

    return 0;
}

char* kv_store_read(char* key) {
    if(key == NULL || gp_Store == NULL)
        return NULL;

    int k = hash_func(key, BUCKET_COUNT);
    char* r = NULL;
    bucket_t* p = &gp_Store->buckets[k];

    assert(sem_wait(&p->protect) == 0);

    if(p->cache_valid == MY_TRUE && cache_valid == MY_TRUE &&
       strncmp(cache_key, key, KEY_COUNT) == 0) {
        if(cache_data[cache_idx] == NULL) {
            r = NULL;
            cache_idx = 0;
        } else {
            r = strndup(cache_data[cache_idx++], KEY_SIZE);
        }
    } else {
        p->cache_valid = MY_TRUE;
        cache_idx = 0;
        cache_valid = MY_TRUE;
        size_t l = 0;

        for(size_t i = 0; i < BUCKET_SIZE; i++) {
            if(strncmp(p->keys[BUCKET_SIZE - i - 1], key, KEY_COUNT) == 0) {
                cache_key = p->keys[BUCKET_SIZE - i - 1];
                cache_data[l++] = p->values[BUCKET_SIZE - i - 1];
            }
        }

        cache_data[l] = NULL;

        if(l > 0)
            r = strndup(cache_data[cache_idx++], KEY_SIZE);
    }

    assert(sem_post(&p->protect) == 0);

    return r;
}

char** kv_store_read_all(char* key) {
    if(key == NULL || gp_Store == NULL)
        return NULL;

    int k = hash_func(key, BUCKET_COUNT);
    char** values = (char**)calloc(BUCKET_SIZE + 1, sizeof(char*));
    values[BUCKET_SIZE] = NULL;

    size_t r = 0;
    bucket_t* p = &gp_Store->buckets[k];

    assert(sem_wait(&p->protect) == 0);

    for(size_t i = 0; i < BUCKET_SIZE; i++) {
        if(strncmp(p->keys[BUCKET_SIZE - i - 1], key, KEY_COUNT) == 0) {
            values[r++] = strndup(p->values[BUCKET_SIZE - i - 1], KEY_SIZE);
        }
    }

    values[r] = NULL;

    if(values[0] == NULL) {
        assert(sem_post(&p->protect) == 0);
        free(values);
        return NULL;
    }

    assert(sem_post(&p->protect) == 0);

    return values;
}

size_t hash_func(char* word, int32_t max) {
    int32_t hashAddress = 5381;
    for(size_t counter = 0; word[counter] != '\0'; counter++) {
        hashAddress = hashAddress % (1 << 24);
        hashAddress = ((hashAddress << 5) + hashAddress) + word[counter];
    }
    return ((hashAddress % max) < 0) ? -hashAddress % max : hashAddress % max;
}

void _intHandler(int dummy) {
    UNUSED(dummy);
    kv_delete_db();
    exit(0);
}

#ifdef STANDALONE
int main(int argc, char** argv) {
#else
int _main(int argc, char** argv) {
#endif
    UNUSED(argc);
    UNUSED(argv);

    // Handlers
    signal(SIGINT, _intHandler);
    signal(SIGQUIT, _intHandler);
    signal(SIGTSTP, _intHandler);

    printf("Test 1\n");

    // - Check create
    int r = kv_store_create("/STORE");

    //- Check write
    r = kv_store_write("Yes!", "You can do this 2B!");
    UNUSED(r);

    r = kv_store_write("Yes!", "You can do this 9S!");

    for(size_t k = 0; k < 17; k++) {
        char str[5];
        sprintf(str, "%zu", k);
        r = kv_store_write("Yes!", str);
    }

    // - Check read
    char* s = kv_store_read("Yes!");

    printf("%s for key 'Yes!'\n", s);
    free(s);

    printf("Test 2\n");

    for(int i = 0;; i++) {
        char* p = kv_store_read("Yes!");

        if(p == NULL) {
            free(p);
            break;
        }

        printf("%d => '%s'\n", i, p);

        free(p);
    }

    printf("\nTest 3\n");

    // - Check read_all
    char** v = kv_store_read_all("Yes!");
    for(size_t i = 0; v[i] != NULL; i++) {
        printf("%zu => '%s'\n", i, v[i]);
        free(v[i]);
    }

    free(v);

    // - Check destroy
    kv_store_destroy("/STORE");

    return 0;
}
