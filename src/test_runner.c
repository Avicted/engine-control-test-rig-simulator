#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "logger.h"
#include "test_runner.h"

#define TEST_LINE_BUFFER_SIZE 256
#define RESULT_COLUMN_WIDTH 8
#define MAX_PROFILE_TICKS 8U

#define ANSI_RESET "\x1b[0m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"

static int print_line(const char *line)
{
    int write_result;

    if (line == (const char *)0)
    {
        return ENGINE_ERROR;
    }

    write_result = fputs(line, stdout);
    if (write_result < 0)
    {
        return ENGINE_ERROR;
    }

    return ENGINE_OK;
}

static int print_test_separator(void)
{
    return print_line("------------------------------------------------------------\n");
}

static const char *result_to_string(int result)
{
    if (result == ENGINE_OK)
    {
        return "OK";
    }
    if (result == ENGINE_WARNING)
    {
        return "WARNING";
    }
    if (result == ENGINE_SHUTDOWN)
    {
        return "SHUTDOWN";
    }
    return "ERROR";
}

static const char *result_to_display_string(int result, int use_color)
{
    if (use_color == 0)
    {
        return result_to_string(result);
    }

    if (result == ENGINE_OK)
    {
        return ANSI_GREEN "OK" ANSI_RESET;
    }
    if (result == ENGINE_WARNING)
    {
        return ANSI_YELLOW "WARNING" ANSI_RESET;
    }
    if (result == ENGINE_SHUTDOWN)
    {
        return ANSI_RED "SHUTDOWN" ANSI_RESET;
    }

    return ANSI_RED "ERROR" ANSI_RESET;
}

static const char *pass_fail_display(int is_pass, int use_color)
{
    if (is_pass != 0)
    {
        if (use_color != 0)
        {
            return ANSI_GREEN "PASS" ANSI_RESET;
        }
        return "PASS";
    }

    if (use_color != 0)
    {
        return ANSI_RED "FAIL" ANSI_RESET;
    }
    return "FAIL";
}

static int print_tick_details(unsigned int tick,
                              const EngineState *engine,
                              int result,
                              int show_sim,
                              int show_control,
                              int show_state)
{
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (engine == (const EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "TICK | %u  result=%s\n",
                       tick,
                       result_to_string(result));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if (show_sim != 0)
    {
        written = snprintf(line,
                           sizeof(line),
                           "SIM  | rpm=%7.2f  temp=%6.2f  oil=%5.2f  run=%d\n",
                           engine->rpm,
                           engine->temperature,
                           engine->oil_pressure,
                           engine->is_running);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (show_control != 0)
    {
        int control_status;
        float control_output = 0.0f;

        control_status = compute_control_output(engine, &control_output);
        if (control_status != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        written = snprintf(line, sizeof(line), "CTRL | output=%6.2f%%\n", control_output);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (show_state != 0)
    {
        const char *mode_string;

        if (engine_get_mode_string(engine, &mode_string) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        written = snprintf(line,
                           sizeof(line),
                           "STATE| mode=%-8s  temp_cnt=%u  oil_cnt=%u  combo_cnt=%u\n",
                           mode_string,
                           engine->fault_counters[ENGINE_FAULT_TEMP],
                           engine->fault_counters[ENGINE_FAULT_OIL_PRESSURE],
                           engine->fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED]);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    return ENGINE_OK;
}

static int execute_profile(EngineState *engine,
                           const float *rpm_values,
                           const float *temp_values,
                           const float *oil_values,
                           unsigned int tick_count,
                           int show_sim,
                           int show_control,
                           int show_state)
{
    unsigned int tick_index;
    int status;
    int result = ENGINE_OK;

    if ((engine == (EngineState *)0) || (rpm_values == (const float *)0) ||
        (temp_values == (const float *)0) || (oil_values == (const float *)0))
    {
        return ENGINE_ERROR;
    }

    if ((tick_count == 0U) || (tick_count > MAX_PROFILE_TICKS))
    {
        return ENGINE_ERROR;
    }

    status = engine_start(engine);
    if (status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    for (tick_index = 0U; tick_index < tick_count; ++tick_index)
    {
        engine->is_running = 1;
        engine->rpm = rpm_values[tick_index];
        engine->temperature = temp_values[tick_index];
        engine->oil_pressure = oil_values[tick_index];

        result = evaluate_engine(engine);
        if (result == ENGINE_ERROR)
        {
            return ENGINE_ERROR;
        }

        if ((show_sim != 0) || (show_control != 0) || (show_state != 0))
        {
            if (print_tick_details(tick_index + 1U, engine, result, show_sim, show_control, show_state) !=
                ENGINE_OK)
            {
                return ENGINE_ERROR;
            }
        }

        status = engine_update(engine);
        if (status != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    return result;
}

static int scenario_normal_operation(EngineState *engine, int show_sim, int show_control, int show_state)
{
    static const float rpm_values[] = {2200.0f, 2600.0f, 3000.0f, 3200.0f};
    static const float temp_values[] = {76.0f, 80.0f, 83.0f, 84.5f};
    static const float oil_values[] = {3.2f, 3.1f, 3.0f, 2.9f};

    return execute_profile(engine,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state);
}

static int scenario_overheat_short_duration(EngineState *engine, int show_sim, int show_control, int show_state)
{
    static const float rpm_values[] = {2400.0f, 2400.0f, 2400.0f, 2400.0f};
    static const float temp_values[] = {96.0f, 97.0f, 90.0f, 89.0f};
    static const float oil_values[] = {3.1f, 3.1f, 3.1f, 3.1f};

    return execute_profile(engine,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state);
}

static int scenario_overheat_persistent(EngineState *engine, int show_sim, int show_control, int show_state)
{
    static const float rpm_values[] = {2500.0f, 2500.0f, 2500.0f, 2500.0f};
    static const float temp_values[] = {96.0f, 97.0f, 98.0f, 99.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state);
}

static int scenario_oil_pressure_persistent(EngineState *engine, int show_sim, int show_control, int show_state)
{
    static const float rpm_values[] = {2600.0f, 2600.0f, 2600.0f, 2600.0f};
    static const float temp_values[] = {82.0f, 82.0f, 82.0f, 82.0f};
    static const float oil_values[] = {2.4f, 2.3f, 2.2f, 2.1f};

    return execute_profile(engine,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state);
}

static int scenario_combined_warning_persistent(EngineState *engine, int show_sim, int show_control, int show_state)
{
    static const float rpm_values[] = {3600.0f, 3600.0f, 3300.0f};
    static const float temp_values[] = {86.0f, 87.0f, 80.0f};
    static const float oil_values[] = {3.0f, 3.0f, 3.0f};

    return execute_profile(engine,
                           rpm_values,
                           temp_values,
                           oil_values,
                           (unsigned int)(sizeof(rpm_values) / sizeof(rpm_values[0])),
                           show_sim,
                           show_control,
                           show_state);
}

static int run_test_case(const TestCase *test_case, int show_sim, int use_color, int show_control, int show_state)
{
    EngineState engine;
    int actual_result;
    int status;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if ((test_case == (const TestCase *)0) || (test_case->name == (const char *)0) ||
        (test_case->scenario_func == (int (*)(EngineState *, int, int, int))0))
    {
        return 0;
    }

    status = engine_reset(&engine);
    if (status != ENGINE_OK)
    {
        return 0;
    }

    if (print_test_separator() != ENGINE_OK)
    {
        return 0;
    }

    written = snprintf(line, sizeof(line), "TEST | %s\n", test_case->name);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return 0;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return 0;
    }

    actual_result = test_case->scenario_func(&engine, show_sim, show_control, show_state);
    if (actual_result == ENGINE_ERROR)
    {
        return 0;
    }

    written = snprintf(line,
                       sizeof(line),
                       "EVAL | expected=%-*s  actual=%-*s  => %s\n",
                       RESULT_COLUMN_WIDTH,
                       result_to_display_string(test_case->expected_result, use_color),
                       RESULT_COLUMN_WIDTH,
                       result_to_display_string(actual_result, use_color),
                       pass_fail_display(actual_result == test_case->expected_result, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return 0;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return 0;
    }

    return (actual_result == test_case->expected_result) ? 1 : 0;
}

int run_all_tests_with_full_options(int show_sim, int use_color, int show_control, int show_state)
{
    const TestCase tests[MAX_TESTS] = {
        {"normal_operation", scenario_normal_operation, ENGINE_OK},
        {"overheat_lt_persistence", scenario_overheat_short_duration, ENGINE_OK},
        {"overheat_ge_persistence", scenario_overheat_persistent, ENGINE_SHUTDOWN},
        {"oil_low_ge_persistence", scenario_oil_pressure_persistent, ENGINE_SHUTDOWN},
        {"combined_warning_persistence", scenario_combined_warning_persistent, ENGINE_WARNING}};
    int passed = 0;
    const int total = MAX_TESTS;
    int index;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (log_event_with_options("INFO", "Running automated validation tests", use_color) != 0)
    {
        return 1;
    }

    for (index = 0; index < MAX_TESTS; ++index)
    {
        passed += run_test_case(&tests[index], show_sim, use_color, show_control, show_state);
    }

    if (print_test_separator() != ENGINE_OK)
    {
        return 1;
    }

    written = snprintf(line, sizeof(line), "Summary: %d/%d tests passed\n", passed, total);
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return 1;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return 1;
    }

    if (passed == total)
    {
        if (log_event_with_options("INFO", "All tests passed", use_color) != 0)
        {
            return 1;
        }
        return 0;
    }

    if (log_event_with_options("ERROR", "One or more tests failed", use_color) != 0)
    {
        return 1;
    }

    return 1;
}

int run_all_tests(void)
{
    return run_all_tests_with_full_options(0, 0, 0, 0);
}

int run_all_tests_with_output(int show_sim)
{
    return run_all_tests_with_full_options(show_sim, 0, 0, 0);
}

int run_all_tests_with_options(int show_sim, int use_color)
{
    return run_all_tests_with_full_options(show_sim, use_color, 0, 0);
}

static int valid_scenario_name(const char *name)
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

int run_named_scenario_with_full_options(const char *name,
                                         int show_sim,
                                         int use_color,
                                         int show_control,
                                         int show_state)
{
    EngineState engine;
    int result;
    int status;
    int log_status;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (valid_scenario_name(name) == 0)
    {
        return ENGINE_ERROR;
    }

    status = engine_reset(&engine);
    if (status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if (strncmp(name, "normal", MAX_SCENARIO_NAME_LEN) == 0)
    {
        result = scenario_normal_operation(&engine, show_sim, show_control, show_state);
    }
    else if (strncmp(name, "overheat", MAX_SCENARIO_NAME_LEN) == 0)
    {
        result = scenario_overheat_persistent(&engine, show_sim, show_control, show_state);
    }
    else if (strncmp(name, "pressure_failure", MAX_SCENARIO_NAME_LEN) == 0)
    {
        result = scenario_oil_pressure_persistent(&engine, show_sim, show_control, show_state);
    }
    else
    {
        return ENGINE_ERROR;
    }

    if (result == ENGINE_ERROR)
    {
        return ENGINE_ERROR;
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
    if (log_status != 0)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line,
                       sizeof(line),
                       "Scenario '%s' result: %s\n",
                       name,
                       result_to_display_string(result, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return ENGINE_ERROR;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    return result;
}

int run_named_scenario(const char *name)
{
    return run_named_scenario_with_full_options(name, 0, 0, 0, 0);
}

int run_named_scenario_with_output(const char *name, int show_sim)
{
    return run_named_scenario_with_full_options(name, show_sim, 0, 0, 0);
}

int run_named_scenario_with_options(const char *name, int show_sim, int use_color)
{
    return run_named_scenario_with_full_options(name, show_sim, use_color, 0, 0);
}
