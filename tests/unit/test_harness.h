#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdint.h>

#include "status.h"

typedef int32_t (*UnitTestFn)(void);

typedef struct
{
    const char *name;
    UnitTestFn function;
} UnitTestCase;

#define ASSERT_TRUE(expr) \
    do                    \
    {                     \
        if (!(expr))      \
        {                 \
            return 0;     \
        }                 \
    } while (0)

#define ASSERT_EQ(expected, actual) \
    do                              \
    {                               \
        if ((expected) != (actual)) \
        {                           \
            return 0;               \
        }                           \
    } while (0)

#define ASSERT_STATUS(expected, actual) ASSERT_EQ((expected), (actual))

int32_t register_control_tests(const UnitTestCase **tests_out, uint32_t *count_out);
int32_t register_state_machine_tests(const UnitTestCase **tests_out, uint32_t *count_out);
int32_t register_hal_decode_tests(const UnitTestCase **tests_out, uint32_t *count_out);
int32_t register_script_parser_tests(const UnitTestCase **tests_out, uint32_t *count_out);

#endif
