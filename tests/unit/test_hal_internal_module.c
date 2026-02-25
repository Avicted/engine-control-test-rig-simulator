#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "hal.h"
#include "test_harness.h"

static int32_t g_hal_ut_fail_printf = 0;

static int hal_ut_printf(const char *format, ...)
{
    int result;
    va_list args;

    if (g_hal_ut_fail_printf != 0)
    {
        g_hal_ut_fail_printf = 0;
        return -1;
    }

    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);
    return result;
}

StatusCode hal_ut_expected_dlc_for_id(uint32_t id, uint8_t *dlc_out);
StatusCode hal_ut_frame_age_record(uint32_t id, uint32_t tick);

#define printf hal_ut_printf
#define hal_init hal_ut_init
#define hal_shutdown hal_ut_shutdown
#define hal_ingest_sensor_frame hal_ut_ingest_sensor_frame
#define hal_read_sensors hal_ut_read_sensors
#define hal_encode_sensor_frame hal_ut_encode_sensor_frame
#define hal_apply_sensors hal_ut_apply_sensors
#define hal_receive_bus hal_ut_receive_bus
#define hal_transmit_bus hal_ut_transmit_bus
#define hal_write_actuators hal_ut_write_actuators
#define hal_get_last_error hal_ut_get_last_error
#define hal_watchdog_configure hal_ut_watchdog_configure
#define hal_watchdog_kick hal_ut_watchdog_kick
#define hal_watchdog_check hal_ut_watchdog_check
#define hal_submit_redundant_temp hal_ut_submit_redundant_temp
#define hal_vote_sensors hal_ut_vote_sensors
#define hal_frame_id_is_known hal_ut_frame_id_is_known
#define hal_expected_dlc_for_id hal_ut_expected_dlc_for_id
#define hal_frame_age_record hal_ut_frame_age_record
#define hal_frame_is_stale hal_ut_frame_is_stale
#include "../../src/platform/hal.c"
#undef hal_frame_is_stale
#undef hal_frame_age_record
#undef hal_expected_dlc_for_id
#undef hal_frame_id_is_known
#undef hal_vote_sensors
#undef hal_submit_redundant_temp
#undef hal_watchdog_check
#undef hal_watchdog_kick
#undef hal_watchdog_configure
#undef hal_get_last_error
#undef hal_write_actuators
#undef hal_transmit_bus
#undef hal_receive_bus
#undef hal_apply_sensors
#undef hal_encode_sensor_frame
#undef hal_read_sensors
#undef hal_ingest_sensor_frame
#undef hal_shutdown
#undef hal_init
#undef printf

static StatusCode hal_int_fail_expected_dlc(uint32_t id, uint8_t *dlc_out)
{
    (void)id;
    (void)dlc_out;
    return STATUS_INVALID_ARGUMENT;
}

static int32_t test_hal_internal_static_null_guards(void)
{
    HAL_FrameQueue queue;
    HAL_TxFrameQueue tx_queue;
    HAL_Frame frame = {0};

    queue_reset((HAL_FrameQueue *)0);
    tx_queue_reset((HAL_TxFrameQueue *)0);

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  queue_push((HAL_FrameQueue *)0, &frame, HAL_MAX_RX_FRAMES));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  queue_pop((HAL_FrameQueue *)0, &frame, HAL_MAX_RX_FRAMES));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  tx_queue_push((HAL_TxFrameQueue *)0, &frame));

    queue_reset(&queue);
    tx_queue_reset(&tx_queue);

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  queue_push(&queue, (const HAL_Frame *)0, HAL_MAX_RX_FRAMES));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  queue_pop(&queue, (HAL_Frame *)0, HAL_MAX_RX_FRAMES));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  tx_queue_push(&tx_queue, (const HAL_Frame *)0));

    ASSERT_EQ(0U, frame_checksum((const HAL_Frame *)0));
    return 1;
}

static int32_t test_hal_internal_write_actuator_io_error(void)
{
    HAL_ControlFrame control;

    control.control_output = 50.0f;
    control.emit_control_line = 1;

    g_hal_ut_fail_printf = 1;
    ASSERT_STATUS(STATUS_IO_ERROR, hal_ut_write_actuators(&control));
    return 1;
}

static int32_t test_hal_internal_redundant_temp_invalid_and_negative_diff(void)
{
    float voted = 0.0f;

    ASSERT_STATUS(STATUS_OK, hal_ut_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  hal_ut_submit_redundant_temp(NAN, 7U));

    ASSERT_STATUS(STATUS_OK, hal_ut_submit_redundant_temp(90.0f, 8U));
    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  hal_ut_vote_sensors(80.0f, &voted));

    ASSERT_STATUS(STATUS_OK, hal_ut_shutdown());
    return 1;
}

static int32_t test_hal_internal_validate_sensor_frame_defensive_paths(void)
{
    HAL_SensorFrame sensor;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  validate_sensor_frame((const HAL_SensorFrame *)0));

    sensor.rpm = NAN;
    sensor.temperature = 80.0f;
    sensor.oil_pressure = 3.0f;
    sensor.is_running = 1;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  validate_sensor_frame(&sensor));
    return 1;
}

static int32_t test_hal_internal_decode_sensor_frame_defensive_paths(void)
{
    HAL_Frame frame = {0};
    HAL_SensorFrame decoded;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  decode_sensor_frame((const HAL_Frame *)0, &decoded));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  decode_sensor_frame(&frame, (HAL_SensorFrame *)0));

    frame.id = HAL_SENSOR_ERROR_FRAME_ID;
    frame.dlc = 8U;
    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  decode_sensor_frame(&frame, &decoded));
    return 1;
}

static int32_t test_hal_internal_supported_transport_frame_defensive_paths(void)
{
    HAL_Frame frame = {0};

    ASSERT_EQ(0, is_supported_sensor_transport_frame((const HAL_Frame *)0));

    frame.id = 0x999U;
    frame.dlc = 8U;
    ASSERT_EQ(0, is_supported_sensor_transport_frame(&frame));
    return 1;
}

static int32_t test_hal_internal_wrapper_function_smoke(void)
{
    HAL_SensorFrame sensor;
    HAL_SensorFrame decoded;
    HAL_Frame frame;
    HAL_Frame bus_frame;
    HAL_ControlFrame control;
    EngineState engine;
    ErrorInfo error;
    int32_t stale = 0;

    ASSERT_STATUS(STATUS_OK, hal_ut_init());

    sensor.rpm = 1500.0f;
    sensor.temperature = 70.0f;
    sensor.oil_pressure = 3.0f;
    sensor.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_ut_encode_sensor_frame(&sensor, &frame));
    ASSERT_STATUS(STATUS_OK, hal_ut_ingest_sensor_frame(&frame, 1U));
    ASSERT_STATUS(STATUS_OK, hal_ut_read_sensors(1U, &decoded));

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    ASSERT_STATUS(STATUS_OK, hal_ut_apply_sensors(&decoded, &engine));

    bus_frame.id = 0x220U;
    bus_frame.dlc = 2U;
    bus_frame.data[0] = 1U;
    bus_frame.data[1] = 2U;
    bus_frame.data[2] = 0U;
    bus_frame.data[3] = 0U;
    bus_frame.data[4] = 0U;
    bus_frame.data[5] = 0U;
    bus_frame.data[6] = 0U;
    bus_frame.data[7] = 0U;
    ASSERT_STATUS(STATUS_OK, hal_ut_receive_bus(&bus_frame));
    ASSERT_STATUS(STATUS_OK, hal_ut_transmit_bus(&bus_frame));

    control.control_output = 55.0f;
    control.emit_control_line = 0;
    ASSERT_STATUS(STATUS_OK, hal_ut_write_actuators(&control));

    ASSERT_STATUS(STATUS_OK, hal_ut_get_last_error(&error));
    ASSERT_TRUE(error.code >= STATUS_OK);

    ASSERT_STATUS(STATUS_OK, hal_ut_watchdog_configure(3U));
    ASSERT_STATUS(STATUS_OK, hal_ut_watchdog_kick());
    ASSERT_STATUS(STATUS_OK, hal_ut_watchdog_check(2U));

    ASSERT_EQ(1, hal_ut_frame_id_is_known(HAL_SENSOR_FRAME_ID));
    ASSERT_STATUS(STATUS_OK, hal_ut_expected_dlc_for_id(HAL_SENSOR_FRAME_ID, &bus_frame.dlc));

    ASSERT_STATUS(STATUS_OK, hal_ut_frame_age_record(HAL_SENSOR_FRAME_ID, 10U));
    ASSERT_STATUS(STATUS_OK, hal_ut_frame_is_stale(HAL_SENSOR_FRAME_ID, 10U, &stale));
    ASSERT_EQ(0, stale);

    ASSERT_STATUS(STATUS_OK, hal_ut_shutdown());
    return 1;
}

static int32_t test_hal_internal_supported_transport_expected_dlc_failure(void)
{
    HAL_Frame frame = {0};

    frame.id = HAL_SENSOR_FRAME_ID;
    frame.dlc = 8U;

    g_expected_dlc_impl = hal_int_fail_expected_dlc;
    ASSERT_EQ(0, is_supported_sensor_transport_frame(&frame));
    g_expected_dlc_impl = hal_ut_expected_dlc_for_id;
    return 1;
}

int32_t register_hal_internal_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"hal_int_static_null", test_hal_internal_static_null_guards},
        {"hal_int_actuator_io", test_hal_internal_write_actuator_io_error},
        {"hal_int_vote_edges", test_hal_internal_redundant_temp_invalid_and_negative_diff},
        {"hal_int_validate_paths", test_hal_internal_validate_sensor_frame_defensive_paths},
        {"hal_int_decode_paths", test_hal_internal_decode_sensor_frame_defensive_paths},
        {"hal_int_supported_paths", test_hal_internal_supported_transport_frame_defensive_paths},
        {"hal_int_wrapper_smoke", test_hal_internal_wrapper_function_smoke},
        {"hal_int_expected_dlc_fail", test_hal_internal_supported_transport_expected_dlc_failure}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
