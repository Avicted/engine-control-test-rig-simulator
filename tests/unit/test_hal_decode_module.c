#include <string.h>

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

/* --- hal_apply_sensors tests --- */

static int32_t test_hal_apply_sensors_updates_engine(void)
{
    HAL_SensorFrame sensor;
    EngineState engine;

    sensor.rpm = 3500.0f;
    sensor.temperature = 92.0f;
    sensor.oil_pressure = 2.8f;
    sensor.is_running = 1;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    ASSERT_STATUS(STATUS_OK, hal_apply_sensors(&sensor, &engine));
    ASSERT_EQ((int32_t)3500, (int32_t)engine.rpm);
    ASSERT_EQ(1, engine.is_running);
    return 1;
}

static int32_t test_hal_apply_sensors_null_engine(void)
{
    HAL_SensorFrame sensor;

    sensor.rpm = 1000.0f;
    sensor.temperature = 50.0f;
    sensor.oil_pressure = 3.0f;
    sensor.is_running = 1;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_apply_sensors(&sensor, (EngineState *)0));
    return 1;
}

static int32_t test_hal_apply_sensors_invalid_frame(void)
{
    HAL_SensorFrame sensor;
    EngineState engine;

    sensor.rpm = -1.0f; /* invalid RPM */
    sensor.temperature = 50.0f;
    sensor.oil_pressure = 3.0f;
    sensor.is_running = 1;

    ASSERT_STATUS(STATUS_OK, engine_init(&engine));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_apply_sensors(&sensor, &engine));
    return 1;
}

/* --- hal_receive_bus / hal_transmit_bus tests --- */

static int32_t test_hal_receive_bus_valid(void)
{
    HAL_Frame frame;

    ASSERT_STATUS(STATUS_OK, hal_init());
    (void)memset(&frame, 0, sizeof(frame));
    frame.id = 0x200U;
    frame.dlc = 4U;
    ASSERT_STATUS(STATUS_OK, hal_receive_bus(&frame));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_receive_bus_null(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_receive_bus((const HAL_Frame *)0));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_receive_bus_invalid_dlc(void)
{
    HAL_Frame frame;

    ASSERT_STATUS(STATUS_OK, hal_init());
    (void)memset(&frame, 0, sizeof(frame));
    frame.id = 0x200U;
    frame.dlc = 9U; /* > 8 */
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_receive_bus(&frame));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_transmit_bus_valid(void)
{
    HAL_Frame frame;

    ASSERT_STATUS(STATUS_OK, hal_init());
    (void)memset(&frame, 0, sizeof(frame));
    frame.id = 0x300U;
    frame.dlc = 8U;
    ASSERT_STATUS(STATUS_OK, hal_transmit_bus(&frame));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_transmit_bus_null(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_transmit_bus((const HAL_Frame *)0));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_transmit_bus_invalid_dlc(void)
{
    HAL_Frame frame;

    ASSERT_STATUS(STATUS_OK, hal_init());
    (void)memset(&frame, 0, sizeof(frame));
    frame.id = 0x300U;
    frame.dlc = 9U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_transmit_bus(&frame));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* --- hal_write_actuators tests --- */

static int32_t test_hal_write_actuators_no_emit(void)
{
    HAL_ControlFrame ctrl;

    ctrl.control_output = 55.0f;
    ctrl.emit_control_line = 0;
    ASSERT_STATUS(STATUS_OK, hal_write_actuators(&ctrl));
    return 1;
}

static int32_t test_hal_write_actuators_emit(void)
{
    HAL_ControlFrame ctrl;

    ctrl.control_output = 75.5f;
    ctrl.emit_control_line = 1;
    ASSERT_STATUS(STATUS_OK, hal_write_actuators(&ctrl));
    return 1;
}

static int32_t test_hal_write_actuators_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_write_actuators((const HAL_ControlFrame *)0));
    return 1;
}

/* --- hal_get_last_error tests --- */

static int32_t test_hal_get_last_error_after_init(void)
{
    ErrorInfo error;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_get_last_error(&error));
    ASSERT_EQ(STATUS_OK, error.code);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_hal_get_last_error_null(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_get_last_error((ErrorInfo *)0));
    return 1;
}

/* --- hal_encode_sensor_frame null/invalid tests --- */

static int32_t test_hal_encode_null_sensor(void)
{
    HAL_Frame frame;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_encode_sensor_frame((const HAL_SensorFrame *)0, &frame));
    return 1;
}

static int32_t test_hal_encode_null_frame(void)
{
    HAL_SensorFrame sensor;

    sensor.rpm = 1000.0f;
    sensor.temperature = 50.0f;
    sensor.oil_pressure = 3.0f;
    sensor.is_running = 1;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_encode_sensor_frame(&sensor, (HAL_Frame *)0));
    return 1;
}

static int32_t test_hal_encode_invalid_sensor_values(void)
{
    HAL_SensorFrame sensor;
    HAL_Frame frame;

    sensor.rpm = 11000.0f; /* > HAL_MAX_RPM */
    sensor.temperature = 50.0f;
    sensor.oil_pressure = 3.0f;
    sensor.is_running = 1;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_encode_sensor_frame(&sensor, &frame));
    return 1;
}

/* --- hal_ingest_sensor_frame null test --- */

static int32_t test_hal_ingest_null(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_ingest_sensor_frame((const HAL_Frame *)0, 1U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* --- hal_read_sensors null test --- */

static int32_t test_hal_read_sensors_null(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_read_sensors(1U, (HAL_SensorFrame *)0));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* --- bus queue overflow test --- */

static int32_t test_hal_receive_bus_overflow(void)
{
    HAL_Frame frame;
    uint32_t idx;

    ASSERT_STATUS(STATUS_OK, hal_init());
    (void)memset(&frame, 0, sizeof(frame));
    frame.id = 0x200U;
    frame.dlc = 4U;

    for (idx = 0U; idx < HAL_MAX_RX_FRAMES; ++idx)
    {
        ASSERT_STATUS(STATUS_OK, hal_receive_bus(&frame));
    }
    /* 33rd push should overflow */
    ASSERT_STATUS(STATUS_BUFFER_OVERFLOW, hal_receive_bus(&frame));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* --- Watchdog timer tests --- */

static int32_t test_watchdog_disabled_by_default(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    /* Watchdog disabled, check should always return OK */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(1U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(2U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(100U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_watchdog_configure_and_kick(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_watchdog_configure(5U));
    /* Check 4 times without kick - should be fine */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(1U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(2U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(3U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(4U));
    /* Kick resets counter */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_kick());
    /* Another 4 ticks after kick - still fine */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(5U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(6U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(7U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(8U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_watchdog_expires(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_watchdog_configure(3U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(1U)); /* counter=1 */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(2U)); /* counter=2 */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(3U)); /* counter=3 */
    /* 4th check without kick - counter=4 > timeout(3) = expired */
    ASSERT_STATUS(STATUS_TIMEOUT, hal_watchdog_check(4U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_watchdog_configure_too_large(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_watchdog_configure(HAL_WATCHDOG_MAX_TICKS + 1U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_watchdog_disable_after_enable(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_watchdog_configure(3U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(1U));
    /* Disable by configuring to 0 */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_configure(0U));
    /* Should never expire now */
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(2U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(3U));
    ASSERT_STATUS(STATUS_OK, hal_watchdog_check(100U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* --- Sensor voting tests --- */

static int32_t test_vote_single_channel(void)
{
    float voted;
    ASSERT_STATUS(STATUS_OK, hal_init());
    /* No redundant reading submitted - single-channel passthrough */
    ASSERT_STATUS(STATUS_OK, hal_vote_sensors(85.0f, &voted));
    ASSERT_TRUE(voted == 85.0f);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_vote_dual_agree(void)
{
    float voted;
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_submit_redundant_temp(84.0f, 1U));
    ASSERT_STATUS(STATUS_OK, hal_vote_sensors(86.0f, &voted));
    /* Average of 84 and 86 = 85 */
    ASSERT_TRUE(voted == 85.0f);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_vote_dual_disagree(void)
{
    float voted;
    ASSERT_STATUS(STATUS_OK, hal_init());
    /* Submit reading 20 deg off - exceeds 5 deg tolerance */
    ASSERT_STATUS(STATUS_OK, hal_submit_redundant_temp(60.0f, 1U));
    ASSERT_STATUS(STATUS_PARSE_ERROR, hal_vote_sensors(85.0f, &voted));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_vote_null_output(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_vote_sensors(85.0f, (float *)0));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_vote_clears_after_use(void)
{
    float voted;
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_submit_redundant_temp(84.0f, 1U));
    ASSERT_STATUS(STATUS_OK, hal_vote_sensors(86.0f, &voted));
    /* Redundant reading consumed - next call is single-channel */
    ASSERT_STATUS(STATUS_OK, hal_vote_sensors(90.0f, &voted));
    ASSERT_TRUE(voted == 90.0f);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* ------------------------------------------------------------------
 * Frame ID Registry tests (Section 1.1)
 * ------------------------------------------------------------------ */

static int32_t test_frame_id_known_valid(void)
{
    ASSERT_EQ(1, hal_frame_id_is_known(HAL_SENSOR_FRAME_ID));
    ASSERT_EQ(1, hal_frame_id_is_known(HAL_SENSOR_ERROR_FRAME_ID));
    ASSERT_EQ(1, hal_frame_id_is_known(HAL_SENSOR_TEMP_B_FRAME_ID));
    return 1;
}

static int32_t test_frame_id_unknown_rejected(void)
{
    ASSERT_EQ(0, hal_frame_id_is_known(0x101U));
    ASSERT_EQ(0, hal_frame_id_is_known(0x000U));
    ASSERT_EQ(0, hal_frame_id_is_known(0xFFFFU));
    return 1;
}

/* ------------------------------------------------------------------
 * DLC-per-ID enforcement tests (Section 1.2)
 * ------------------------------------------------------------------ */

static int32_t test_expected_dlc_sensor(void)
{
    uint8_t dlc = 0U;
    ASSERT_STATUS(STATUS_OK, hal_expected_dlc_for_id(HAL_SENSOR_FRAME_ID, &dlc));
    ASSERT_EQ(8U, dlc);
    return 1;
}

static int32_t test_expected_dlc_error_frame(void)
{
    uint8_t dlc = 0U;
    ASSERT_STATUS(STATUS_OK, hal_expected_dlc_for_id(HAL_SENSOR_ERROR_FRAME_ID, &dlc));
    ASSERT_EQ(1U, dlc);
    return 1;
}

static int32_t test_expected_dlc_temp_b(void)
{
    uint8_t dlc = 0U;
    ASSERT_STATUS(STATUS_OK, hal_expected_dlc_for_id(HAL_SENSOR_TEMP_B_FRAME_ID, &dlc));
    ASSERT_EQ(2U, dlc);
    return 1;
}

static int32_t test_expected_dlc_unknown_id(void)
{
    uint8_t dlc = 0U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_expected_dlc_for_id(0x999U, &dlc));
    return 1;
}

static int32_t test_expected_dlc_null_output(void)
{
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_expected_dlc_for_id(HAL_SENSOR_FRAME_ID, (uint8_t *)0));
    return 1;
}

static int32_t test_dlc_mismatch_rejected_on_ingest(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;

    source.rpm = 2000.0f;
    source.temperature = 70.0f;
    source.oil_pressure = 3.0f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));
    /* Corrupt the DLC to a wrong value for this ID */
    frame.dlc = 4U;
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_ingest_sensor_frame(&frame, 1U));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* ------------------------------------------------------------------
 * Frame Aging tests (Section 1.3)
 * ------------------------------------------------------------------ */

static int32_t test_frame_age_fresh_after_receipt(void)
{
    int32_t stale = -1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_frame_age_record(HAL_SENSOR_FRAME_ID, 5U));
    ASSERT_STATUS(STATUS_OK, hal_frame_is_stale(HAL_SENSOR_FRAME_ID, 5U, &stale));
    ASSERT_EQ(0, stale);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_frame_age_stale_at_exact_threshold(void)
{
    int32_t stale = -1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_frame_age_record(HAL_SENSOR_FRAME_ID, 1U));

    /* At threshold boundary: age == threshold → NOT stale (uses >) */
    ASSERT_STATUS(STATUS_OK, hal_frame_is_stale(HAL_SENSOR_FRAME_ID,
                                                1U + HAL_FRAME_STALE_THRESHOLD_TICKS, &stale));
    ASSERT_EQ(0, stale);

    /* One tick past threshold → stale */
    ASSERT_STATUS(STATUS_OK, hal_frame_is_stale(HAL_SENSOR_FRAME_ID,
                                                1U + HAL_FRAME_STALE_THRESHOLD_TICKS + 1U, &stale));
    ASSERT_EQ(1, stale);

    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_frame_age_never_received_not_stale(void)
{
    int32_t stale = -1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    /* Never recorded - should report not stale */
    ASSERT_STATUS(STATUS_OK, hal_frame_is_stale(HAL_SENSOR_FRAME_ID, 999U, &stale));
    ASSERT_EQ(0, stale);
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_frame_age_unknown_id_rejected(void)
{
    int32_t stale = -1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_frame_age_record(0x999U, 1U));
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_frame_is_stale(0x999U, 1U, &stale));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

static int32_t test_frame_age_null_output(void)
{
    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_INVALID_ARGUMENT, hal_frame_is_stale(HAL_SENSOR_FRAME_ID, 1U, (int32_t *)0));
    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* ------------------------------------------------------------------
 * Sensor queue overflow test (Section 1.4)
 * ------------------------------------------------------------------ */

static int32_t test_sensor_rx_queue_overflow(void)
{
    HAL_SensorFrame source;
    HAL_Frame frame;
    uint32_t idx;
    ErrorInfo error;

    source.rpm = 2000.0f;
    source.temperature = 70.0f;
    source.oil_pressure = 3.0f;
    source.is_running = 1;

    ASSERT_STATUS(STATUS_OK, hal_init());
    ASSERT_STATUS(STATUS_OK, hal_encode_sensor_frame(&source, &frame));

    for (idx = 0U; idx < HAL_MAX_RX_FRAMES; ++idx)
    {
        ASSERT_STATUS(STATUS_OK, hal_ingest_sensor_frame(&frame, idx + 1U));
    }
    /* Next ingest must overflow */
    ASSERT_STATUS(STATUS_BUFFER_OVERFLOW,
                  hal_ingest_sensor_frame(&frame, HAL_MAX_RX_FRAMES + 1U));

    /* Verify structured error was recorded */
    ASSERT_STATUS(STATUS_OK, hal_get_last_error(&error));
    ASSERT_EQ(STATUS_BUFFER_OVERFLOW, error.code);

    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

/* ------------------------------------------------------------------
 * TX queue overflow test (Section 1.4)
 * ------------------------------------------------------------------ */

static int32_t test_hal_transmit_bus_overflow(void)
{
    HAL_Frame frame;
    uint32_t idx;
    ErrorInfo error;

    ASSERT_STATUS(STATUS_OK, hal_init());
    (void)memset(&frame, 0, sizeof(frame));
    frame.id = 0x300U;
    frame.dlc = 8U;

    for (idx = 0U; idx < HAL_MAX_TX_FRAMES; ++idx)
    {
        ASSERT_STATUS(STATUS_OK, hal_transmit_bus(&frame));
    }
    ASSERT_STATUS(STATUS_BUFFER_OVERFLOW, hal_transmit_bus(&frame));

    ASSERT_STATUS(STATUS_OK, hal_get_last_error(&error));
    ASSERT_EQ(STATUS_BUFFER_OVERFLOW, error.code);

    ASSERT_STATUS(STATUS_OK, hal_shutdown());
    return 1;
}

int32_t register_hal_decode_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        /* original tests */
        {"hal_valid_decode", test_valid_frame_decodes_to_sensor_struct},
        {"hal_invalid_id", test_invalid_id_rejected},
        {"hal_wrong_dlc", test_wrong_dlc_rejected},
        {"hal_corrupt_payload", test_corrupt_payload_rejected},
        {"hal_timeout", test_timeout_is_reported_deterministically},
        {"hal_fifo_order", test_fifo_order_is_deterministic},
        {"hal_error_frame", test_sensor_error_frame_returns_parse_error},
        {"hal_timeout_after_last_sample", test_timeout_after_last_valid_sample},
        /* hal_apply_sensors tests */
        {"hal_apply_sensors_ok", test_hal_apply_sensors_updates_engine},
        {"hal_apply_sensors_null_eng", test_hal_apply_sensors_null_engine},
        {"hal_apply_sensors_invalid", test_hal_apply_sensors_invalid_frame},
        /* bus rx/tx tests */
        {"hal_receive_bus_valid", test_hal_receive_bus_valid},
        {"hal_receive_bus_null", test_hal_receive_bus_null},
        {"hal_receive_bus_bad_dlc", test_hal_receive_bus_invalid_dlc},
        {"hal_transmit_bus_valid", test_hal_transmit_bus_valid},
        {"hal_transmit_bus_null", test_hal_transmit_bus_null},
        {"hal_transmit_bus_bad_dlc", test_hal_transmit_bus_invalid_dlc},
        /* actuator tests */
        {"hal_actuators_no_emit", test_hal_write_actuators_no_emit},
        {"hal_actuators_emit", test_hal_write_actuators_emit},
        {"hal_actuators_null", test_hal_write_actuators_null},
        /* error info tests */
        {"hal_last_error_init", test_hal_get_last_error_after_init},
        {"hal_last_error_null", test_hal_get_last_error_null},
        /* encode defensive tests */
        {"hal_encode_null_sensor", test_hal_encode_null_sensor},
        {"hal_encode_null_frame", test_hal_encode_null_frame},
        {"hal_encode_invalid_vals", test_hal_encode_invalid_sensor_values},
        /* ingest/read null tests */
        {"hal_ingest_null", test_hal_ingest_null},
        {"hal_read_sensors_null", test_hal_read_sensors_null},
        /* overflow test */
        {"hal_bus_rx_overflow", test_hal_receive_bus_overflow},
        /* watchdog tests */
        {"hal_wd_disabled_default", test_watchdog_disabled_by_default},
        {"hal_wd_configure_kick", test_watchdog_configure_and_kick},
        {"hal_wd_expires", test_watchdog_expires},
        {"hal_wd_too_large", test_watchdog_configure_too_large},
        {"hal_wd_disable_after", test_watchdog_disable_after_enable},
        /* sensor voting tests */
        {"hal_vote_single", test_vote_single_channel},
        {"hal_vote_dual_agree", test_vote_dual_agree},
        {"hal_vote_dual_disagree", test_vote_dual_disagree},
        {"hal_vote_null_out", test_vote_null_output},
        {"hal_vote_clears", test_vote_clears_after_use},
        /* Frame ID registry tests (Section 1.1) */
        {"hal_fid_known_valid", test_frame_id_known_valid},
        {"hal_fid_unknown_reject", test_frame_id_unknown_rejected},
        /* DLC per-ID enforcement tests (Section 1.2) */
        {"hal_dlc_sensor", test_expected_dlc_sensor},
        {"hal_dlc_error_frame", test_expected_dlc_error_frame},
        {"hal_dlc_temp_b", test_expected_dlc_temp_b},
        {"hal_dlc_unknown_id", test_expected_dlc_unknown_id},
        {"hal_dlc_null_out", test_expected_dlc_null_output},
        {"hal_dlc_mismatch_ingest", test_dlc_mismatch_rejected_on_ingest},
        /* Frame aging tests (Section 1.3) */
        {"hal_age_fresh_receipt", test_frame_age_fresh_after_receipt},
        {"hal_age_stale_boundary", test_frame_age_stale_at_exact_threshold},
        {"hal_age_never_recv", test_frame_age_never_received_not_stale},
        {"hal_age_unknown_id", test_frame_age_unknown_id_rejected},
        {"hal_age_null_out", test_frame_age_null_output},
        /* Queue overflow tests (Section 1.4) */
        {"hal_sensor_rx_overflow", test_sensor_rx_queue_overflow},
        {"hal_tx_overflow", test_hal_transmit_bus_overflow}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
