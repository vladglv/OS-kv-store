#ifndef __COMP310_ASSIGNMENT2_TEST_CASE_READ_EVAL__
#define __COMP310_ASSIGNMENT2_TEST_CASE_READ_EVAL__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

// - Macro to inhibit unused variable warnings
#define UNUSED(expr)  \
    do {              \
        (void)(expr); \
    } while(0)

void read_eval();

#endif
