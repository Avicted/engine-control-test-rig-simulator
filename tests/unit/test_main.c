#include <stdio.h>

#include "test_harness.h"

static int32_t run_group(const UnitTestCase *tests, uint32_t count, uint32_t *pass_count)
{
    uint32_t index;

    if ((tests == (const UnitTestCase *)0) || (pass_count == (uint32_t *)0))
    {
        return 0;
    }

    for (index = 0U; index < count; ++index)
    {
        if (tests[index].function() != 0)
        {
            *pass_count += 1U;
            (void)printf("[PASS] %s\n", tests[index].name);
        }
        else
        {
            (void)printf("[FAIL] %s\n", tests[index].name);
        }
    }

    return 1;
}

int main(void)
{
    const UnitTestCase *group_tests;
    uint32_t group_count;
    uint32_t pass_count = 0U;
    uint32_t total_count = 0U;

    if (register_control_tests(&group_tests, &group_count) == 0)
    {
        return 1;
    }
    total_count += group_count;
    if (run_group(group_tests, group_count, &pass_count) == 0)
    {
        return 1;
    }

    if (register_state_machine_tests(&group_tests, &group_count) == 0)
    {
        return 1;
    }
    total_count += group_count;
    if (run_group(group_tests, group_count, &pass_count) == 0)
    {
        return 1;
    }

    if (register_hal_decode_tests(&group_tests, &group_count) == 0)
    {
        return 1;
    }
    total_count += group_count;
    if (run_group(group_tests, group_count, &pass_count) == 0)
    {
        return 1;
    }

    if (register_script_parser_tests(&group_tests, &group_count) == 0)
    {
        return 1;
    }
    total_count += group_count;
    if (run_group(group_tests, group_count, &pass_count) == 0)
    {
        return 1;
    }

    if (register_logger_tests(&group_tests, &group_count) == 0)
    {
        return 1;
    }
    total_count += group_count;
    if (run_group(group_tests, group_count, &pass_count) == 0)
    {
        return 1;
    }

    (void)printf("Summary: %u/%u tests passed\n", pass_count, total_count);
    return (pass_count == total_count) ? 0 : 1;
}
