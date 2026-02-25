#include <string.h>

#include "control.h"
#include "engine.h"
#include "test_harness.h"

/* --- evaluate_engine tests --- */

static int32_t test_single_tick_threshold_no_shutdown(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
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

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
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

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
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

static int32_t test_temperature_persistence_boundary_transition(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;
    engine.oil_pressure = 3.0f;

    engine.temperature = 100.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(1U, engine.fault_counters[ENGINE_FAULT_TEMP]);
    ASSERT_EQ(ENGINE_OK, result);

    engine.temperature = 100.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS - 1U, engine.fault_counters[ENGINE_FAULT_TEMP]);
    ASSERT_EQ(ENGINE_OK, result);

    engine.temperature = 100.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS, engine.fault_counters[ENGINE_FAULT_TEMP]);
    ASSERT_EQ(ENGINE_WARNING, result);

    engine.temperature = 100.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(CONTROL_DEFAULT_TEMP_PERSISTENCE_TICKS + 1U, engine.fault_counters[ENGINE_FAULT_TEMP]);
    ASSERT_EQ(ENGINE_SHUTDOWN, result);
    return 1;
}

static int32_t test_oil_persistence_boundary_transition(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;
    engine.temperature = 85.0f;

    engine.oil_pressure = 2.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(ENGINE_OK, result);

    engine.oil_pressure = 2.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS - 1U, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);
    ASSERT_EQ(ENGINE_OK, result);

    engine.oil_pressure = 2.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);
    ASSERT_EQ(ENGINE_WARNING, result);

    engine.oil_pressure = 2.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(CONTROL_DEFAULT_OIL_PERSISTENCE_TICKS + 1U, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);
    ASSERT_EQ(ENGINE_SHUTDOWN, result);
    return 1;
}

/* --- evaluate_engine null-argument tests --- */

static int32_t test_evaluate_engine_null_engine(void)
{
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, evaluate_engine((EngineState *)0, &result));
    return 1;
}

static int32_t test_evaluate_engine_null_result(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, evaluate_engine(&engine, (int32_t *)0));
    return 1;
}

/* --- oil fault counter reset on recovery --- */

static int32_t test_oil_persistence_reset_on_recovery(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;
    engine.temperature = 85.0f;

    /* Accumulate 2 ticks of low oil */
    engine.oil_pressure = 2.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(1U, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);

    engine.oil_pressure = 2.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(2U, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);

    /* Oil recovers - counter must reset */
    engine.oil_pressure = 3.5f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(0U, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);
    ASSERT_EQ(ENGINE_OK, result);
    return 1;
}

/* --- combined warning counter reset --- */

static int32_t test_combined_warning_reset_on_recovery(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;
    engine.oil_pressure = 3.0f;

    /* One tick of combined condition */
    engine.rpm = 3600.0f;
    engine.temperature = 86.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(1U, engine.fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED]);

    /* RPM drops below threshold - counter resets */
    engine.rpm = 1000.0f;
    engine.temperature = 86.0f;
    ASSERT_STATUS(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(0U, engine.fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED]);
    ASSERT_EQ(ENGINE_OK, result);
    return 1;
}

/* --- compute_control_output tests --- */

static int32_t test_compute_control_output_normal(void)
{
    EngineState engine;
    float output = -1.0f;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;
    engine.rpm = 3000.0f;
    engine.temperature = 85.0f;
    engine.oil_pressure = 3.0f;

    ASSERT_STATUS(STATUS_OK, compute_control_output(&engine, &output));
    ASSERT_TRUE(output >= 0.0f);
    ASSERT_TRUE(output <= 100.0f);
    return 1;
}

static int32_t test_compute_control_output_clamp_high(void)
{
    EngineState engine;
    float output = -1.0f;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.rpm = 10000.0f;
    engine.temperature = 25.0f;
    engine.oil_pressure = 10.0f;

    ASSERT_STATUS(STATUS_OK, compute_control_output(&engine, &output));
    ASSERT_TRUE(output <= 100.0f);
    return 1;
}

static int32_t test_compute_control_output_clamp_low(void)
{
    EngineState engine;
    float output = 999.0f;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    engine.rpm = 0.0f;
    engine.temperature = 200.0f;
    engine.oil_pressure = 0.1f;

    ASSERT_STATUS(STATUS_OK, compute_control_output(&engine, &output));
    ASSERT_TRUE(output >= 0.0f);
    return 1;
}

static int32_t test_compute_control_output_null_engine(void)
{
    float output = 0.0f;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, compute_control_output((const EngineState *)0, &output));
    return 1;
}

static int32_t test_compute_control_output_null_output(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, compute_control_output(&engine, (float *)0));
    return 1;
}

/* --- calibration API tests --- */

static int32_t test_configure_calibration_valid(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.temperature_limit = 100.0f;
    ASSERT_STATUS(STATUS_OK, control_configure_calibration(&cal));
    /* Cleanup for subsequent tests */
    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    return 1;
}

static int32_t test_configure_calibration_double_rejected(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    ASSERT_STATUS(STATUS_OK, control_configure_calibration(&cal));
    /* Second configure must be rejected */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    return 1;
}

static int32_t test_configure_calibration_invalid_temp_limit(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.temperature_limit = 0.0f; /* invalid: <= 0 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_configure_calibration_invalid_oil_limit(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.oil_pressure_limit = 0.05f; /* invalid: < 0.1 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_configure_calibration_zero_persistence(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.temp_persistence_ticks = 0U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_configure_calibration_null(void)
{
    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration((const ControlCalibration *)0));
    return 1;
}

static int32_t test_get_default_calibration(void)
{
    ControlCalibration cal;

    (void)memset(&cal, 0, sizeof(cal));
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    ASSERT_TRUE(cal.temperature_limit > 0.0f);
    ASSERT_TRUE(cal.oil_pressure_limit > 0.0f);
    ASSERT_TRUE(cal.temp_persistence_ticks > 0U);
    ASSERT_TRUE(cal.oil_persistence_ticks > 0U);
    ASSERT_TRUE(cal.combined_warning_persistence_ticks > 0U);
    return 1;
}

static int32_t test_get_default_calibration_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_get_default_calibration((ControlCalibration *)0));
    return 1;
}

static int32_t test_get_active_calibration(void)
{
    ControlCalibration cal;
    ControlCalibration active;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.temperature_limit = 110.0f;
    ASSERT_STATUS(STATUS_OK, control_configure_calibration(&cal));
    ASSERT_STATUS(STATUS_OK, control_get_active_calibration(&active));
    ASSERT_TRUE(active.temperature_limit > 109.0f);
    ASSERT_TRUE(active.temperature_limit < 111.0f);
    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    return 1;
}

static int32_t test_get_active_calibration_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_get_active_calibration((ControlCalibration *)0));
    return 1;
}

static int32_t test_reset_calibration_allows_reconfigure(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    ASSERT_STATUS(STATUS_OK, control_configure_calibration(&cal));
    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    /* After reset, should accept a new configuration */
    cal.temperature_limit = 120.0f;
    ASSERT_STATUS(STATUS_OK, control_configure_calibration(&cal));
    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    return 1;
}

/* --- calibration boundary validation tests --- */

static int32_t test_calibration_temp_limit_above_max(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.temperature_limit = 201.0f; /* > 200 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_oil_limit_above_max(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.oil_pressure_limit = 10.1f; /* > 10 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_rpm_threshold_below_min(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.high_rpm_warning_threshold = 50.0f; /* < 100 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_rpm_threshold_above_max(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.high_rpm_warning_threshold = 10001.0f; /* > 10000 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_high_temp_threshold_below_min(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.high_temp_warning_threshold = -51.0f; /* < -50 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_high_temp_threshold_above_max(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.high_temp_warning_threshold = 201.0f; /* > 200 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_zero_oil_persistence(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.oil_persistence_ticks = 0U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

static int32_t test_calibration_zero_combined_persistence(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_get_default_calibration(&cal));
    cal.combined_warning_persistence_ticks = 0U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, control_configure_calibration(&cal));
    return 1;
}

int32_t register_control_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        /* evaluate_engine core tests */
        {"control_single_tick_threshold", test_single_tick_threshold_no_shutdown},
        {"control_persistence_reset", test_persistence_counter_increment_and_reset},
        {"control_combined_warning", test_combined_warning_escalation},
        {"control_temp_persistence_boundary", test_temperature_persistence_boundary_transition},
        {"control_oil_persistence_boundary", test_oil_persistence_boundary_transition},
        /* evaluate_engine defensive tests */
        {"control_eval_null_engine", test_evaluate_engine_null_engine},
        {"control_eval_null_result", test_evaluate_engine_null_result},
        /* fault counter recovery tests */
        {"control_oil_recovery_reset", test_oil_persistence_reset_on_recovery},
        {"control_combined_recovery_reset", test_combined_warning_reset_on_recovery},
        /* compute_control_output tests */
        {"control_output_normal", test_compute_control_output_normal},
        {"control_output_clamp_high", test_compute_control_output_clamp_high},
        {"control_output_clamp_low", test_compute_control_output_clamp_low},
        {"control_output_null_engine", test_compute_control_output_null_engine},
        {"control_output_null_output", test_compute_control_output_null_output},
        /* calibration API tests */
        {"control_configure_valid", test_configure_calibration_valid},
        {"control_configure_double", test_configure_calibration_double_rejected},
        {"control_configure_invalid_temp", test_configure_calibration_invalid_temp_limit},
        {"control_configure_invalid_oil", test_configure_calibration_invalid_oil_limit},
        {"control_configure_zero_persist", test_configure_calibration_zero_persistence},
        {"control_configure_null", test_configure_calibration_null},
        {"control_default_cal", test_get_default_calibration},
        {"control_default_cal_null", test_get_default_calibration_null},
        {"control_active_cal", test_get_active_calibration},
        {"control_active_cal_null", test_get_active_calibration_null},
        {"control_reset_reconfigure", test_reset_calibration_allows_reconfigure},
        /* calibration boundary validation tests */
        {"control_cal_temp_above_max", test_calibration_temp_limit_above_max},
        {"control_cal_oil_above_max", test_calibration_oil_limit_above_max},
        {"control_cal_rpm_below_min", test_calibration_rpm_threshold_below_min},
        {"control_cal_rpm_above_max", test_calibration_rpm_threshold_above_max},
        {"control_cal_htemp_below_min", test_calibration_high_temp_threshold_below_min},
        {"control_cal_htemp_above_max", test_calibration_high_temp_threshold_above_max},
        {"control_cal_zero_oil_persist", test_calibration_zero_oil_persistence},
        {"control_cal_zero_comb_persist", test_calibration_zero_combined_persistence}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
