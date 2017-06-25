#ifndef __COMP310_ASSIGNMENT2_TEST_CASE_CONFIG__
#define __COMP310_ASSIGNMENT2_TEST_CASE_CONFIG__

#include <stdlib.h>

#define DATA_BASE_NAME "database"
#define KEY_MAX_LENGTH 32
#define VALUE_MAX_LENGTH 256

/* if you use the default interface uncomment below */
extern int kv_store_create(const char* name);
extern int kv_store_write(const char* key, const char* value);
extern char* kv_store_read(const char* key);
extern char** kv_store_read_all(const char* key);

/* if you write your own interface, please fill the following adaptor */
// int    (*kv_store_create)(const char*)             = NULL;
// int    (*kv_store_write)(const char*, const char*) = NULL;
// char*  (*kv_store_read)(const char*)               = NULL;
// char** (*kv_store_read_all)(const char* key)       = NULL;

#endif
