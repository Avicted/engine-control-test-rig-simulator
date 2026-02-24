#include <assert.h>
#include <stdint.h>

#include "control.h"
#include "engine.h"
#include "hal.h"

static void test_shutdown_requires_persistence_and_escalation(void)
{
    EngineState engine;
    int32_t index;
    int result = ENGINE_ERROR;

    assert(engine_reset(&engine) == STATUS_OK);
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;

    for (index = 0; index < 3; ++index)
    {
        engine.rpm = 2500.0f;
        engine.temperature = 100.0f;
        engine.oil_pressure = 3.1f;
        assert(evaluate_engine(&engine, &result) == STATUS_OK);
    }

    assert(engine.mode == ENGINE_STATE_WARNING);
    assert(result == ENGINE_WARNING);

    engine.rpm = 2500.0f;
    engine.temperature = 100.0f;
    engine.oil_pressure = 3.1f;
    assert(evaluate_engine(&engine, &result) == STATUS_OK);
    assert(engine.mode == ENGINE_STATE_SHUTDOWN);
    assert(result == ENGINE_SHUTDOWN);
}

static void test_control_output_clamps_to_0_100(void)
{
    EngineState engine;
    float output = 0.0f;

    assert(engine_reset(&engine) == STATUS_OK);

    engine.rpm = 9000.0f;
    engine.temperature = 25.0f;
    engine.oil_pressure = 3.0f;
    assert(compute_control_output(&engine, &output) == STATUS_OK);
    assert(output <= 100.0f);

    engine.rpm = 500.0f;
    engine.temperature = 190.0f;
    engine.oil_pressure = 0.1f;
    assert(compute_control_output(&engine, &output) == STATUS_OK);
    assert(output >= 0.0f);
}

static void test_hal_rejects_non_finite_or_out_of_range_sensor_values(void)
{
    HAL_SensorFrame frame;

    frame.rpm = 12000.0f;
    frame.temperature = 80.0f;
    frame.oil_pressure = 3.0f;
    frame.is_running = 1;
    assert(hal_read_sensors(&frame) == STATUS_INVALID_ARGUMENT);

    frame.rpm = 2500.0f;
    frame.temperature = 300.0f;
    frame.oil_pressure = 3.0f;
    frame.is_running = 1;
    assert(hal_read_sensors(&frame) == STATUS_INVALID_ARGUMENT);

    frame.rpm = 2500.0f;
    frame.temperature = 80.0f;
    frame.oil_pressure = 2.9f;
    frame.is_running = 1;
    assert(hal_read_sensors(&frame) == STATUS_OK);
}

int main(void)
{
    test_shutdown_requires_persistence_and_escalation();
    test_control_output_clamps_to_0_100();
    test_hal_rejects_non_finite_or_out_of_range_sensor_values();
    return 0;
}
