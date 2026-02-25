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

int32_t register_state_machine_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"state_illegal_transition", test_illegal_transition_rejected},
        {"state_legal_transitions", test_legal_transition_sequence},
        {"state_init_starting_running", test_init_starting_running_path}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
