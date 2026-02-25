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

static int32_t test_comments_and_whitespace_non_strict_allowed(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_comments_and_whitespace.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("   # first comment\n", file) >= 0);
    ASSERT_TRUE(fputs(" \t \r\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 1 RPM 1500 TEMP 70 OIL 2.5 RUN 0\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 2 RPM 1600 TEMP 71 OIL 2.6 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_OK,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_EQ(2U, scenario_data.tick_count);
    ASSERT_EQ(1U, scenario_data.parse_warning_count);
    ASSERT_EQ(1U, scenario_data.tick_values[0]);
    ASSERT_EQ(2U, scenario_data.tick_values[1]);
    return 1;
}

static int32_t test_comment_in_strict_mode_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_comment_strict.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("# strict mode comment\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 1 RPM 1500 TEMP 70 OIL 2.5 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 1));
    ASSERT_TRUE(strstr(error_message, "comments are not allowed") != (char *)0);
    return 1;
}

static int32_t test_malformed_sensor_key_order_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_bad_key_order.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("RPM 1500 TICK 1 TEMP 70 OIL 2.5 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "Malformed") != (char *)0);
    return 1;
}

static int32_t test_invalid_run_value_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_invalid_run.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 1500 TEMP 70 OIL 2.5 RUN 2\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    return 1;
}

static int32_t test_out_of_range_sensor_value_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_oil_out_of_range.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 1500 TEMP 70 OIL 11.0 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    return 1;
}

static int32_t test_invalid_tick_value_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_invalid_tick.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK +1 RPM 1500 TEMP 70 OIL 2.5 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    return 1;
}

static int32_t test_overlong_line_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_overlong_line.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 1500 TEMP 70 OIL 2.5 RUN 1", file) >= 0);
    ASSERT_TRUE(fputs("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", file) >= 0);
    ASSERT_TRUE(fputs("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", file) >= 0);
    ASSERT_TRUE(fputs("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", file) >= 0);
    ASSERT_TRUE(fputs("\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "exceeds maximum length") != (char *)0);
    return 1;
}

static int32_t test_script_file_not_found_and_invalid_arguments(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];

    ASSERT_STATUS(STATUS_IO_ERROR,
                  script_parser_parse_file("build/no_such_script.txt",
                                           &scenario_data,
                                           error_message,
                                           (uint32_t)sizeof(error_message),
                                           0));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  script_parser_parse_file((const char *)0,
                                           &scenario_data,
                                           error_message,
                                           (uint32_t)sizeof(error_message),
                                           0));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  script_parser_parse_file("scenarios/normal_operation.txt",
                                           (ScriptScenarioData *)0,
                                           error_message,
                                           (uint32_t)sizeof(error_message),
                                           0));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  script_parser_parse_file("scenarios/normal_operation.txt",
                                           &scenario_data,
                                           (char *)0,
                                           (uint32_t)sizeof(error_message),
                                           0));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  script_parser_parse_file("scenarios/normal_operation.txt",
                                           &scenario_data,
                                           error_message,
                                           0U,
                                           0));

    return 1;
}

static int32_t test_empty_script_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_empty.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "contains no tick data") != (char *)0);
    return 1;
}

static int32_t test_corrupt_directive_missing_token_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_corrupt_missing_token.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 2 FRAME\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "Malformed") != (char *)0);
    return 1;
}

static int32_t test_corrupt_directive_invalid_tick_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_corrupt_invalid_tick.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK +2 FRAME CORRUPT\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "Malformed") != (char *)0);
    return 1;
}

static int32_t test_tick_count_limit_rejected(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_tick_limit.txt";
    FILE *file;
    uint32_t tick;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    for (tick = 1U; tick <= (SCRIPT_PARSER_MAX_TICKS + 1U); ++tick)
    {
        ASSERT_TRUE(fprintf(file,
                            "TICK %u RPM 2500 TEMP 80 OIL 3.0 RUN 1\n",
                            tick) > 0);
    }
    ASSERT_EQ(0, fclose(file));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file(path, &scenario_data, error_message, (uint32_t)sizeof(error_message), 0));
    ASSERT_TRUE(strstr(error_message, "tick count exceeds limit") != (char *)0);
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
        {"parser_corrupt_before_sensor", test_corrupt_directive_before_sensor_rejected},
        {"parser_comments_whitespace", test_comments_and_whitespace_non_strict_allowed},
        {"parser_comment_strict", test_comment_in_strict_mode_rejected},
        {"parser_bad_key_order", test_malformed_sensor_key_order_rejected},
        {"parser_invalid_run", test_invalid_run_value_rejected},
        {"parser_range_rejected", test_out_of_range_sensor_value_rejected},
        {"parser_invalid_tick", test_invalid_tick_value_rejected},
        {"parser_overlong_line", test_overlong_line_rejected},
        {"parser_io_and_args", test_script_file_not_found_and_invalid_arguments},
        {"parser_empty_script", test_empty_script_rejected},
        {"parser_corrupt_missing_token", test_corrupt_directive_missing_token_rejected},
        {"parser_corrupt_invalid_tick", test_corrupt_directive_invalid_tick_rejected},
        {"parser_tick_limit", test_tick_count_limit_rejected}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
