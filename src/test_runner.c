#include <stdio.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "logger.h"
#include "sensors.h"
#include "test_runner.h"

#define TEST_LINE_BUFFER_SIZE 128

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

static int run_test_case(const TestCase *test_case)
{
    EngineState engine;
    int actual_result;
    int status;
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

    actual_result = evaluate_engine(&engine);
    if (actual_result == ENGINE_ERROR)
    {
        return 0;
    }

    if (actual_result == test_case->expected_result)
    {
        written = snprintf(line,
                           sizeof(line),
                           "PASS: %s (expected=%s, actual=%s)\n",
                           test_case->name,
                           result_to_string(test_case->expected_result),
                           result_to_string(actual_result));
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
                       "FAIL: %s (expected=%s, actual=%s)\n",
                       test_case->name,
                       result_to_string(test_case->expected_result),
                       result_to_string(actual_result));
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

int run_all_tests(void)
{
    const TestCase tests[MAX_TESTS] = {
        {"normal_operation", scenario_normal_ramp_up, ENGINE_OK},
        {"overheat_test", scenario_overheat, ENGINE_SHUTDOWN},
        {"pressure_failure", scenario_oil_pressure_failure, ENGINE_SHUTDOWN}};
    int passed = 0;
    const int total = MAX_TESTS;
    int index;
    char line[TEST_LINE_BUFFER_SIZE];
    int written;

    if (log_event("INFO", "Running automated validation tests") != 0)
    {
        return 1;
    }

    for (index = 0; index < MAX_TESTS; ++index)
    {
        passed += run_test_case(&tests[index]);
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
        if (log_event("INFO", "All tests passed") != 0)
        {
            return 1;
        }
        return 0;
    }

    if (log_event("ERROR", "One or more tests failed") != 0)
    {
        return 1;
    }

    return 1;
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

int run_named_scenario(const char *name)
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

    result = evaluate_engine(&engine);
    if (result == ENGINE_ERROR)
    {
        return ENGINE_ERROR;
    }

    if (result == ENGINE_OK)
    {
        log_status = log_event("INFO", "Scenario evaluated: OK");
    }
    else if (result == ENGINE_WARNING)
    {
        log_status = log_event("WARN", "Scenario evaluated: WARNING");
    }
    else
    {
        log_status = log_event("ERROR", "Scenario evaluated: SHUTDOWN");
    }
    if (log_status != 0)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(line, sizeof(line), "Scenario '%s' result: %s\n", name, result_to_string(result));
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
