#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "hal.h"
#include "reporting/logger.h"
#include "reporting/output.h"
#include "scenario/requirements.h"
#include "scenario/scenario_catalog.h"
#include "scenario/scenario_profiles.h"
#include "scenario/scenario_report.h"
#include "script_parser.h"
#include "test_runner.h"

#define TEST_LINE_BUFFER_SIZE 256
#define MAX_PROFILE_TICKS 26U

static int32_t print_test_separator(void)
{
    return output_write_line("------------------------------------------------------------\n");
}

static int32_t run_test_case(const TestCase *test_case,
                             int32_t show_sim,
                             int32_t use_color,
                             int32_t show_control,
                             int32_t show_state,
                             int32_t json_output,
                             int32_t json_prefix_comma)
{
    EngineState engine;
    int32_t actual_result;
    StatusCode status;
    TickReport tick_reports[MAX_PROFILE_TICKS];
    uint32_t tick_report_count = 0U;
    int32_t local_show_sim;
    int32_t local_show_control;
    int32_t local_show_state;

    if ((test_case == (const TestCase *)0) || (test_case->test_name == (const char *)0) ||
        (test_case->requirement_id == (const char *)0) ||
        (test_case->scenario_func == (int32_t (*)(EngineState *, int32_t, int32_t, int32_t, void *, uint32_t, uint32_t *))0))
    {
        return 0;
    }

    status = engine_reset(&engine);
    if (status != STATUS_OK)
    {
        return 0;
    }

    if (json_output == 0)
    {
        if (print_test_separator() != ENGINE_OK)
        {
            return 0;
        }
    }

    local_show_sim = (json_output != 0) ? 0 : show_sim;
    local_show_control = (json_output != 0) ? 0 : show_control;
    local_show_state = (json_output != 0) ? 0 : show_state;

    actual_result = test_case->scenario_func(&engine,
                                             local_show_sim,
                                             local_show_control,
                                             local_show_state,
                                             tick_reports,
                                             MAX_PROFILE_TICKS,
                                             &tick_report_count);
    if (actual_result == ENGINE_ERROR)
    {
        return 0;
    }

    if (json_output != 0)
    {
        if (json_prefix_comma != 0)
        {
            if (output_write_line(",\n") != ENGINE_OK)
            {
                return 0;
            }
        }

        if (scenario_report_print_json_scenario_object(test_case->test_name,
                                                       test_case->requirement_id,
                                                       tick_reports,
                                                       tick_report_count,
                                                       test_case->expected_result,
                                                       actual_result,
                                                       1) != STATUS_OK)
        {
            return 0;
        }
    }
    else
    {
        char line[TEST_LINE_BUFFER_SIZE];
        int32_t written;

        written = snprintf(line,
                           sizeof(line),
                           "%s | %s | expected=%s | actual=%s | %s\n",
                           test_case->requirement_id,
                           test_case->test_name,
                           scenario_report_result_to_display_string(test_case->expected_result, use_color),
                           scenario_report_result_to_display_string(actual_result, use_color),
                           scenario_report_pass_fail_display(actual_result == test_case->expected_result, use_color));
        if ((written < 0) || (written >= (int32_t)sizeof(line)))
        {
            return 0;
        }
        if (output_write_line(line) != ENGINE_OK)
        {
            return 0;
        }
    }

    return (actual_result == test_case->expected_result) ? 1 : 0;
}

static int32_t valid_scenario_name(const char *name)
{
    size_t name_len;

    if (name == (const char *)0)
    {
        return 0;
    }

    name_len = strlen(name);
    if ((name_len == 0U) || (name_len >= (size_t)MAX_SCENARIO_NAME_LEN))
    {
        return 0;
    }

    return 1;
}

static int32_t validate_hal_negative_cases(void)
{
    if (hal_read_sensors(0U, (HAL_SensorFrame *)0) != STATUS_INVALID_ARGUMENT)
    {
        return ENGINE_ERROR;
    }

    return ENGINE_OK;
}

StatusCode run_all_tests_with_json(int32_t show_sim,
                                   int32_t use_color,
                                   int32_t show_control,
                                   int32_t show_state,
                                   int32_t json_output)
{
    const TestCase *tests;
    const int32_t total = (int32_t)scenario_catalog_count();
    int32_t passed = 0;
    int32_t index;
    char line[TEST_LINE_BUFFER_SIZE];
    int32_t written;

    if (hal_init() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (validate_hal_negative_cases() != ENGINE_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (json_output == 0)
    {
        if (log_event_with_options("INFO", "Running automated validation tests", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (scenario_report_print_json_header() != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    tests = scenario_catalog_tests();

    for (index = 0; index < total; ++index)
    {
        passed += run_test_case(&tests[index],
                                show_sim,
                                use_color,
                                show_control,
                                show_state,
                                json_output,
                                (json_output != 0) && (index > 0));
    }

    if (json_output != 0)
    {
        if (scenario_report_print_json_footer(passed, total, (const ErrorInfo *)0) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (print_test_separator() != ENGINE_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        written = snprintf(line, sizeof(line), "Summary: %d/%d tests passed\n", passed, total);
        if ((written < 0) || (written >= (int32_t)sizeof(line)))
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (output_write_line(line) != ENGINE_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        if (passed == total)
        {
            if (log_event_with_options("INFO", "All tests passed", use_color) != STATUS_OK)
            {
                (void)hal_shutdown();
                return STATUS_INTERNAL_ERROR;
            }
            if (hal_shutdown() != STATUS_OK)
            {
                return STATUS_INTERNAL_ERROR;
            }
            return STATUS_OK;
        }

        if (log_event_with_options("ERROR", "One or more tests failed", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (hal_shutdown() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return (passed == total) ? STATUS_OK : STATUS_INTERNAL_ERROR;
}

StatusCode run_all_tests_with_full_options(int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state)
{
    return run_all_tests_with_json(show_sim, use_color, show_control, show_state, 0);
}

StatusCode run_all_tests(void)
{
    return run_all_tests_with_full_options(0, 0, 0, 0);
}

StatusCode run_all_tests_with_output(int32_t show_sim)
{
    return run_all_tests_with_full_options(show_sim, 0, 0, 0);
}

StatusCode run_all_tests_with_options(int32_t show_sim, int32_t use_color)
{
    return run_all_tests_with_full_options(show_sim, use_color, 0, 0);
}

StatusCode run_named_scenario_with_json(const char *name,
                                        int32_t show_sim,
                                        int32_t use_color,
                                        int32_t show_control,
                                        int32_t show_state,
                                        int32_t json_output)
{
    const TestCase *selected_test;
    EngineState engine;
    int32_t result;
    StatusCode status;
    StatusCode log_status;
    int32_t expected_result;
    char line[TEST_LINE_BUFFER_SIZE];
    int32_t written;
    TickReport tick_reports[MAX_PROFILE_TICKS];
    uint32_t tick_report_count = 0U;
    int32_t local_show_sim;
    int32_t local_show_control;
    int32_t local_show_state;

    if (hal_init() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (valid_scenario_name(name) == 0)
    {
        (void)hal_shutdown();
        return STATUS_INVALID_ARGUMENT;
    }

    selected_test = scenario_catalog_find_named(name);
    if (selected_test == (const TestCase *)0)
    {
        (void)hal_shutdown();
        return STATUS_INVALID_ARGUMENT;
    }

    status = engine_reset(&engine);
    if (status != STATUS_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    local_show_sim = (json_output != 0) ? 0 : show_sim;
    local_show_control = (json_output != 0) ? 0 : show_control;
    local_show_state = (json_output != 0) ? 0 : show_state;

    expected_result = selected_test->expected_result;
    result = selected_test->scenario_func(&engine,
                                          local_show_sim,
                                          local_show_control,
                                          local_show_state,
                                          tick_reports,
                                          MAX_PROFILE_TICKS,
                                          &tick_report_count);

    if (result == ENGINE_ERROR)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (json_output != 0)
    {
        if (scenario_report_print_json_header() != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (scenario_report_print_json_scenario_object(name,
                                                       selected_test->requirement_id,
                                                       tick_reports,
                                                       tick_report_count,
                                                       expected_result,
                                                       result,
                                                       1) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (scenario_report_print_json_footer((result == expected_result) ? 1 : 0, 1, (const ErrorInfo *)0) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (hal_shutdown() != STATUS_OK)
        {
            return STATUS_INTERNAL_ERROR;
        }
        return STATUS_OK;
    }

    if (result == ENGINE_OK)
    {
        log_status = log_event_with_options("INFO", "Scenario evaluated: OK", use_color);
    }
    else if (result == ENGINE_WARNING)
    {
        log_status = log_event_with_options("WARN", "Scenario evaluated: WARNING", use_color);
    }
    else
    {
        log_status = log_event_with_options("ERROR", "Scenario evaluated: SHUTDOWN", use_color);
    }
    if (log_status != STATUS_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "Scenario '%s' result: %s\n",
                       name,
                       scenario_report_result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int32_t)sizeof(line)))
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }
    if (output_write_line(line) != ENGINE_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (hal_shutdown() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_OK;
}

StatusCode run_named_scenario_with_full_options(const char *name,
                                                int32_t show_sim,
                                                int32_t use_color,
                                                int32_t show_control,
                                                int32_t show_state)
{
    return run_named_scenario_with_json(name, show_sim, use_color, show_control, show_state, 0);
}

StatusCode run_named_scenario(const char *name)
{
    return run_named_scenario_with_full_options(name, 0, 0, 0, 0);
}

StatusCode run_named_scenario_with_output(const char *name, int32_t show_sim)
{
    return run_named_scenario_with_full_options(name, show_sim, 0, 0, 0);
}

StatusCode run_named_scenario_with_options(const char *name, int32_t show_sim, int32_t use_color)
{
    return run_named_scenario_with_full_options(name, show_sim, use_color, 0, 0);
}

StatusCode run_scripted_scenario_with_json(const char *script_path,
                                           int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state,
                                           int32_t json_output,
                                           int32_t strict_mode)
{
    EngineState engine;
    ScriptScenarioData script_data;
    TickReport tick_reports[SCRIPT_PARSER_MAX_TICKS + 1U];
    uint32_t tick_report_count = 0U;
    StatusCode status;
    int32_t result;
    char error_message[TEST_LINE_BUFFER_SIZE];
    char line[TEST_LINE_BUFFER_SIZE];
    int32_t written;
    ErrorInfo error_info;

    error_info.code = STATUS_OK;
    error_info.module = "test_runner";
    error_info.function = "run_scripted_scenario_with_json";
    error_info.tick = 0U;
    error_info.severity = SEVERITY_INFO;
    error_info.recoverability = RECOVERABILITY_RECOVERABLE;

    if (hal_init() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (script_parser_parse_file(script_path,
                                 &script_data,
                                 error_message,
                                 (uint32_t)sizeof(error_message),
                                 strict_mode) != STATUS_OK)
    {
        error_info.code = STATUS_PARSE_ERROR;
        error_info.module = "script_parser";
        error_info.function = "script_parser_parse_file";
        error_info.tick = 0U;
        error_info.severity = SEVERITY_ERROR;
        error_info.recoverability = RECOVERABILITY_RECOVERABLE;
        if (json_output == 0)
        {
            (void)log_event_with_options("ERROR", error_message, use_color);
        }
        else
        {
            if ((scenario_report_print_json_header() == STATUS_OK) &&
                (scenario_report_print_json_footer(0, 0, &error_info) == STATUS_OK))
            {
                (void)hal_shutdown();
                return STATUS_PARSE_ERROR;
            }
        }
        (void)hal_shutdown();
        return STATUS_PARSE_ERROR;
    }

    status = engine_reset(&engine);
    if (status != STATUS_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if ((strict_mode != 0) && (script_data.parse_warning_count > 0U))
    {
        (void)hal_shutdown();
        return STATUS_PARSE_ERROR;
    }

    if ((strict_mode == 0) && (script_data.parse_warning_count > 0U) && (json_output == 0))
    {
        if (log_event_with_options("WARN", "Script parse warnings detected and tolerated (non-strict mode)", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    result = execute_profile_frames(&engine,
                                    script_data.tick_values,
                                    script_data.sensor_frames,
                                    script_data.tick_count,
                                    (json_output != 0) ? 0 : show_sim,
                                    (json_output != 0) ? 0 : show_control,
                                    (json_output != 0) ? 0 : show_state,
                                    tick_reports,
                                    SCRIPT_PARSER_MAX_TICKS + 1U,
                                    &tick_report_count);
    if (result == ENGINE_ERROR)
    {
        if (hal_get_last_error(&error_info) != STATUS_OK)
        {
            error_info.code = STATUS_INTERNAL_ERROR;
            error_info.module = "scenario";
            error_info.function = "execute_profile_frames";
            error_info.tick = 0U;
            error_info.severity = SEVERITY_FATAL;
            error_info.recoverability = RECOVERABILITY_NON_RECOVERABLE;
        }
        if (json_output != 0)
        {
            if ((scenario_report_print_json_header() == STATUS_OK) &&
                (scenario_report_print_json_footer(0, 0, &error_info) == STATUS_OK))
            {
                (void)hal_shutdown();
                return STATUS_INTERNAL_ERROR;
            }
        }
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (json_output != 0)
    {
        if (scenario_report_print_json_header() != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        if (scenario_report_print_json_scenario_object("scripted_scenario",
                                                       REQ_ENG_SCRIPT,
                                                       tick_reports,
                                                       tick_report_count,
                                                       result,
                                                       result,
                                                       1) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }

        if (scenario_report_print_json_footer(1, 1, (const ErrorInfo *)0) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
        if (hal_shutdown() != STATUS_OK)
        {
            return STATUS_INTERNAL_ERROR;
        }
        return STATUS_OK;
    }

    if (result == ENGINE_OK)
    {
        if (log_event_with_options("INFO", "Scripted scenario evaluated: OK", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else if (result == ENGINE_WARNING)
    {
        if (log_event_with_options("WARN", "Scripted scenario evaluated: WARNING", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (log_event_with_options("ERROR", "Scripted scenario evaluated: SHUTDOWN", use_color) != STATUS_OK)
        {
            (void)hal_shutdown();
            return STATUS_INTERNAL_ERROR;
        }
    }

    written = snprintf(line,
                       sizeof(line),
                       "Script '%s' result: %s\n",
                       script_path,
                       scenario_report_result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int32_t)sizeof(line)))
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (output_write_line(line) != ENGINE_OK)
    {
        (void)hal_shutdown();
        return STATUS_INTERNAL_ERROR;
    }

    if (hal_shutdown() != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_OK;
}
