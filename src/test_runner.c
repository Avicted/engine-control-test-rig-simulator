#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "hal.h"
#include "logger.h"
#include "output.h"
#include "requirements.h"
#include "scenario_catalog.h"
#include "scenario_profiles.h"
#include "scenario_report.h"
#include "script_parser.h"
#include "test_runner.h"

#define TEST_LINE_BUFFER_SIZE 256
#define MAX_PROFILE_TICKS 26U

static int32_t print_test_separator(void)
{
    return output_write_line("------------------------------------------------------------\n");
}

int32_t execute_profile(EngineState *engine,
                        const uint32_t *tick_values,
                        const float *rpm_values,
                        const float *temp_values,
                        const float *oil_values,
                        const int32_t *run_values,
                        uint32_t tick_count,
                        int32_t show_sim,
                        int32_t show_control,
                        int32_t show_state,
                        TickReport *tick_reports,
                        uint32_t tick_report_capacity,
                        uint32_t *tick_report_count)
{
    HAL_SensorFrame sensor_frame;
    uint32_t tick_index;
    StatusCode status;
    int32_t result = ENGINE_OK;

    if ((engine == (EngineState *)0) || (rpm_values == (const float *)0) ||
        (temp_values == (const float *)0) || (oil_values == (const float *)0))
    {
        return ENGINE_ERROR;
    }

    if ((tick_count == 0U) || (tick_count > SCRIPT_PARSER_MAX_TICKS))
    {
        return ENGINE_ERROR;
    }

    if (tick_report_count != (uint32_t *)0)
    {
        *tick_report_count = 0U;
    }

    if ((tick_reports != (TickReport *)0) && (tick_report_capacity < tick_count + 1U))
    {
        return ENGINE_ERROR;
    }

    /* Record tick 0: INIT state before engine_start */
    if (tick_reports != (TickReport *)0)
    {
        tick_reports[0U].tick = 0U;
        tick_reports[0U].rpm = engine->rpm;
        tick_reports[0U].temp = engine->temperature;
        tick_reports[0U].oil = engine->oil_pressure;
        tick_reports[0U].run = engine->is_running;
        tick_reports[0U].result = ENGINE_OK;
        tick_reports[0U].control = 0.0f;
        tick_reports[0U].mode = engine->mode;
    }

    status = engine_start(engine);
    if (status != STATUS_OK)
    {
        return ENGINE_ERROR;
    }

    for (tick_index = 0U; tick_index < tick_count; ++tick_index)
    {
        int32_t run_flag;

        if (run_values == (const int32_t *)0)
        {
            run_flag = 1;
        }
        else
        {
            run_flag = run_values[tick_index];
        }

        if ((run_flag != 0) && (run_flag != 1))
        {
            return ENGINE_ERROR;
        }

        sensor_frame.is_running = run_flag;
        sensor_frame.rpm = rpm_values[tick_index];
        sensor_frame.temperature = temp_values[tick_index];
        sensor_frame.oil_pressure = oil_values[tick_index];

        if (hal_read_sensors(&sensor_frame) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        if (hal_apply_sensors(&sensor_frame, engine) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        if (evaluate_engine(engine, &result) != STATUS_OK)
        {
            return ENGINE_ERROR;
        }

        if (tick_reports != (TickReport *)0)
        {
            float control_output = 0.0f;
            uint32_t report_index = tick_index + 1U; /* +1: index 0 is the pre-start INIT tick */

            if (compute_control_output(engine, &control_output) != STATUS_OK)
            {
                return ENGINE_ERROR;
            }

            tick_reports[report_index].tick =
                (tick_values == (const uint32_t *)0) ? (tick_index + 1U) : tick_values[tick_index];
            tick_reports[report_index].rpm = rpm_values[tick_index];
            tick_reports[report_index].temp = temp_values[tick_index];
            tick_reports[report_index].oil = oil_values[tick_index];
            tick_reports[report_index].run = run_flag;
            tick_reports[report_index].result = result;
            tick_reports[report_index].control = control_output;
            tick_reports[report_index].mode = engine->mode;
        }

        if ((show_sim != 0) || (show_control != 0) || (show_state != 0))
        {
            uint32_t tick_value;

            tick_value = (tick_values == (const uint32_t *)0) ? (tick_index + 1U) : tick_values[tick_index];
            if (scenario_report_print_tick_details(tick_value,
                                                   engine,
                                                   result,
                                                   show_sim,
                                                   show_control,
                                                   show_state) != ENGINE_OK)
            {
                return ENGINE_ERROR;
            }
        }

        status = engine_update(engine);
        if (status != STATUS_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (tick_report_count != (uint32_t *)0)
    {
        *tick_report_count = tick_count + 1U; /* +1 for the pre-start INIT tick at index 0 */
    }

    return result;
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
        int written;

        written = snprintf(line,
                           sizeof(line),
                           "%s | %s | expected=%s | actual=%s | %s\n",
                           test_case->requirement_id,
                           test_case->test_name,
                           scenario_report_result_to_display_string(test_case->expected_result, use_color),
                           scenario_report_result_to_display_string(actual_result, use_color),
                           scenario_report_pass_fail_display(actual_result == test_case->expected_result, use_color));
        if ((written < 0) || (written >= (int)sizeof(line)))
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
    if (hal_read_sensors((HAL_SensorFrame *)0) != STATUS_INVALID_ARGUMENT)
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
    int written;

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
        if (scenario_report_print_json_footer(passed, total) != STATUS_OK)
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
        if ((written < 0) || (written >= (int)sizeof(line)))
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
    int written;
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
        if (scenario_report_print_json_footer((result == expected_result) ? 1 : 0, 1) != STATUS_OK)
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
    if ((written < 0) || (written >= (int)sizeof(line)))
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
    int written;

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
        if (json_output == 0)
        {
            (void)log_event_with_options("ERROR", error_message, use_color);
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

    result = execute_profile(&engine,
                             script_data.tick_values,
                             script_data.rpm_values,
                             script_data.temp_values,
                             script_data.oil_values,
                             script_data.run_values,
                             script_data.tick_count,
                             (json_output != 0) ? 0 : show_sim,
                             (json_output != 0) ? 0 : show_control,
                             (json_output != 0) ? 0 : show_state,
                             tick_reports,
                             SCRIPT_PARSER_MAX_TICKS + 1U,
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

        if (scenario_report_print_json_footer(1, 1) != STATUS_OK)
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
    if ((written < 0) || (written >= (int)sizeof(line)))
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
