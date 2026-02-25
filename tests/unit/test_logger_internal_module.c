#include <stdio.h>
#include <string.h>

#include "reporting/logger.h"
#include "test_harness.h"

static int32_t g_logger_ut_fail_fputs = 0;

static int logger_ut_fputs(const char *line, FILE *stream)
{
    if (g_logger_ut_fail_fputs != 0)
    {
        g_logger_ut_fail_fputs = 0;
        return -1;
    }

    return fputs(line, stream);
}

StatusCode logger_ut_log_event_with_options(const char *level, const char *message, int32_t use_color);

#define fputs logger_ut_fputs
#define logger_set_level logger_ut_set_level
#define logger_set_level_from_string logger_ut_set_level_from_string
#define logger_get_level logger_ut_get_level
#define log_event logger_ut_log_event
#define log_event_with_options logger_ut_log_event_with_options
#define logger_log_tick logger_ut_log_tick
#include "../../src/reporting/logger.c"
#undef logger_log_tick
#undef log_event_with_options
#undef log_event
#undef logger_get_level
#undef logger_set_level_from_string
#undef logger_set_level
#undef fputs

static int32_t test_logger_internal_log_event_io_error(void)
{
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level(LOG_LEVEL_DEBUG));
    g_logger_ut_fail_fputs = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  logger_ut_log_event_with_options("INFO", "io fail", 0));
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_logger_internal_log_tick_io_error(void)
{
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level(LOG_LEVEL_DEBUG));
    g_logger_ut_fail_fputs = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  logger_ut_log_tick("HAL", LOG_LEVEL_INFO, 1U, "io fail", 0));
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_logger_internal_unknown_tick_level_uses_default_string(void)
{
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_OK,
                  logger_ut_log_tick("HAL", (LogLevel)99, 2U, "unknown enum", 0));
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_logger_internal_wrapper_function_smoke(void)
{
    ASSERT_STATUS(STATUS_OK, logger_ut_set_level_from_string("INFO"));
    ASSERT_EQ(LOG_LEVEL_INFO, logger_ut_get_level());
    ASSERT_STATUS(STATUS_OK, logger_ut_log_event("INFO", "smoke"));
    return 1;
}

int32_t register_logger_internal_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"logger_int_event_io", test_logger_internal_log_event_io_error},
        {"logger_int_tick_io", test_logger_internal_log_tick_io_error},
        {"logger_int_unknown_tick", test_logger_internal_unknown_tick_level_uses_default_string},
        {"logger_int_wrapper_smoke", test_logger_internal_wrapper_function_smoke}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
