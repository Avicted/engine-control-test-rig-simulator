#include <string.h>

#include "engine.h"
#include "test_harness.h"

static int32_t test_illegal_transition_rejected(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    return 1;
}

static int32_t test_legal_transition_sequence(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_INIT));
    return 1;
}

static int32_t test_init_starting_running_path(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_EQ(ENGINE_STATE_INIT, engine.mode);
    ASSERT_STATUS(STATUS_OK, engine_start(&engine));
    ASSERT_EQ(ENGINE_STATE_STARTING, engine.mode);
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    ASSERT_EQ(ENGINE_STATE_RUNNING, engine.mode);
    return 1;
}

/* --- exhaustive illegal transition tests --- */

static int32_t test_init_cannot_go_to_running(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    return 1;
}

static int32_t test_init_cannot_go_to_warning(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    return 1;
}

static int32_t test_init_cannot_go_to_shutdown(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    return 1;
}

static int32_t test_starting_cannot_go_to_warning(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    return 1;
}

static int32_t test_starting_can_emergency_shutdown(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    ASSERT_EQ(ENGINE_STATE_SHUTDOWN, engine.mode);
    return 1;
}

static int32_t test_starting_cannot_go_to_init(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_INIT));
    return 1;
}

static int32_t test_running_cannot_go_to_init(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_INIT));
    return 1;
}

static int32_t test_running_can_emergency_shutdown(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    ASSERT_EQ(ENGINE_STATE_SHUTDOWN, engine.mode);
    return 1;
}

static int32_t test_warning_cannot_go_to_init(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_INIT));
    return 1;
}

static int32_t test_warning_cannot_go_to_running(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    return 1;
}

static int32_t test_shutdown_cannot_go_to_running(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    return 1;
}

/* --- self-transition is legal --- */

static int32_t test_self_transition_is_legal(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_INIT));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    return 1;
}

/* --- engine_update tests --- */

static int32_t test_engine_update_shutdown_clears_state(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_start(&engine));
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    /* Now RUNNING, set values */
    engine.rpm = 3000.0f;
    engine.is_running = 1;
    /* Force to SHUTDOWN via transitions */
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    ASSERT_EQ(0, engine.is_running);
    ASSERT_TRUE(engine.rpm < 1.0f);
    return 1;
}

static int32_t test_engine_update_running_ramps_rpm(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_start(&engine));
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    ASSERT_EQ(ENGINE_STATE_RUNNING, engine.mode);
    /* After one update in RUNNING, RPM should have increased */
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    ASSERT_TRUE(engine.rpm > 0.0f);
    return 1;
}

static int32_t test_engine_update_running_ramps_temperature(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_start(&engine));
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    /* engine.temperature starts at 25.0 */
    float initial_temp = engine.temperature;
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    ASSERT_TRUE(engine.temperature > initial_temp);
    return 1;
}

static int32_t test_engine_update_not_running_noop(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    /* INIT state, is_running = 0 */
    float initial_rpm = engine.rpm;
    ASSERT_STATUS(STATUS_OK, engine_update(&engine));
    ASSERT_TRUE(engine.rpm == initial_rpm);
    return 1;
}

static int32_t test_engine_update_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_update((EngineState *)0));
    return 1;
}

/* --- engine_start tests --- */

static int32_t test_engine_start_from_non_init_rejected(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_start(&engine));
    /* Now STARTING - cannot start again */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_start(&engine));
    return 1;
}

static int32_t test_engine_start_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_start((EngineState *)0));
    return 1;
}

/* --- engine_get_mode_string tests --- */

static int32_t test_engine_get_mode_string_init(void)
{
    EngineState engine;
    const char *mode_str = (const char *)0;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_get_mode_string(&engine, &mode_str));
    ASSERT_TRUE(mode_str != (const char *)0);
    ASSERT_EQ(0, strcmp(mode_str, "INIT"));
    return 1;
}

static int32_t test_engine_get_mode_string_running(void)
{
    EngineState engine;
    const char *mode_str = (const char *)0;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_STATUS(STATUS_OK, engine_get_mode_string(&engine, &mode_str));
    ASSERT_TRUE(mode_str != (const char *)0);
    ASSERT_EQ(0, strcmp(mode_str, "RUNNING"));
    return 1;
}

static int32_t test_engine_get_mode_string_null_engine(void)
{
    const char *mode_str = (const char *)0;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_get_mode_string((const EngineState *)0, &mode_str));
    return 1;
}

static int32_t test_engine_get_mode_string_null_string(void)
{
    EngineState engine;

    ASSERT_STATUS(STATUS_OK, engine_reset(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_get_mode_string(&engine, (const char **)0));
    return 1;
}

/* --- engine_init / engine_reset tests --- */

static int32_t test_engine_init_sets_defaults(void)
{
    EngineState engine;

    (void)memset(&engine, 0xFF, sizeof(engine));
    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    ASSERT_TRUE(engine.rpm < 1.0f);
    ASSERT_TRUE(engine.temperature > 24.0f);
    ASSERT_TRUE(engine.temperature < 26.0f);
    ASSERT_EQ(0, engine.is_running);
    ASSERT_EQ(ENGINE_STATE_INIT, engine.mode);
    ASSERT_EQ(0U, engine.fault_counters[ENGINE_FAULT_TEMP]);
    ASSERT_EQ(0U, engine.fault_counters[ENGINE_FAULT_OIL_PRESSURE]);
    ASSERT_EQ(0U, engine.fault_counters[ENGINE_FAULT_RPM_TEMP_COMBINED]);
    return 1;
}

static int32_t test_engine_init_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_init((EngineState *)0));
    return 1;
}

static int32_t test_engine_reset_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_reset((EngineState *)0));
    return 1;
}

static int32_t test_engine_transition_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_transition_mode((EngineState *)0, ENGINE_STATE_INIT));
    return 1;
}

/* --- Engine physics configuration tests --- */

static int32_t test_engine_get_default_physics(void)
{
    EnginePhysicsConfig config;
    ASSERT_STATUS(STATUS_OK, engine_get_default_physics(&config));
    ASSERT_TRUE(config.target_rpm == ENGINE_DEFAULT_TARGET_RPM);
    ASSERT_TRUE(config.target_temperature == ENGINE_DEFAULT_TARGET_TEMP);
    ASSERT_TRUE(config.target_oil_pressure == ENGINE_DEFAULT_TARGET_OIL);
    ASSERT_TRUE(config.rpm_ramp_rate == ENGINE_DEFAULT_RPM_RAMP_RATE);
    ASSERT_TRUE(config.temp_ramp_rate == ENGINE_DEFAULT_TEMP_RAMP_RATE);
    ASSERT_TRUE(config.oil_decay_rate == ENGINE_DEFAULT_OIL_DECAY_RATE);
    return 1;
}

static int32_t test_engine_configure_physics(void)
{
    EnginePhysicsConfig config;
    (void)engine_reset_physics();
    config.target_rpm = 2000.0f;
    config.target_temperature = 80.0f;
    config.target_oil_pressure = 4.0f;
    config.rpm_ramp_rate = 100.0f;
    config.temp_ramp_rate = 0.5f;
    config.oil_decay_rate = 0.02f;
    ASSERT_STATUS(STATUS_OK, engine_configure_physics(&config));
    (void)engine_reset_physics();
    return 1;
}

static int32_t test_engine_configure_physics_null(void)
{
    (void)engine_reset_physics();
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_configure_physics((EnginePhysicsConfig *)0));
    return 1;
}

static int32_t test_engine_configure_physics_twice(void)
{
    EnginePhysicsConfig config;
    (void)engine_reset_physics();
    (void)engine_get_default_physics(&config);
    ASSERT_STATUS(STATUS_OK, engine_configure_physics(&config));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_configure_physics(&config));
    (void)engine_reset_physics();
    return 1;
}

static int32_t test_engine_configure_physics_invalid(void)
{
    EnginePhysicsConfig config;
    (void)engine_reset_physics();
    (void)engine_get_default_physics(&config);
    config.target_rpm = -1.0f; /* invalid */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_configure_physics(&config));
    (void)engine_reset_physics();
    return 1;
}

static int32_t test_engine_reset_physics(void)
{
    EnginePhysicsConfig config;
    EnginePhysicsConfig active;
    (void)engine_reset_physics();
    config.target_rpm = 5000.0f;
    config.target_temperature = 120.0f;
    config.target_oil_pressure = 5.0f;
    config.rpm_ramp_rate = 200.0f;
    config.temp_ramp_rate = 1.0f;
    config.oil_decay_rate = 0.05f;
    ASSERT_STATUS(STATUS_OK, engine_configure_physics(&config));
    ASSERT_STATUS(STATUS_OK, engine_reset_physics());
    ASSERT_STATUS(STATUS_OK, engine_get_active_physics(&active));
    ASSERT_TRUE(active.target_rpm == ENGINE_DEFAULT_TARGET_RPM);
    return 1;
}

static int32_t test_engine_get_active_physics(void)
{
    EnginePhysicsConfig active;
    (void)engine_reset_physics();
    ASSERT_STATUS(STATUS_OK, engine_get_active_physics(&active));
    ASSERT_TRUE(active.target_rpm == ENGINE_DEFAULT_TARGET_RPM);
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, engine_get_active_physics((EnginePhysicsConfig *)0));
    return 1;
}

static int32_t test_engine_physics_affects_update(void)
{
    EngineState engine;
    EnginePhysicsConfig config;
    (void)engine_reset_physics();
    config.target_rpm = 500.0f;
    config.target_temperature = 50.0f;
    config.target_oil_pressure = 2.0f;
    config.rpm_ramp_rate = 500.0f; /* reaches target in 1 tick */
    config.temp_ramp_rate = 50.0f; /* reaches target in 1 tick */
    config.oil_decay_rate = 10.0f; /* reaches target in 1 tick */
    ASSERT_STATUS(STATUS_OK, engine_configure_physics(&config));

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    ASSERT_STATUS(STATUS_OK, engine_start(&engine));
    ASSERT_STATUS(STATUS_OK, engine_update(&engine)); /* STARTING -> RUNNING */
    ASSERT_STATUS(STATUS_OK, engine_update(&engine)); /* ramp with custom physics */
    ASSERT_TRUE(engine.rpm == 500.0f);
    ASSERT_TRUE(engine.temperature == 50.0f);

    (void)engine_reset_physics();
    return 1;
}

int32_t register_state_machine_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"state_illegal_transition", test_illegal_transition_rejected},
        {"state_legal_transitions", test_legal_transition_sequence},
        {"state_init_starting_running", test_init_starting_running_path},
        /* exhaustive illegal transitions */
        {"state_init_no_running", test_init_cannot_go_to_running},
        {"state_init_no_warning", test_init_cannot_go_to_warning},
        {"state_init_no_shutdown", test_init_cannot_go_to_shutdown},
        {"state_starting_no_warning", test_starting_cannot_go_to_warning},
        {"state_starting_emerg_sd", test_starting_can_emergency_shutdown},
        {"state_starting_no_init", test_starting_cannot_go_to_init},
        {"state_running_no_init", test_running_cannot_go_to_init},
        {"state_running_emerg_sd", test_running_can_emergency_shutdown},
        {"state_warning_no_init", test_warning_cannot_go_to_init},
        {"state_warning_no_running", test_warning_cannot_go_to_running},
        {"state_shutdown_no_running", test_shutdown_cannot_go_to_running},
        {"state_self_transition_ok", test_self_transition_is_legal},
        /* engine_update tests */
        {"state_update_shutdown_clear", test_engine_update_shutdown_clears_state},
        {"state_update_running_rpm", test_engine_update_running_ramps_rpm},
        {"state_update_running_temp", test_engine_update_running_ramps_temperature},
        {"state_update_not_running", test_engine_update_not_running_noop},
        {"state_update_null", test_engine_update_null},
        /* engine_start tests */
        {"state_start_non_init", test_engine_start_from_non_init_rejected},
        {"state_start_null", test_engine_start_null},
        /* engine_get_mode_string tests */
        {"state_mode_str_init", test_engine_get_mode_string_init},
        {"state_mode_str_running", test_engine_get_mode_string_running},
        {"state_mode_str_null_engine", test_engine_get_mode_string_null_engine},
        {"state_mode_str_null_string", test_engine_get_mode_string_null_string},
        /* engine_init / engine_reset null tests */
        {"state_init_defaults", test_engine_init_sets_defaults},
        {"state_init_null", test_engine_init_null},
        {"state_reset_null", test_engine_reset_null},
        {"state_transition_null", test_engine_transition_null},
        /* engine physics config tests */
        {"physics_default", test_engine_get_default_physics},
        {"physics_configure", test_engine_configure_physics},
        {"physics_configure_null", test_engine_configure_physics_null},
        {"physics_configure_twice", test_engine_configure_physics_twice},
        {"physics_configure_invalid", test_engine_configure_physics_invalid},
        {"physics_reset", test_engine_reset_physics},
        {"physics_active", test_engine_get_active_physics},
        {"physics_affects_update", test_engine_physics_affects_update}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
