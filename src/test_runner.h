#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "engine.h"

#define MAX_TESTS 6
#define MAX_SCENARIO_NAME_LEN 32

typedef struct
{
    const char *name;
    int (*scenario_func)(EngineState *);
    int expected_result;
} TestCase;

int run_all_tests(void);
int run_named_scenario(const char *name);
int run_all_tests_with_output(int show_sim);
int run_named_scenario_with_output(const char *name, int show_sim);
int run_all_tests_with_options(int show_sim, int use_color);
int run_named_scenario_with_options(const char *name, int show_sim, int use_color);
int run_all_tests_with_full_options(int show_sim, int use_color, int show_control);
int run_named_scenario_with_full_options(const char *name, int show_sim, int use_color, int show_control);

#endif
