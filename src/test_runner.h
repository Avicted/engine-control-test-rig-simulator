#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "engine.h"

#define MAX_TESTS 10
#define MAX_SCENARIO_NAME_LEN 32

typedef struct
{
    const char *name;
    int (*scenario_func)(EngineState *, int, int, int, void *, unsigned int, unsigned int *);
    int expected_result;
} TestCase;

int run_all_tests(void);
int run_named_scenario(const char *name);
int run_all_tests_with_output(int show_sim);
int run_named_scenario_with_output(const char *name, int show_sim);
int run_all_tests_with_options(int show_sim, int use_color);
int run_named_scenario_with_options(const char *name, int show_sim, int use_color);
int run_all_tests_with_full_options(int show_sim, int use_color, int show_control, int show_state);
int run_named_scenario_with_full_options(const char *name,
                                         int show_sim,
                                         int use_color,
                                         int show_control,
                                         int show_state);
int run_all_tests_with_json(int show_sim, int use_color, int show_control, int show_state, int json_output);
int run_named_scenario_with_json(const char *name,
                                 int show_sim,
                                 int use_color,
                                 int show_control,
                                 int show_state,
                                 int json_output);
int run_scripted_scenario_with_json(const char *script_path,
                                    int show_sim,
                                    int use_color,
                                    int show_control,
                                    int show_state,
                                    int json_output);

#endif
