#ifndef SCENARIO_REPORT_H
#define SCENARIO_REPORT_H

#include <stdint.h>

#include "scenario/scenario_profiles.h"
#include "status.h"

const char *scenario_report_result_to_string(int32_t result);
const char *scenario_report_mode_to_string(EngineStateMode mode);
const char *scenario_report_result_to_display_string(int32_t result, int32_t use_color);
const char *scenario_report_pass_fail_display(int32_t is_pass, int32_t use_color);

int32_t scenario_report_print_tick_details(uint32_t tick,
                                           const EngineState *engine,
                                           int32_t result,
                                           int32_t show_sim,
                                           int32_t show_control,
                                           int32_t show_state);

/*
 * @requirement REQ-ENG-JSON-001
 * @pre none
 * @post writes deterministic top-level JSON metadata and opens scenarios array
 * @deterministic yes
 */
StatusCode scenario_report_print_json_header(void);

/*
 * @requirement REQ-ENG-DIAG-002
 * @pre error_info may be NULL
 * @post writes structured JSON error object when error_info->code != STATUS_OK
 * @deterministic yes
 */
StatusCode scenario_report_print_json_error(const ErrorInfo *error_info);

/*
 * @requirement REQ-ENG-JSON-002
 * @pre passed >= 0, total >= 0
 * @post closes scenarios array and writes optional error + summary
 * @deterministic yes
 */
StatusCode scenario_report_print_json_footer(int32_t passed, int32_t total, const ErrorInfo *error_info);
StatusCode scenario_report_print_json_scenario_object(const char *scenario_name,
                                                      const char *requirement_id,
                                                      const TickReport *tick_reports,
                                                      uint32_t tick_count,
                                                      int32_t expected_result,
                                                      int32_t actual_result,
                                                      int32_t has_expected);

#endif
