#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "engine.h"
#include "status.h"

#define MAX_TESTS 10
#define MAX_SCENARIO_NAME_LEN 32

typedef struct
{
    const char *test_name;
    const char *requirement_id;
    int (*scenario_func)(EngineState *, int, int, int, void *, unsigned int, unsigned int *);
    int expected_result;
} TestCase;

StatusCode run_all_tests(void);
StatusCode run_named_scenario(const char *name);
StatusCode run_all_tests_with_output(int show_sim);
StatusCode run_named_scenario_with_output(const char *name, int show_sim);
StatusCode run_all_tests_with_options(int show_sim, int use_color);
StatusCode run_named_scenario_with_options(const char *name, int show_sim, int use_color);
StatusCode run_all_tests_with_full_options(int show_sim, int use_color, int show_control, int show_state);
StatusCode run_named_scenario_with_full_options(const char *name,
                                                int show_sim,
                                                int use_color,
                                                int show_control,
                                                int show_state);
StatusCode run_all_tests_with_json(int show_sim, int use_color, int show_control, int show_state, int json_output);
StatusCode run_named_scenario_with_json(const char *name,
                                        int show_sim,
                                        int use_color,
                                        int show_control,
                                        int show_state,
                                        int json_output);
StatusCode run_scripted_scenario_with_json(const char *script_path,
                                           int show_sim,
                                           int use_color,
                                           int show_control,
                                           int show_state,
                                           int json_output,
                                           int strict_mode);

#endif
