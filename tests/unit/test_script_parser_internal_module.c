#include <stdio.h>
#include <string.h>

#include "script_parser.h"
#include "test_harness.h"

static int32_t g_sp_encode_call_count = 0;
static int32_t g_sp_fail_on_encode_call = -1;
static int32_t g_sp_fail_fclose = 0;

static StatusCode script_parser_ut_hal_encode_sensor_frame(const HAL_SensorFrame *sensor_frame, HAL_Frame *frame_out)
{
    (void)sensor_frame;

    g_sp_encode_call_count += 1;
    if ((g_sp_fail_on_encode_call > 0) && (g_sp_encode_call_count == g_sp_fail_on_encode_call))
    {
        return STATUS_PARSE_ERROR;
    }

    if (frame_out == (HAL_Frame *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    frame_out->id = HAL_SENSOR_FRAME_ID;
    frame_out->dlc = 8U;
    (void)memset(frame_out->data, 0, sizeof(frame_out->data));
    return STATUS_OK;
}

static int script_parser_ut_fclose(FILE *file)
{
    if (g_sp_fail_fclose != 0)
    {
        g_sp_fail_fclose = 0;
        return EOF;
    }

    return fclose(file);
}

#define hal_encode_sensor_frame script_parser_ut_hal_encode_sensor_frame
#define fclose script_parser_ut_fclose
#define script_parser_parse_file script_parser_parse_file_internal_ut
#include "../../src/scenario/script_parser.c"
#undef script_parser_parse_file
#undef fclose
#undef hal_encode_sensor_frame

static int32_t test_script_internal_parse_helpers_null(void)
{
    uint32_t tick = 0U;
    float f = 0.0f;
    int32_t run = 0;

    ASSERT_EQ(0, parse_strict_uint((const char *)0, &tick));
    ASSERT_EQ(0, parse_strict_float((const char *)0, &f));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_script_line((const char *)0, &tick, &f, &f, &f, &run));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_corrupt_frame_line((const char *)0, &tick));
    ASSERT_EQ(1, line_is_whitespace_only((const char *)0));
    ASSERT_EQ(0, line_is_comment((const char *)0));
    return 1;
}

static int32_t test_script_internal_strict_parse_invalid_numeric(void)
{
    uint32_t tick = 0U;
    float f = 0.0f;

    ASSERT_EQ(0, parse_strict_uint("abc", &tick));
    ASSERT_EQ(0, parse_strict_float("nanx", &f));
    return 1;
}

static int32_t test_script_internal_encode_failure_sensor_line(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_internal_encode_fail_sensor.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    g_sp_encode_call_count = 0;
    g_sp_fail_on_encode_call = 1;
    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file_internal_ut(path,
                                                       &scenario_data,
                                                       error_message,
                                                       (uint32_t)sizeof(error_message),
                                                       0));
    ASSERT_TRUE(strstr(error_message, "out-of-range sensor values") != (char *)0);
    g_sp_fail_on_encode_call = -1;
    return 1;
}

static int32_t test_script_internal_encode_failure_corrupt_line(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_internal_encode_fail_corrupt.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 2 FRAME CORRUPT\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    g_sp_encode_call_count = 0;
    g_sp_fail_on_encode_call = 2;
    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  script_parser_parse_file_internal_ut(path,
                                                       &scenario_data,
                                                       error_message,
                                                       (uint32_t)sizeof(error_message),
                                                       0));
    ASSERT_TRUE(strstr(error_message, "failed to build corrupt frame") != (char *)0);
    g_sp_fail_on_encode_call = -1;
    return 1;
}

static int32_t test_script_internal_close_failure_returns_io_error(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *path = "build/unit_script_internal_close_fail.txt";
    FILE *file;

    file = fopen(path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    g_sp_fail_fclose = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  script_parser_parse_file_internal_ut(path,
                                                       &scenario_data,
                                                       error_message,
                                                       (uint32_t)sizeof(error_message),
                                                       0));
    ASSERT_TRUE(strstr(error_message, "Failed to close script file") != (char *)0);
    return 1;
}

int32_t register_script_parser_internal_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"script_internal_helpers_null", test_script_internal_parse_helpers_null},
        {"script_internal_numeric_invalid", test_script_internal_strict_parse_invalid_numeric},
        {"script_internal_encode_fail_sensor", test_script_internal_encode_failure_sensor_line},
        {"script_internal_encode_fail_corrupt", test_script_internal_encode_failure_corrupt_line},
        {"script_internal_close_fail", test_script_internal_close_failure_returns_io_error}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
