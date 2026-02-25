#include "control.h"
#include "engine.h"
#include "test_harness.h"

static int32_t test_single_tick_threshold_no_shutdown(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;
    engine.temperature = 100.0f;
    engine.oil_pressure = 3.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(ENGINE_OK, result);
    ASSERT_EQ(1U, engine.fault_counters[ENGINE_FAULT_TEMP]);
    return 1;
}

static int32_t test_persistence_counter_increment_and_reset(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;

    engine.temperature = 100.0f;
    engine.oil_pressure = 3.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(1U, engine.fault_counters[ENGINE_FAULT_TEMP]);

    engine.temperature = 85.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(0U, engine.fault_counters[ENGINE_FAULT_TEMP]);
    return 1;
}

static int32_t test_combined_warning_escalation(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;

    engine.rpm = 3600.0f;
    engine.temperature = 86.0f;
    engine.oil_pressure = 3.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(ENGINE_OK, result);

    engine.rpm = 3600.0f;
    engine.temperature = 87.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(ENGINE_WARNING, result);
    ASSERT_TRUE(engine.fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED] >= 2U);
    return 1;
}

int32_t register_control_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"control_single_tick_threshold", test_single_tick_threshold_no_shutdown},
        {"control_persistence_reset", test_persistence_counter_increment_and_reset},
        {"control_combined_warning", test_combined_warning_escalation}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
