#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "control.h"
#include "engine.h"
#include "hal.h"
#include "script_parser.h"

typedef int32_t (*TestFn)(void);

typedef struct
{
    const char *name;
    TestFn function;
} TestCase;

#define ASSERT_TRUE(expr) \
    do                    \
    {                     \
        if (!(expr))      \
        {                 \
            return 0;     \
        }                 \
    } while (0)

#define ASSERT_EQ(expected, actual) \
    do                              \
    {                               \
        if ((expected) != (actual)) \
        {                           \
            return 0;               \
        }                           \
    } while (0)

static int32_t test_shutdown_requires_persistence_and_escalation(void)
{
    EngineState engine;
    int32_t index;
    int32_t result = ENGINE_ERROR;

    ASSERT_EQ(STATUS_OK, engine_reset(&engine));
    engine.mode = ENGINE_STATE_RUNNING;
    engine.is_running = 1;

    for (index = 0; index < 3; ++index)
    {
        engine.rpm = 2500.0f;
        engine.temperature = 100.0f;
        engine.oil_pressure = 3.1f;
        ASSERT_EQ(STATUS_OK, evaluate_engine(&engine, &result));
    }

    ASSERT_EQ(ENGINE_STATE_WARNING, engine.mode);
    ASSERT_EQ(ENGINE_WARNING, result);

    engine.rpm = 2500.0f;
    engine.temperature = 100.0f;
    engine.oil_pressure = 3.1f;
    ASSERT_EQ(STATUS_OK, evaluate_engine(&engine, &result));
    ASSERT_EQ(ENGINE_STATE_SHUTDOWN, engine.mode);
    ASSERT_EQ(ENGINE_SHUTDOWN, result);
    return 1;
}

static int32_t test_control_output_clamps_to_0_100(void)
{
    EngineState engine;
    float output = 0.0f;

    ASSERT_EQ(STATUS_OK, engine_reset(&engine));

    engine.rpm = 9000.0f;
    engine.temperature = 25.0f;
    engine.oil_pressure = 3.0f;
    ASSERT_EQ(STATUS_OK, compute_control_output(&engine, &output));
    ASSERT_TRUE(output <= 100.0f);

    engine.rpm = 500.0f;
    engine.temperature = 190.0f;
    engine.oil_pressure = 0.1f;
    ASSERT_EQ(STATUS_OK, compute_control_output(&engine, &output));
    ASSERT_TRUE(output >= 0.0f);
    return 1;
}

static int32_t test_state_machine_transition_contract(void)
{
    EngineState engine;

    ASSERT_EQ(STATUS_OK, engine_reset(&engine));
    ASSERT_EQ(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_STARTING));
    ASSERT_EQ(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    ASSERT_EQ(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_WARNING));
    ASSERT_EQ(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_SHUTDOWN));
    ASSERT_EQ(STATUS_OK, engine_transition_mode(&engine, ENGINE_STATE_INIT));
    ASSERT_EQ(STATUS_INVALID_ARGUMENT, engine_transition_mode(&engine, ENGINE_STATE_RUNNING));
    return 1;
}

static int32_t test_hal_rejects_non_finite_or_out_of_range_sensor_values(void)
{
    HAL_SensorFrame frame;
    HAL_Frame encoded_frame;
    HAL_SensorFrame decoded_frame;

    frame.rpm = 12000.0f;
    frame.temperature = 80.0f;
    frame.oil_pressure = 3.0f;
    frame.is_running = 1;
    ASSERT_EQ(STATUS_INVALID_ARGUMENT, hal_encode_sensor_frame(&frame, &encoded_frame));

    frame.rpm = 2500.0f;
    frame.temperature = 300.0f;
    frame.oil_pressure = 3.0f;
    frame.is_running = 1;
    ASSERT_EQ(STATUS_INVALID_ARGUMENT, hal_encode_sensor_frame(&frame, &encoded_frame));

    frame.rpm = 2500.0f;
    frame.temperature = 80.0f;
    frame.oil_pressure = 2.9f;
    frame.is_running = 1;
    ASSERT_EQ(STATUS_OK, hal_init());
    ASSERT_EQ(STATUS_OK, hal_encode_sensor_frame(&frame, &encoded_frame));
    ASSERT_EQ(STATUS_OK, hal_ingest_sensor_frame(&encoded_frame, 1U));
    ASSERT_EQ(STATUS_OK, hal_read_sensors(1U, &decoded_frame));
    ASSERT_EQ(1, decoded_frame.is_running);
    ASSERT_EQ(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_timeout_behavior(void)
{
    HAL_SensorFrame decoded_frame;

    ASSERT_EQ(STATUS_OK, hal_init());
    ASSERT_EQ(STATUS_INVALID_ARGUMENT, hal_read_sensors(1U, &decoded_frame));
    ASSERT_EQ(STATUS_INVALID_ARGUMENT, hal_read_sensors(2U, &decoded_frame));
    ASSERT_EQ(STATUS_INVALID_ARGUMENT, hal_read_sensors(3U, &decoded_frame));
    ASSERT_EQ(STATUS_IO_ERROR, hal_read_sensors(4U, &decoded_frame));
    ASSERT_EQ(STATUS_IO_ERROR, hal_read_sensors(5U, &decoded_frame));
    ASSERT_EQ(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_script_parser_edge_validation(void)
{
    ScriptScenarioData scenario_data;
    char error_message[128];
    const char *invalid_path = "build/unit_invalid_script.txt";
    FILE *file;

    file = fopen(invalid_path, "w");
    ASSERT_TRUE(file != (FILE *)0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2500 TEMP 80 OIL 3.0 RUN 1\n", file) >= 0);
    ASSERT_TRUE(fputs("TICK 1 RPM 2600 TEMP 81 OIL 3.1 RUN 1\n", file) >= 0);
    ASSERT_EQ(0, fclose(file));

    ASSERT_EQ(STATUS_PARSE_ERROR,
              script_parser_parse_file(invalid_path,
                                       &scenario_data,
                                       error_message,
                                       (uint32_t)sizeof(error_message),
                                       0));
    ASSERT_TRUE(strstr(error_message, "non-increasing") != (char *)0);
    return 1;
}

static int32_t run_test_suite(void)
{
    static const TestCase test_cases[] = {
        {"shutdown_persistence", test_shutdown_requires_persistence_and_escalation},
        {"control_output_clamp", test_control_output_clamps_to_0_100},
        {"state_machine_transitions", test_state_machine_transition_contract},
        {"hal_frame_decode", test_hal_rejects_non_finite_or_out_of_range_sensor_values},
        {"hal_timeout", test_hal_timeout_behavior},
        {"script_parser_edges", test_script_parser_edge_validation}};
    uint32_t idx;
    uint32_t pass_count = 0U;

    for (idx = 0U; idx < (uint32_t)(sizeof(test_cases) / sizeof(test_cases[0])); ++idx)
    {
        int32_t passed = test_cases[idx].function();
        if (passed != 0)
        {
            pass_count += 1U;
            (void)printf("[PASS] %s\n", test_cases[idx].name);
        }
        else
        {
            (void)printf("[FAIL] %s\n", test_cases[idx].name);
        }
    }

    (void)printf("Summary: %u/%u tests passed\n",
                 pass_count,
                 (uint32_t)(sizeof(test_cases) / sizeof(test_cases[0])));
    return (pass_count == (uint32_t)(sizeof(test_cases) / sizeof(test_cases[0]))) ? 0 : 1;
}

int main(void)
{
    return run_test_suite();
}
