/**
 * @file scenario_contract.h
 * @brief Contract type for deterministic integration test scenarios.
 */

#ifndef SCENARIO_CONTRACT_H
#define SCENARIO_CONTRACT_H

#include <stdint.h>

#include "engine.h"

/* Forward declaration — full definition in scenario_profiles.h */
struct TickReport;

/** @brief Maximum number of test cases in a single test suite. */
#define MAX_TESTS 10
/** @brief Maximum length of a scenario name string (including NUL). */
#define MAX_SCENARIO_NAME_LEN 32

_Static_assert(MAX_TESTS > 0, "MAX_TESTS must be non-zero");
_Static_assert(MAX_TESTS <= 64, "MAX_TESTS exceeds validated bound");
_Static_assert(MAX_SCENARIO_NAME_LEN >= 16, "MAX_SCENARIO_NAME_LEN too small for supported scenario names");

/**
 * @brief Describes one integration test case with traceability.
 *
 * Each test case references a requirement ID and a deterministic
 * scenario callback that operates on bounded ticks with no dynamic allocation.
 */
typedef struct
{
    const char *test_name;      /**< Human-readable test name. */
    const char *requirement_id; /**< Traced requirement (e.g., "REQ-ENG-001"). */
    /**
     * @brief Scenario callback.
     * @note Contract: deterministic, no dynamic allocation, bounded ticks.
     */
    int32_t (*scenario_func)(EngineState *, int32_t, int32_t, int32_t, struct TickReport *, uint32_t, uint32_t *);
    int32_t expected_result; /**< Expected return value from scenario_func. */
} TestCase;

#endif
