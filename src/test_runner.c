#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "logger.h"
#include "sensors.h"
#include "test_runner.h"

#define TEST_LINE_BUFFER_SIZE 192
#define TEST_NAME_COLUMN_WIDTH 24
#define RESULT_COLUMN_WIDTH 8

#define ANSI_RESET "\x1b[0m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"

static int print_line(const char *line);

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

static int scenario_warning_high_rpm_temp(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    engine->rpm = 3600.0f;
    engine->temperature = 86.0f;
    engine->oil_pressure = 3.0f;
    return ENGINE_OK;
}

static int scenario_temperature_threshold_ok(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    engine->rpm = 2000.0f;
    engine->temperature = 95.0f;
    engine->oil_pressure = 3.2f;
    return ENGINE_OK;
}

static int scenario_oil_threshold_ok(EngineState *engine)
{
    if (engine == (EngineState *)0)
    {
        return ENGINE_ERROR;
    }

    engine->is_running = 1;
    engine->rpm = 2200.0f;
    engine->temperature = 80.0f;
    engine->oil_pressure = 2.5f;
    return ENGINE_OK;
}

static int run_test_case(const TestCase *test_case, int show_sim, int use_color, int show_control)
{
    EngineState engine;
    int actual_result;
    int status;
    int control_status;
    float control_output = 0.0f;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if ((test_case == (const TestCase *)0) || (test_case->name == (const char *)0) ||
        (test_case->scenario_func == (int (*)(EngineState *))0))
    {
        return 0;
    }

    status = engine_reset(&engine);
    if (status != ENGINE_OK)
    {
        return 0;
    }

    status = test_case->scenario_func(&engine);
    if (status != ENGINE_OK)
    {
        return 0;
    }

    if (show_control != 0)
    {
        control_status = compute_control_output(&engine, &control_output);
        if (control_status != ENGINE_OK)
        {
            return 0;
        }
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

    if (show_sim != 0)
    {
        written = snprintf(line,
                           sizeof(line),
                           "SIM  | rpm=%7.2f  temp=%6.2f  oil=%5.2f  run=%d\n",
                           engine.rpm,
                           engine.temperature,
                           engine.oil_pressure,
                           engine.is_running);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 0;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return 0;
        }
    }

    if (show_control != 0)
    {
        written = snprintf(line, sizeof(line), "CTRL | output=%6.2f%%\n", control_output);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 0;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return 0;
        }
    }

    actual_result = evaluate_engine(&engine);
    if (actual_result == ENGINE_ERROR)
    {
        return 0;
    }

    if (actual_result == test_case->expected_result)
    {
        written = snprintf(line,
                           sizeof(line),
                           "EVAL | expected=%-*s  actual=%-*s  => %s\n",
                           RESULT_COLUMN_WIDTH,
                           result_to_display_string(test_case->expected_result, use_color),
                           RESULT_COLUMN_WIDTH,
                           result_to_display_string(actual_result, use_color),
                           pass_fail_display(1, use_color));
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return 0;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return 0;
        }
        return 1;
    }

    written = snprintf(line,
                       sizeof(line),
                       "EVAL | expected=%-*s  actual=%-*s  => %s\n",
                       RESULT_COLUMN_WIDTH,
                       result_to_display_string(test_case->expected_result, use_color),
                       RESULT_COLUMN_WIDTH,
                       result_to_display_string(actual_result, use_color),
                       pass_fail_display(0, use_color));
    if ((written < 0) || (written >= (int)sizeof(line)))
    {
        return 0;
    }
    if (print_line(line) != ENGINE_OK)
    {
        return 0;
    }

    return 0;
}

int run_all_tests_with_full_options(int show_sim, int use_color, int show_control)
{
    const TestCase tests[MAX_TESTS] = {
        {"normal_operation", scenario_normal_ramp_up, ENGINE_OK},
        {"overheat_test", scenario_overheat, ENGINE_SHUTDOWN},
        {"pressure_failure", scenario_oil_pressure_failure, ENGINE_SHUTDOWN},
        {"warning_high_rpm_temp", scenario_warning_high_rpm_temp, ENGINE_WARNING},
        {"temp_threshold_ok", scenario_temperature_threshold_ok, ENGINE_OK},
        {"oil_threshold_ok", scenario_oil_threshold_ok, ENGINE_OK}};
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
        passed += run_test_case(&tests[index], show_sim, use_color, show_control);
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
    return run_all_tests_with_full_options(0, 0, 0);
}

int run_all_tests_with_output(int show_sim)
{
    return run_all_tests_with_full_options(show_sim, 0, 0);
}

int run_all_tests_with_options(int show_sim, int use_color)
{
    return run_all_tests_with_full_options(show_sim, use_color, 0);
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

int run_named_scenario_with_full_options(const char *name, int show_sim, int use_color, int show_control)
{
    EngineState engine;
    int result;
    int status;
    int control_status;
    int log_status;
    float control_output = 0.0f;
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
        status = scenario_normal_ramp_up(&engine);
    }
    else if (strncmp(name, "overheat", MAX_SCENARIO_NAME_LEN) == 0)
    {
        status = scenario_overheat(&engine);
    }
    else if (strncmp(name, "pressure_failure", MAX_SCENARIO_NAME_LEN) == 0)
    {
        status = scenario_oil_pressure_failure(&engine);
    }
    else
    {
        return ENGINE_ERROR;
    }

    if (status != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if ((show_sim != 0) || (show_control != 0))
    {
        if (print_test_separator() != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }

        written = snprintf(line, sizeof(line), "TEST | %s\n", name);
        if ((written < 0) || (written >= (int)sizeof(line)))
        {
            return ENGINE_ERROR;
        }
        if (print_line(line) != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    if (show_sim != 0)
    {
        written = snprintf(line,
                           sizeof(line),
                           "SIM  | rpm=%7.2f  temp=%6.2f  oil=%5.2f  run=%d\n",
                           engine.rpm,
                           engine.temperature,
                           engine.oil_pressure,
                           engine.is_running);
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
        control_status = compute_control_output(&engine, &control_output);
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

    if ((show_sim != 0) || (show_control != 0))
    {
        if (print_test_separator() != ENGINE_OK)
        {
            return ENGINE_ERROR;
        }
    }

    result = evaluate_engine(&engine);
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
    return run_named_scenario_with_full_options(name, 0, 0, 0);
}

int run_named_scenario_with_output(const char *name, int show_sim)
{
    return run_named_scenario_with_full_options(name, show_sim, 0, 0);
}

int run_named_scenario_with_options(const char *name, int show_sim, int use_color)
{
    return run_named_scenario_with_full_options(name, show_sim, use_color, 0);
}
