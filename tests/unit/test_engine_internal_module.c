#include "engine.h"
#include "test_harness.h"

#define engine_get_default_physics engine_ut_get_default_physics
#define engine_get_active_physics engine_ut_get_active_physics
#define engine_configure_physics engine_ut_configure_physics
#define engine_reset_physics engine_ut_reset_physics
#define engine_transition_mode engine_ut_transition_mode
#define engine_get_mode_string engine_ut_get_mode_string
#define engine_init engine_ut_init
#define engine_reset engine_ut_reset
#define engine_start engine_ut_start
#define engine_update engine_ut_update
#include "../../src/domain/engine.c"
#undef engine_update
#undef engine_start
#undef engine_reset
#undef engine_init
#undef engine_get_mode_string
#undef engine_transition_mode
#undef engine_reset_physics
#undef engine_configure_physics
#undef engine_get_active_physics
#undef engine_get_default_physics

static int32_t test_engine_internal_physics_valid_null(void)
{
    ASSERT_EQ(0, physics_valid((const EnginePhysicsConfig *)0));
    return 1;
}

static int32_t test_engine_internal_wrapper_function_smoke(void)
{
    EnginePhysicsConfig config;
    EngineState engine;
    const char *mode = (const char *)0;

    ASSERT_STATUS(STATUS_OK, engine_ut_get_default_physics(&config));
    ASSERT_STATUS(STATUS_OK, engine_ut_get_active_physics(&config));
    ASSERT_STATUS(STATUS_OK, engine_ut_reset_physics());

    ASSERT_STATUS(STATUS_OK, engine_ut_get_default_physics(&config));
    ASSERT_STATUS(STATUS_OK, engine_ut_configure_physics(&config));
    ASSERT_STATUS(STATUS_OK, engine_ut_reset_physics());

    ASSERT_STATUS(STATUS_OK, engine_ut_init(&engine));
    ASSERT_STATUS(STATUS_OK, engine_ut_get_mode_string(&engine, &mode));
    ASSERT_TRUE(mode != (const char *)0);

    ASSERT_STATUS(STATUS_OK, engine_ut_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_STATUS(STATUS_OK, engine_ut_reset(&engine));
    ASSERT_STATUS(STATUS_OK, engine_ut_start(&engine));
    ASSERT_STATUS(STATUS_OK, engine_ut_update(&engine));
    return 1;
}

int32_t register_engine_internal_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"engine_int_physics_null", test_engine_internal_physics_valid_null},
        {"engine_int_wrapper_smoke", test_engine_internal_wrapper_function_smoke}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
