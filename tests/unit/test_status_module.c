#include <string.h>

#include "status.h"
#include "test_harness.h"

static int32_t test_status_code_to_string_values(void)
{
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_OK), "STATUS_OK"));
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_INVALID_ARGUMENT), "STATUS_INVALID_ARGUMENT"));
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_PARSE_ERROR), "STATUS_PARSE_ERROR"));
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_IO_ERROR), "STATUS_IO_ERROR"));
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_TIMEOUT), "STATUS_TIMEOUT"));
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_BUFFER_OVERFLOW), "STATUS_BUFFER_OVERFLOW"));
    ASSERT_EQ(0, strcmp(status_code_to_string(STATUS_INTERNAL_ERROR), "STATUS_INTERNAL_ERROR"));
    ASSERT_EQ(0, strcmp(status_code_to_string((StatusCode)99), "STATUS_UNKNOWN"));
    return 1;
}

static int32_t test_severity_to_string_values(void)
{
    ASSERT_EQ(0, strcmp(severity_to_string(SEVERITY_INFO), "INFO"));
    ASSERT_EQ(0, strcmp(severity_to_string(SEVERITY_WARNING), "WARNING"));
    ASSERT_EQ(0, strcmp(severity_to_string(SEVERITY_ERROR), "ERROR"));
    ASSERT_EQ(0, strcmp(severity_to_string(SEVERITY_FATAL), "FATAL"));
    ASSERT_EQ(0, strcmp(severity_to_string((Severity)99), "UNKNOWN"));
    return 1;
}

static int32_t test_recoverability_to_string_values(void)
{
    ASSERT_EQ(0, strcmp(recoverability_to_string(RECOVERABILITY_RECOVERABLE), "RECOVERABLE"));
    ASSERT_EQ(0, strcmp(recoverability_to_string(RECOVERABILITY_NON_RECOVERABLE), "NON_RECOVERABLE"));
    ASSERT_EQ(0, strcmp(recoverability_to_string((Recoverability)99), "UNKNOWN"));
    return 1;
}

static int32_t test_status_default_recoverability_mapping(void)
{
    ASSERT_EQ(RECOVERABILITY_NON_RECOVERABLE, status_code_default_recoverability(STATUS_IO_ERROR));
    ASSERT_EQ(RECOVERABILITY_NON_RECOVERABLE, status_code_default_recoverability(STATUS_INTERNAL_ERROR));
    ASSERT_EQ(RECOVERABILITY_RECOVERABLE, status_code_default_recoverability(STATUS_OK));
    ASSERT_EQ(RECOVERABILITY_RECOVERABLE, status_code_default_recoverability(STATUS_PARSE_ERROR));
    ASSERT_EQ(RECOVERABILITY_RECOVERABLE, status_code_default_recoverability(STATUS_TIMEOUT));
    ASSERT_EQ(RECOVERABILITY_RECOVERABLE, status_code_default_recoverability((StatusCode)99));
    return 1;
}

int32_t register_status_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"status_code_to_string", test_status_code_to_string_values},
        {"status_severity_to_string", test_severity_to_string_values},
        {"status_recoverability_to_string", test_recoverability_to_string_values},
        {"status_default_recoverability", test_status_default_recoverability_mapping}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
