/* Written by Xiru Zhu
 * This is for testing your assignment 2.
 */
#ifndef __A2_TEST__
#define __A2_TEST__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include "../kv.h"

// LEAVE MAX KEYS as twice the number of pods
#define __TEST_MAX_KEY__ 512
#define __TEST_MAX_KEY_SIZE__ 16
#define __TEST_MAX_DATA_LENGTH__ 32
#define __TEST_MAX_POD_ENTRY__ 256
#define __TEST_SHARED_MEM_NAME__ "/GTX_1080_TI"
#define __TEST_SHARED_SEM_NAME__ "/ONLY_TEARS"
#define __TEST_FORK_NUM__ 4
#define RUN_ITERATIONS 1020

sem_t* open_sem_lock;
pid_t pids[__TEST_FORK_NUM__];

void kill_shared_mem();
void intHandler(int dummy);
void generate_string(char buf[], int length);
void generate_key(char buf[], int length, char** keys_buf, int num_keys);

void generate_string(char buf[], int length) {
    int type;
    for(int i = 0; i < length; i++) {
        type = rand() % 3;
        if(type == 2)
            buf[i] = rand() % 26 + 'a';
        else if(type == 1)
            buf[i] = rand() % 10 + '0';
        else
            buf[i] = rand() % 26 + 'A';
    }
    buf[length - 1] = '\0';
}

void
generate_unique_data(char buf[], int length, char** keys_buf, int num_keys) {
    generate_string(buf, __TEST_MAX_DATA_LENGTH__);
    int counter = 0;
    for(int i = 0; i < num_keys; i++) {
        if(strcmp(keys_buf[i], buf) == 0) {
            counter++;
        }
    }
    if(counter > 1) {
        generate_unique_data(buf, length, keys_buf, num_keys);
    }
    return;
}

void generate_key(char buf[], int length, char** keys_buf, int num_keys) {
    generate_string(buf, __TEST_MAX_KEY_SIZE__);
    int counter = 0;
    for(int i = 0; i < num_keys; i++) {
        if(strcmp(keys_buf[i], buf) == 0) {
            counter++;
        }
    }
    if(counter > 1) {
        generate_key(buf, length, keys_buf, num_keys);
    }
    return;
}

#endif
