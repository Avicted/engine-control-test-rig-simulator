#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdint.h>

#include "scenario_contract.h"
#include "status.h"

/*
 * @requirement REQ-ENG-TEST-001
 * @pre none
 * @post executes full requirement-mapped scenario registry and returns STATUS_OK only on full pass
 * @deterministic yes
 */
StatusCode run_all_tests(void);

/*
 * @requirement REQ-ENG-TEST-002
 * @pre name != NULL and references a supported named scenario token
 * @post scenario executes once
 * @deterministic yes
 */
StatusCode run_named_scenario(const char *name);

/*
 * @requirement REQ-ENG-TEST-001
 * @pre show_sim is 0/1
 * @post complete registry executed with optional tick simulation display
 * @deterministic yes
 */
StatusCode run_all_tests_with_output(int32_t show_sim);

/*
 * @requirement REQ-ENG-TEST-002
 * @pre name valid, show_sim is 0/1
 * @post selected scenario executed with optional simulation display
 * @deterministic yes
 */
StatusCode run_named_scenario_with_output(const char *name, int32_t show_sim);

/*
 * @requirement REQ-ENG-TEST-001
 * @pre show_sim/use_color are 0/1
 * @post full registry executed with requested display options
 * @deterministic yes
 */
StatusCode run_all_tests_with_options(int32_t show_sim, int32_t use_color);

/*
 * @requirement REQ-ENG-TEST-002
 * @pre name valid, show_sim/use_color are 0/1
 * @post selected scenario executed with requested options
 * @deterministic yes
 */
StatusCode run_named_scenario_with_options(const char *name, int32_t show_sim, int32_t use_color);

/*
 * @requirement REQ-ENG-TEST-001
 * @pre option flags are 0/1
 * @post full registry executed and deterministic summary returned
 * @deterministic yes
 */
StatusCode run_all_tests_with_full_options(int32_t show_sim, int32_t use_color, int32_t show_control, int32_t show_state);

/*
 * @requirement REQ-ENG-TEST-002
 * @pre name valid, option flags are 0/1
 * @post selected scenario executed and logged deterministically
 * @deterministic yes
 */
StatusCode run_named_scenario_with_full_options(const char *name,
                                                int32_t show_sim,
                                                int32_t use_color,
                                                int32_t show_control,
                                                int32_t show_state);

/*
 * @requirement REQ-ENG-JSON-001
 * @pre option flags are 0/1
 * @post JSON includes schema_version, software_version, build_commit, requirement_id, and summary
 * @deterministic yes
 */
StatusCode run_all_tests_with_json(int32_t show_sim,
                                   int32_t use_color,
                                   int32_t show_control,
                                   int32_t show_state,
                                   int32_t json_output);

/*
 * @requirement REQ-ENG-JSON-001
 * @pre name valid, option flags are 0/1
 * @post single-scenario JSON output includes mapped requirement_id
 * @deterministic yes
 */
StatusCode run_named_scenario_with_json(const char *name,
                                        int32_t show_sim,
                                        int32_t use_color,
                                        int32_t show_control,
                                        int32_t show_state,
                                        int32_t json_output);

/*
 * @requirement REQ-ENG-SCRIPT-001
 * @pre script_path valid and parseable; option flags are 0/1
 * @post parsed scripted scenario executes deterministically
 * @deterministic yes
 */
StatusCode run_scripted_scenario_with_json(const char *script_path,
                                           int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state,
                                           int32_t json_output,
                                           int32_t strict_mode);

#endif
