#include <math.h>

#include "control.h"
#include "engine.h"
#include "test_harness.h"

static int32_t g_control_ut_fail_transition = 0;

static StatusCode control_ut_engine_transition_mode(EngineState *engine, EngineStateMode target_mode)
{
    if (g_control_ut_fail_transition != 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (engine == (EngineState *)0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    engine->mode = target_mode;
    return STATUS_OK;
}

#define engine_transition_mode control_ut_engine_transition_mode
#define evaluate_engine control_ut_evaluate_engine
#define compute_control_output control_ut_compute_control_output
#define control_get_default_calibration control_ut_get_default_calibration
#define control_get_active_calibration control_ut_get_active_calibration
#define control_configure_calibration control_ut_configure_calibration
#define control_reset_calibration control_ut_reset_calibration
#include "../../src/domain/control.c"
#undef control_reset_calibration
#undef control_configure_calibration
#undef control_get_active_calibration
#undef control_get_default_calibration
#undef compute_control_output
#undef evaluate_engine
#undef engine_transition_mode

static int32_t test_control_internal_mode_to_result_null(void)
{
    ASSERT_EQ(ENGINE_ERROR, mode_to_result_code((const EngineState *)0));
    return 1;
}

static int32_t test_control_internal_update_fault_counter_null(void)
{
    update_fault_counter(1, (uint32_t *)0);
    return 1;
}

static int32_t test_control_internal_transition_null_engine(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  transition_engine_mode_by_result((EngineState *)0, ENGINE_OK));
    return 1;
}

static int32_t test_control_internal_init_to_starting_transition_failure(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    engine.mode = ENGINE_STATE_INIT;
    engine.is_running = 1;

    g_control_ut_fail_transition = 1;
    ASSERT_STATUS(STATUS_INTERNAL_ERROR,
                  transition_engine_mode_by_result(&engine, ENGINE_OK));
    g_control_ut_fail_transition = 0;
    return 1;
}

static int32_t test_control_internal_init_to_starting_transition_success(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    engine.mode = ENGINE_STATE_INIT;
    engine.is_running = 1;

    g_control_ut_fail_transition = 0;
    ASSERT_STATUS(STATUS_OK,
                  transition_engine_mode_by_result(&engine, ENGINE_OK));
    ASSERT_EQ(ENGINE_STATE_STARTING, engine.mode);
    return 1;
}

static int32_t test_control_internal_running_to_warning_transition_failure(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;

    g_control_ut_fail_transition = 1;
    ASSERT_STATUS(STATUS_INTERNAL_ERROR,
                  transition_engine_mode_by_result(&engine, ENGINE_WARNING));
    g_control_ut_fail_transition = 0;
    return 1;
}

static int32_t test_control_internal_warning_to_shutdown_transition_failure(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    engine.mode = ENGINE_STATE_WARNING;
    engine.is_running = 1;

    g_control_ut_fail_transition = 1;
    ASSERT_STATUS(STATUS_INTERNAL_ERROR,
                  transition_engine_mode_by_result(&engine, ENGINE_SHUTDOWN));
    g_control_ut_fail_transition = 0;
    return 1;
}

static int32_t test_control_internal_evaluate_propagates_transition_failure(void)
{
    EngineState engine;
    int32_t result = ENGINE_ERROR;

    ASSERT_STATUS(STATUS_OK, control_ut_reset_calibration());
    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    engine.mode = ENGINE_STATE_INIT;
    engine.is_running = 1;
    engine.temperature = 25.0f;
    engine.oil_pressure = 3.0f;

    g_control_ut_fail_transition = 1;
    ASSERT_STATUS(STATUS_INTERNAL_ERROR,
                  control_ut_evaluate_engine(&engine, &result));
    g_control_ut_fail_transition = 0;
    return 1;
}

static int32_t test_control_internal_calibration_nan_rejected(void)
{
    ControlCalibration calibration;

    ASSERT_STATUS(STATUS_OK, control_ut_reset_calibration());
    ASSERT_STATUS(STATUS_OK, control_ut_get_default_calibration(&calibration));
    calibration.temperature_limit = NAN;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  control_ut_configure_calibration(&calibration));
    return 1;
}

static int32_t test_control_internal_wrapper_function_smoke(void)
{
    EngineState engine;
    float output = 0.0f;
    ControlCalibration calibration;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    engine.rpm = 2000.0f;
    engine.temperature = 80.0f;
    engine.oil_pressure = 3.0f;

    ASSERT_STATUS(STATUS_OK, control_ut_compute_control_output(&engine, &output));
    ASSERT_TRUE(output >= 0.0f);
    ASSERT_TRUE(output <= 100.0f);

    ASSERT_STATUS(STATUS_OK, control_ut_get_active_calibration(&calibration));
    ASSERT_TRUE(calibration.temperature_limit > 0.0f);
    return 1;
}

int32_t register_control_internal_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"control_int_mode_null", test_control_internal_mode_to_result_null},
        {"control_int_fault_counter_null", test_control_internal_update_fault_counter_null},
        {"control_int_transition_null", test_control_internal_transition_null_engine},
        {"control_int_init_start_fail", test_control_internal_init_to_starting_transition_failure},
        {"control_int_init_start_ok", test_control_internal_init_to_starting_transition_success},
        {"control_int_run_warn_fail", test_control_internal_running_to_warning_transition_failure},
        {"control_int_warn_sd_fail", test_control_internal_warning_to_shutdown_transition_failure},
        {"control_int_eval_transition_fail", test_control_internal_evaluate_propagates_transition_failure},
        {"control_int_cal_nan", test_control_internal_calibration_nan_rejected},
        {"control_int_wrapper_smoke", test_control_internal_wrapper_function_smoke}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
