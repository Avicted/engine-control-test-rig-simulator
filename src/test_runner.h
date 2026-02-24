#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "engine.h"

#define MAX_TESTS 3
#define MAX_SCENARIO_NAME_LEN 32

typedef struct
{
    const char *name;
    int (*scenario_func)(EngineState *);
    int expected_result;
} TestCase;

int run_all_tests(void);
int run_named_scenario(const char *name);

#endif
