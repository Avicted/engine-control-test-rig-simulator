#ifndef SCENARIO_CONTRACT_H
#define SCENARIO_CONTRACT_H

#include <stdint.h>

#include "engine.h"

#define MAX_TESTS 10
#define MAX_SCENARIO_NAME_LEN 32

_Static_assert(MAX_TESTS > 0, "MAX_TESTS must be non-zero");
_Static_assert(MAX_TESTS <= 64, "MAX_TESTS exceeds validated bound");
_Static_assert(MAX_SCENARIO_NAME_LEN >= 16, "MAX_SCENARIO_NAME_LEN too small for supported scenario names");

typedef struct
{
    const char *test_name;
    const char *requirement_id;
    /* Scenario callback contract: deterministic, no dynamic allocation, bounded ticks. */
    int32_t (*scenario_func)(EngineState *, int32_t, int32_t, int32_t, void *, uint32_t, uint32_t *);
    int32_t expected_result;
} TestCase;

#endif