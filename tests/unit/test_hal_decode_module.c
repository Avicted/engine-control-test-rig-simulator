#include "hal.h"
#include "test_harness.h"

static int32_t test_valid_frame_decodes_to_sensor_struct(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;
    HAL_SensorFrame decoded;

    source.rpm = 2500.0f;
    source.temperature = 80.0f;
    source.oil_pressure = 3.1f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));
    ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&frame, 1U));
    ASSERT_STATUS(STATUS_OK, hal_read_sensors(1U, &decoded));
    ASSERT_EQ(1, decoded.is_running);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_invalid_id_rejected(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;
    HAL_SensorFrame decoded;

    source.rpm = 2200.0f;
    source.temperature = 75.0f;
    source.oil_pressure = 3.0f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));
    frame.id = 0x101U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_ingest_sensor_frame(&frame, 1U));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_read_sensors(1U, &decoded));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_wrong_dlc_rejected(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;

    source.rpm = 2200.0f;
    source.temperature = 75.0f;
    source.oil_pressure = 3.0f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));
    frame.dlc = 7U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_ingest_sensor_frame(&frame, 2U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_corrupt_payload_rejected(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;
    HAL_SensorFrame decoded;

    source.rpm = 2200.0f;
    source.temperature = 75.0f;
    source.oil_pressure = 3.0f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));
    frame.data[7] ^= 0xFFU;
    ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&frame, 1U));
    ASSERT_STATUS(STATUS_PARSE_ERROR, hal_read_sensors(1U, &decoded));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_timeout_is_reported_deterministically(void)
{
    HAL_SensorFrame decoded;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_read_sensors(1U, &decoded));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_read_sensors(2U, &decoded));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_read_sensors(3U, &decoded));
    ASSERT_STATUS(STATUS_TIMEOUT, hal_read_sensors(4U, &decoded));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_fifo_order_is_deterministic(void)
{
    HAL_SensorFrame first_source;
    HAL_SensorFrame second_source;
    HAL_Frame first_frame;
    HAL_Frame second_frame;
    HAL_SensorFrame decoded;

    first_source.rpm = 1500.0f;
    first_source.temperature = 65.0f;
    first_source.oil_pressure = 2.9f;
    first_source.is_running = 1;

    second_source.rpm = 3200.0f;
    second_source.temperature = 92.0f;
    second_source.oil_pressure = 2.1f;
    second_source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&first_source, &first_frame));
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&second_source, &second_frame));
    ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&first_frame, 1U));
    ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&second_frame, 2U));

    ASSERT_STATUS(STATUS_OK, hal_read_sensors(2U, &decoded));
    ASSERT_EQ(1, decoded.is_running);
    ASSERT_EQ((int32_t)first_source.rpm, (int32_t)decoded.rpm);

    ASSERT_STATUS(STATUS_OK, hal_read_sensors(3U, &decoded));
    ASSERT_EQ(1, decoded.is_running);
    ASSERT_EQ((int32_t)second_source.rpm, (int32_t)decoded.rpm);

    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_sensor_error_frame_returns_parse_error(void)
{
    HAL_Frame error_frame;
    HAL_SensorFrame decoded;

    error_frame.id = HAL_SENSOR_ERROR_FRAME_ID;
    error_frame.dlc = 1U;
    error_frame.data[0] = 0x01U;
    error_frame.data[1] = 0U;
    error_frame.data[2] = 0U;
    error_frame.data[3] = 0U;
    error_frame.data[4] = 0U;
    error_frame.data[5] = 0U;
    error_frame.data[6] = 0U;
    error_frame.data[7] = 0U;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&error_frame, 5U));
    ASSERT_STATUS(STATUS_PARSE_ERROR, hal_read_sensors(5U, &decoded));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_timeout_after_last_valid_sample(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;
    HAL_SensorFrame decoded;

    source.rpm = 2100.0f;
    source.temperature = 78.0f;
    source.oil_pressure = 3.3f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));
    ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&frame, 1U));
    ASSERT_STATUS(STATUS_OK, hal_read_sensors(1U, &decoded));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_read_sensors(4U, &decoded));
    ASSERT_STATUS(STATUS_TIMEOUT, hal_read_sensors(5U, &decoded));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

int32_t register_hal_decode_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"hal_valid_decode", test_valid_frame_decodes_to_sensor_struct},
        {"hal_invalid_id", test_invalid_id_rejected},
        {"hal_wrong_dlc", test_wrong_dlc_rejected},
        {"hal_corrupt_payload", test_corrupt_payload_rejected},
        {"hal_timeout", test_timeout_is_reported_deterministically},
        {"hal_fifo_order", test_fifo_order_is_deterministic},
        {"hal_error_frame", test_sensor_error_frame_returns_parse_error},
        {"hal_timeout_after_last_sample", test_timeout_after_last_valid_sample}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
