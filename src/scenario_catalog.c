#include <string.h>

#include "requirements.h"
#include "scenario_catalog.h"
#include "scenario_profiles.h"

typedef struct
{
    const char *name_token;
    uint32_t test_index;
} NamedScenarioAlias;

static const TestCase TEST_REGISTRY[MAX_TESTS] = {
    {"normal_operation", REQ_ENG_003, scenario_normal_operation, ENGINE_OK},
    {"overheat_lt_persistence", REQ_ENG_001, scenario_overheat_short_duration, ENGINE_OK},
    {"overheat_ge_persistence", REQ_ENG_001, scenario_overheat_persistent, ENGINE_SHUTDOWN},
    {"oil_low_ge_persistence", REQ_ENG_002, scenario_oil_pressure_persistent, ENGINE_SHUTDOWN},
    {"combined_warning_persistence", REQ_ENG_003, scenario_combined_warning_persistent, ENGINE_WARNING},
    {"cold_start", REQ_ENG_003, scenario_cold_start_warmup_and_ramp, ENGINE_OK},
    {"high_load", REQ_ENG_003, scenario_high_load_warning_then_recovery, ENGINE_WARNING},
    {"oil_drain", REQ_ENG_002, scenario_oil_pressure_gradual_drain, ENGINE_SHUTDOWN},
    {"thermal_runaway", REQ_ENG_001, scenario_thermal_runaway_with_load_surge, ENGINE_SHUTDOWN},
    {"intermittent_oil", REQ_ENG_002, scenario_intermittent_oil_then_combined_fault, ENGINE_WARNING}};

static const NamedScenarioAlias NAMED_SCENARIO_ALIASES[] = {
    {"normal", 0U},
    {"overheat", 2U},
    {"pressure_failure", 3U},
    {"cold_start", 5U},
    {"high_load", 6U},
    {"oil_drain", 7U},
    {"thermal_runaway", 8U},
    {"intermittent_oil", 9U}};

_Static_assert((sizeof(TEST_REGISTRY) / sizeof(TEST_REGISTRY[0])) == MAX_TESTS,
               "TEST_REGISTRY size must match MAX_TESTS");

const TestCase *scenario_catalog_tests(void)
{
    return TEST_REGISTRY;
}

uint32_t scenario_catalog_count(void)
{
    return (uint32_t)(sizeof(TEST_REGISTRY) / sizeof(TEST_REGISTRY[0]));
}

const TestCase *scenario_catalog_find_named(const char *name)
{
    uint32_t index;
    uint32_t alias_count;

    if (name == (const char *)0)
    {
        return (const TestCase *)0;
    }

    alias_count = (uint32_t)(sizeof(NAMED_SCENARIO_ALIASES) / sizeof(NAMED_SCENARIO_ALIASES[0]));
    for (index = 0U; index < alias_count; ++index)
    {
        if (strncmp(name, NAMED_SCENARIO_ALIASES[index].name_token, MAX_SCENARIO_NAME_LEN) == 0)
        {
            return &TEST_REGISTRY[NAMED_SCENARIO_ALIASES[index].test_index];
        }
    }

    return (const TestCase *)0;
}
