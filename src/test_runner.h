#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdint.h>

#include "engine.h"
#include "status.h"

#define MAX_TESTS 10
#define MAX_SCENARIO_NAME_LEN 32

_Static_assert(MAX_TESTS > 0, "MAX_TESTS must be non-zero");
_Static_assert(MAX_TESTS <= 64, "MAX_TESTS exceeds validated bound");
_Static_assert(MAX_SCENARIO_NAME_LEN >= 16, "MAX_SCENARIO_NAME_LEN too small for supported scenario names");

typedef struct
{
    const char *test_name;
    const char *requirement_id;
    /* Scenario callback contract: deterministic, no dynamic allocation, bounded ticks. */
    int32_t (*scenario_func)(EngineState *, int32_t, int32_t, int32_t, void *, uint32_t, uint32_t *);
    int32_t expected_result;
} TestCase;

/* Executes full requirement-mapped scenario registry and returns STATUS_OK only on full pass. */
StatusCode run_all_tests(void);

/* PRE: name != NULL and references a supported named scenario token. POST: scenario executes once. */
StatusCode run_named_scenario(const char *name);

/* PRE: show_sim is 0/1. POST: complete registry executed with optional tick simulation display. */
StatusCode run_all_tests_with_output(int32_t show_sim);

/* PRE: name valid, show_sim is 0/1. POST: selected scenario executed with optional simulation display. */
StatusCode run_named_scenario_with_output(const char *name, int32_t show_sim);

/* PRE: show_sim/use_color are 0/1. POST: full registry executed with requested display options. */
StatusCode run_all_tests_with_options(int32_t show_sim, int32_t use_color);

/* PRE: name valid, show_sim/use_color are 0/1. POST: selected scenario executed with requested options. */
StatusCode run_named_scenario_with_options(const char *name, int32_t show_sim, int32_t use_color);

/* PRE: option flags are 0/1. POST: full registry executed and deterministic summary returned. */
StatusCode run_all_tests_with_full_options(int32_t show_sim, int32_t use_color, int32_t show_control, int32_t show_state);

/* PRE: name valid, option flags are 0/1. POST: selected scenario executed and logged deterministically. */
StatusCode run_named_scenario_with_full_options(const char *name,
                                                int32_t show_sim,
                                                int32_t use_color,
                                                int32_t show_control,
                                                int32_t show_state);

/* PRE: option flags are 0/1. POST: JSON includes schema_version, software_version, requirement_id, and summary. */
StatusCode run_all_tests_with_json(int32_t show_sim,
                                   int32_t use_color,
                                   int32_t show_control,
                                   int32_t show_state,
                                   int32_t json_output);

/* PRE: name valid, option flags are 0/1. POST: single-scenario JSON output includes mapped requirement_id. */
StatusCode run_named_scenario_with_json(const char *name,
                                        int32_t show_sim,
                                        int32_t use_color,
                                        int32_t show_control,
                                        int32_t show_state,
                                        int32_t json_output);

/* PRE: script_path valid and parseable; option flags are 0/1. POST: parsed scripted scenario executes deterministically. */
StatusCode run_scripted_scenario_with_json(const char *script_path,
                                           int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state,
                                           int32_t json_output,
                                           int32_t strict_mode);

#endif
