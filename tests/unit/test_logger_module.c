#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>

#include "reporting/logger.h"
#include "test_harness.h"

/* --- logger_set_level / logger_get_level tests --- */

static int32_t test_logger_set_and_get_level(void)
{
    /*
     * When the CI environment variable is set, logger_set_level()
     * silently promotes LOG_LEVEL_DEBUG to LOG_LEVEL_INFO.
     * Account for that so the test passes in both environments.
     */
    const int32_t in_ci = (getenv("CI") != NULL) ? 1 : 0;
    const LogLevel debug_expected = (in_ci != 0) ? LOG_LEVEL_INFO : LOG_LEVEL_DEBUG;

    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_EQ(debug_expected, logger_get_level());

    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_WARN));
    ASSERT_EQ(LOG_LEVEL_WARN, logger_get_level());

    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_ERROR));
    ASSERT_EQ(LOG_LEVEL_ERROR, logger_get_level());

    /* Restore to INFO for subsequent tests */
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    ASSERT_EQ(LOG_LEVEL_INFO, logger_get_level());
    return 1;
}

static int32_t test_logger_set_level_invalid(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_set_level((LogLevel)-1));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_set_level((LogLevel)99));
    return 1;
}

/* --- logger_set_level_from_string tests --- */

static int32_t test_logger_set_level_from_string_valid(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level_from_string("INFO"));
    ASSERT_EQ(LOG_LEVEL_INFO, logger_get_level());

    ASSERT_STATUS(STATUS_OK, logger_set_level_from_string("WARN"));
    ASSERT_EQ(LOG_LEVEL_WARN, logger_get_level());

    ASSERT_STATUS(STATUS_OK, logger_set_level_from_string("ERROR"));
    ASSERT_EQ(LOG_LEVEL_ERROR, logger_get_level());

    /* Restore */
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_logger_set_level_from_string_invalid(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_set_level_from_string("TRACE"));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_set_level_from_string("info"));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_set_level_from_string(""));
    return 1;
}

static int32_t test_logger_set_level_from_string_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_set_level_from_string((const char *)0));
    return 1;
}

/* --- log_event level filtering tests --- */

static int32_t test_log_event_suppressed_by_level(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_ERROR));
    /* DEBUG message at ERROR level should be suppressed (returns OK, just doesn't emit) */
    ASSERT_STATUS(STATUS_OK, log_event("DEBUG", "this should be suppressed"));
    ASSERT_STATUS(STATUS_OK, log_event("INFO", "this should also be suppressed"));
    ASSERT_STATUS(STATUS_OK, log_event("WARN", "this should also be suppressed"));

    /* Restore */
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_log_event_null_arguments(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, log_event((const char *)0, "message"));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, log_event("INFO", (const char *)0));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

/* --- log_event_with_options tests --- */

static int32_t test_log_event_with_color(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    ASSERT_STATUS(STATUS_OK, log_event_with_options("INFO", "color test", 1));
    ASSERT_STATUS(STATUS_OK, log_event_with_options("WARN", "color test", 1));
    ASSERT_STATUS(STATUS_OK, log_event_with_options("ERROR", "color test", 1));
    return 1;
}

static int32_t test_log_event_without_color(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    ASSERT_STATUS(STATUS_OK, log_event_with_options("INFO", "no color test", 0));
    return 1;
}

/* --- logger_log_tick tests --- */

static int32_t test_logger_log_tick_basic(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    ASSERT_STATUS(STATUS_OK, logger_log_tick("HAL", LOG_LEVEL_INFO, 42U, "sensor read ok", 0));
    return 1;
}

static int32_t test_logger_log_tick_suppressed(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_ERROR));
    /* INFO message should be suppressed at ERROR level */
    ASSERT_STATUS(STATUS_OK, logger_log_tick("HAL", LOG_LEVEL_INFO, 1U, "suppressed", 0));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_logger_log_tick_null_module(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_log_tick((const char *)0, LOG_LEVEL_INFO, 1U, "test", 0));
    return 1;
}

static int32_t test_logger_log_tick_null_message(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, logger_log_tick("HAL", LOG_LEVEL_INFO, 1U, (const char *)0, 0));
    return 1;
}

static int32_t test_logger_log_tick_with_color(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    ASSERT_STATUS(STATUS_OK, logger_log_tick("CTRL", LOG_LEVEL_WARN, 10U, "warning test", 1));
    return 1;
}

static int32_t test_logger_log_tick_level_string_variants(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_OK, logger_log_tick("CTRL", LOG_LEVEL_DEBUG, 7U, "debug path", 1));
    ASSERT_STATUS(STATUS_OK, logger_log_tick("CTRL", LOG_LEVEL_ERROR, 8U, "error path", 1));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_log_event_unknown_level_uses_raw_level(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_OK, log_event_with_options("TRACE", "unknown level still logs", 1));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_log_event_level_buffer_overflow_rejected(void)
{
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_BUFFER_OVERFLOW,
                  log_event_with_options("LEVEL_NAME_EXCEEDING_BUFFER_SIZE_FOR_TEST", "msg", 0));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_log_event_message_buffer_overflow_rejected(void)
{
    char long_message[160];
    uint32_t index;

    for (index = 0U; index < (uint32_t)(sizeof(long_message) - 1U); ++index)
    {
        long_message[index] = 'X';
    }
    long_message[sizeof(long_message) - 1U] = '\0';

    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_BUFFER_OVERFLOW, log_event_with_options("INFO", long_message, 0));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

static int32_t test_logger_log_tick_buffer_overflow_rejected(void)
{
    char long_message[160];
    uint32_t index;

    for (index = 0U; index < (uint32_t)(sizeof(long_message) - 1U); ++index)
    {
        long_message[index] = 'Y';
    }
    long_message[sizeof(long_message) - 1U] = '\0';

    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    ASSERT_STATUS(STATUS_BUFFER_OVERFLOW, logger_log_tick("MODULE", LOG_LEVEL_INFO, 1U, long_message, 0));
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_INFO));
    return 1;
}

/* --- CI environment behavior test --- */

static int32_t test_logger_ci_env_suppresses_debug(void)
{
    const char *original_ci;
    LogLevel level_before;

    /* Save current state */
    level_before = logger_get_level();
    original_ci = getenv("CI");

    /* Set CI=true and try to set DEBUG level */
    (void)setenv("CI", "true", 1);
    ASSERT_STATUS(STATUS_OK, logger_set_level(LOG_LEVEL_DEBUG));
    /* In CI mode, DEBUG should be promoted to INFO */
    ASSERT_EQ(LOG_LEVEL_INFO, logger_get_level());

    /* Restore */
    if (original_ci != (const char *)0)
    {
        (void)setenv("CI", original_ci, 1);
    }
    else
    {
        (void)unsetenv("CI");
    }
    ASSERT_STATUS(STATUS_OK, logger_set_level(level_before));
    return 1;
}

int32_t register_logger_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"logger_set_get_level", test_logger_set_and_get_level},
        {"logger_set_level_invalid", test_logger_set_level_invalid},
        {"logger_from_string_valid", test_logger_set_level_from_string_valid},
        {"logger_from_string_invalid", test_logger_set_level_from_string_invalid},
        {"logger_from_string_null", test_logger_set_level_from_string_null},
        {"logger_event_suppressed", test_log_event_suppressed_by_level},
        {"logger_event_null_args", test_log_event_null_arguments},
        {"logger_event_with_color", test_log_event_with_color},
        {"logger_event_no_color", test_log_event_without_color},
        {"logger_tick_basic", test_logger_log_tick_basic},
        {"logger_tick_suppressed", test_logger_log_tick_suppressed},
        {"logger_tick_null_module", test_logger_log_tick_null_module},
        {"logger_tick_null_message", test_logger_log_tick_null_message},
        {"logger_tick_with_color", test_logger_log_tick_with_color},
        {"logger_tick_level_variants", test_logger_log_tick_level_string_variants},
        {"logger_unknown_level_raw", test_log_event_unknown_level_uses_raw_level},
        {"logger_level_overflow", test_log_event_level_buffer_overflow_rejected},
        {"logger_event_overflow", test_log_event_message_buffer_overflow_rejected},
        {"logger_tick_overflow", test_logger_log_tick_buffer_overflow_rejected},
        {"logger_ci_suppresses_debug", test_logger_ci_env_suppresses_debug}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
