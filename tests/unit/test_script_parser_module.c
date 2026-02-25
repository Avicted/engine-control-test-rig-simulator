#include <stdio.h>
#include <string.h>

#include "script_parser.h"
#include "test_harness.h"

static int32_t test_missing_tokens_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_missing_tokens.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "Malformed") != (char *)0);
    return 1;
}

static int32_t test_duplicate_ticks_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_duplicate_ticks.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2600 TEMP 81 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "non-increasing") != (char *)0);
    return 1;
}

static int32_t test_non_numeric_values_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_nonnumeric.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM fast TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    return 1;
}

static int32_t test_out_of_order_ticks_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_out_of_order.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 2 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2400 TEMP 79 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "non-increasing") != (char *)0);
    return 1;
}

static int32_t test_corrupt_frame_directive_is_accepted(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_corrupt_directive.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 5 FRAME CORRUPT\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_OK,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_EQ(2U, scenario_data.tick_count);
    return 1;
}

static int32_t test_missing_run_field_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_missing_run.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "Malformed") != (char *)0);
    return 1;
}

static int32_t test_corrupt_directive_before_sensor_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_corrupt_before_sensor.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 FRAME CORRUPT\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "before first valid sensor frame") != (char *)0);
    return 1;
}

int32_t register_script_parser_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"parser_missing_tokens", test_missing_tokens_rejected},
        {"parser_duplicate_ticks", test_duplicate_ticks_rejected},
        {"parser_non_numeric", test_non_numeric_values_rejected},
        {"parser_out_of_order", test_out_of_order_ticks_rejected},
        {"parser_corrupt_directive", test_corrupt_frame_directive_is_accepted},
        {"parser_missing_run_field", test_missing_run_field_rejected},
        {"parser_corrupt_before_sensor", test_corrupt_directive_before_sensor_rejected}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
