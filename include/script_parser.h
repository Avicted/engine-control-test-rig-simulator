#ifndef SCRIPT_PARSER_H
#define SCRIPT_PARSER_H

#include <stdint.h>

#include "hal.h"
#include "status.h"

#define SCRIPT_PARSER_MAX_TICKS 64U

typedef struct
{
    uint32_t tick_values[SCRIPT_PARSER_MAX_TICKS];
    HAL_Frame sensor_frames[SCRIPT_PARSER_MAX_TICKS];
    uint32_t tick_count;
    uint32_t parse_warning_count;
} ScriptScenarioData;

_Static_assert(SCRIPT_PARSER_MAX_TICKS > 0U, "SCRIPT_PARSER_MAX_TICKS must be non-zero");
_Static_assert(SCRIPT_PARSER_MAX_TICKS <= 1024U, "SCRIPT_PARSER_MAX_TICKS exceeds validated bound");
_Static_assert(sizeof(ScriptScenarioData) <= 4096U, "ScriptScenarioData footprint exceeds expected bound");

/*
 * @requirement REQ-ENG-SCRIPT-001
 * @pre script_path != NULL, scenario_data != NULL, error_message != NULL, error_message_size > 0
 * @post scenario_data is populated with validated monotonic ticks and HAL transport frames
 * @deterministic yes
 */
StatusCode script_parser_parse_file(const char *script_path,
                                    ScriptScenarioData *scenario_data,
                                    char *error_message,
                                    uint32_t error_message_size,
                                    int32_t strict_mode);

#endif
