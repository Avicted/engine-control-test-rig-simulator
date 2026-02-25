#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hal.h"

#define HAL_MIN_RPM 0.0f
#define HAL_MAX_RPM 10000.0f
#define HAL_MIN_TEMP -50.0f
#define HAL_MAX_TEMP 200.0f
#define HAL_MIN_OIL 0.0f
#define HAL_MAX_OIL 10.0f

typedef struct
{
    HAL_Frame frames[HAL_MAX_RX_FRAMES];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} HAL_FrameQueue;

typedef struct
{
    HAL_Frame frames[HAL_MAX_TX_FRAMES];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} HAL_TxFrameQueue;

/*
 * NOTE: All module-level state assumes single-threaded execution.
 * If threaded scenarios are introduced (see MISRA Dir 5.1/5.2),
 * add volatile or _Atomic qualifiers and appropriate synchronisation.
 */
static HAL_FrameQueue g_sensor_rx_queue;
static HAL_FrameQueue g_bus_rx_queue;
static HAL_TxFrameQueue g_bus_tx_queue;
static uint32_t g_last_sensor_tick;
static int32_t g_has_sensor_tick;
static ErrorInfo g_last_error;

/* --- Software watchdog state --- */
static uint32_t g_watchdog_timeout = 0U; /* 0 = disabled */
static uint32_t g_watchdog_counter = 0U;
static int32_t g_watchdog_enabled = 0;

/* --- Sensor voting state --- */
static float g_redundant_temp_b = 0.0f;
static int32_t g_has_redundant_temp = 0;

static void hal_set_error(StatusCode code, const char *function, uint32_t tick, Severity severity)
{
    g_last_error.code = code;
    g_last_error.module = "hal";
    g_last_error.function = function;
    g_last_error.tick = tick;
    g_last_error.severity = severity;
    g_last_error.recoverability = status_code_default_recoverability(code);
}

static void queue_reset(HAL_FrameQueue *queue)
{
    if (queue == NULL)
    {
        return;
    }

    (void)memset(queue->frames, 0, sizeof(queue->frames));
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
}

static void tx_queue_reset(HAL_TxFrameQueue *queue)
{
    if (queue == NULL)
    {
        return;
    }

    (void)memset(queue->frames, 0, sizeof(queue->frames));
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
}

static StatusCode queue_push(HAL_FrameQueue *queue, const HAL_Frame *frame, uint32_t capacity)
{
    if ((queue == NULL) || (frame == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (queue->count >= capacity)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    queue->frames[queue->tail] = *frame;
    queue->tail = (queue->tail + 1U) % capacity;
    queue->count += 1U;
    return STATUS_OK;
}

static StatusCode queue_pop(HAL_FrameQueue *queue, HAL_Frame *frame_out, uint32_t capacity)
{
    if ((queue == NULL) || (frame_out == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (queue->count == 0U)
    {
        return STATUS_IO_ERROR;
    }

    *frame_out = queue->frames[queue->head];
    queue->head = (queue->head + 1U) % capacity;
    queue->count -= 1U;
    return STATUS_OK;
}

static StatusCode tx_queue_push(HAL_TxFrameQueue *queue, const HAL_Frame *frame)
{
    if ((queue == NULL) || (frame == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (queue->count >= HAL_MAX_TX_FRAMES)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    queue->frames[queue->tail] = *frame;
    queue->tail = (queue->tail + 1U) % HAL_MAX_TX_FRAMES;
    queue->count += 1U;
    return STATUS_OK;
}

static uint8_t frame_checksum(const HAL_Frame *frame)
{
    uint8_t checksum = 0U;
    uint32_t idx;

    if (frame == NULL)
    {
        return 0U;
    }

    for (idx = 0U; idx < 7U; ++idx)
    {
        checksum ^= frame->data[idx];
    }

    return checksum;
}

static StatusCode validate_sensor_frame(const HAL_SensorFrame *frame)
{
    if (frame == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (!isfinite(frame->rpm) || !isfinite(frame->temperature) || !isfinite(frame->oil_pressure))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((frame->rpm < HAL_MIN_RPM) || (frame->rpm > HAL_MAX_RPM) || (frame->temperature < HAL_MIN_TEMP) ||
        (frame->temperature > HAL_MAX_TEMP) || (frame->oil_pressure < HAL_MIN_OIL) ||
        (frame->oil_pressure > HAL_MAX_OIL) || ((frame->is_running != 0) && (frame->is_running != 1)))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

static StatusCode decode_sensor_frame(const HAL_Frame *frame, HAL_SensorFrame *sensor_frame)
{
    uint16_t rpm_encoded;
    uint16_t temp_encoded;
    uint16_t oil_encoded;

    if ((frame == NULL) || (sensor_frame == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((frame->id != HAL_SENSOR_FRAME_ID) || (frame->dlc != 8U))
    {
        return STATUS_PARSE_ERROR;
    }

    if (frame->data[7] != frame_checksum(frame))
    {
        return STATUS_PARSE_ERROR;
    }

    rpm_encoded = (uint16_t)(((uint16_t)frame->data[0] << 8U) | (uint16_t)frame->data[1]);
    temp_encoded = (uint16_t)(((uint16_t)frame->data[2] << 8U) | (uint16_t)frame->data[3]);
    oil_encoded = (uint16_t)(((uint16_t)frame->data[4] << 8U) | (uint16_t)frame->data[5]);

    sensor_frame->rpm = (float)rpm_encoded;
    sensor_frame->temperature = ((float)temp_encoded - 500.0f) / 10.0f;
    sensor_frame->oil_pressure = (float)oil_encoded / 100.0f;
    sensor_frame->is_running = (int32_t)frame->data[6];

    return validate_sensor_frame(sensor_frame);
}

static int32_t is_supported_sensor_transport_frame(const HAL_Frame *frame)
{
    if (frame == NULL)
    {
        return 0;
    }

    if ((frame->id == HAL_SENSOR_FRAME_ID) && (frame->dlc == 8U))
    {
        return 1;
    }

    if ((frame->id == HAL_SENSOR_ERROR_FRAME_ID) && (frame->dlc == 1U))
    {
        return 1;
    }

    return 0;
}

StatusCode hal_init(void)
{
    queue_reset(&g_sensor_rx_queue);
    queue_reset(&g_bus_rx_queue);
    tx_queue_reset(&g_bus_tx_queue);
    g_last_sensor_tick = 0U;
    g_has_sensor_tick = 0;
    g_watchdog_timeout = 0U;
    g_watchdog_counter = 0U;
    g_watchdog_enabled = 0;
    g_redundant_temp_b = 0.0f;
    g_has_redundant_temp = 0;
    hal_set_error(STATUS_OK, "hal_init", 0U, SEVERITY_INFO);
    return STATUS_OK;
}

StatusCode hal_shutdown(void)
{
    queue_reset(&g_sensor_rx_queue);
    queue_reset(&g_bus_rx_queue);
    tx_queue_reset(&g_bus_tx_queue);
    g_last_sensor_tick = 0U;
    g_has_sensor_tick = 0;
    g_watchdog_timeout = 0U;
    g_watchdog_counter = 0U;
    g_watchdog_enabled = 0;
    g_redundant_temp_b = 0.0f;
    g_has_redundant_temp = 0;
    hal_set_error(STATUS_OK, "hal_shutdown", 0U, SEVERITY_INFO);
    return STATUS_OK;
}

StatusCode hal_ingest_sensor_frame(const HAL_Frame *frame, uint32_t tick)
{
    StatusCode queue_status;

    if (frame == NULL)
    {
        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_ingest_sensor_frame", tick, SEVERITY_ERROR);
        return STATUS_INVALID_ARGUMENT;
    }

    if (is_supported_sensor_transport_frame(frame) == 0)
    {
        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_ingest_sensor_frame", tick, SEVERITY_ERROR);
        return STATUS_INVALID_ARGUMENT;
    }

    queue_status = queue_push(&g_sensor_rx_queue, frame, HAL_MAX_RX_FRAMES);
    if (queue_status != STATUS_OK)
    {
        hal_set_error(queue_status, "hal_ingest_sensor_frame", tick, SEVERITY_ERROR);
        return queue_status;
    }

    hal_set_error(STATUS_OK, "hal_ingest_sensor_frame", tick, SEVERITY_INFO);
    return STATUS_OK;
}

StatusCode hal_read_sensors(uint32_t tick, HAL_SensorFrame *frame_out)
{
    HAL_Frame frame;
    StatusCode status;

    if (frame_out == NULL)
    {
        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_read_sensors", tick, SEVERITY_ERROR);
        return STATUS_INVALID_ARGUMENT;
    }

    status = queue_pop(&g_sensor_rx_queue, &frame, HAL_MAX_RX_FRAMES);
    if (status != STATUS_OK)
    {
        if ((g_has_sensor_tick == 0) && (tick > HAL_SENSOR_TIMEOUT_TICKS))
        {
            hal_set_error(STATUS_TIMEOUT, "hal_read_sensors", tick, SEVERITY_FATAL);
            return STATUS_TIMEOUT;
        }

        if ((g_has_sensor_tick != 0) && ((tick - g_last_sensor_tick) > HAL_SENSOR_TIMEOUT_TICKS))
        {
            hal_set_error(STATUS_TIMEOUT, "hal_read_sensors", tick, SEVERITY_FATAL);
            return STATUS_TIMEOUT;
        }

        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_read_sensors", tick, SEVERITY_WARNING);
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame.id == HAL_SENSOR_ERROR_FRAME_ID)
    {
        hal_set_error(STATUS_PARSE_ERROR, "hal_read_sensors", tick, SEVERITY_ERROR);
        return STATUS_PARSE_ERROR;
    }

    status = decode_sensor_frame(&frame, frame_out);
    if (status != STATUS_OK)
    {
        hal_set_error(status, "hal_read_sensors", tick, SEVERITY_ERROR);
        return status;
    }

    g_last_sensor_tick = tick;
    g_has_sensor_tick = 1;
    hal_set_error(STATUS_OK, "hal_read_sensors", tick, SEVERITY_INFO);
    return STATUS_OK;
}

StatusCode hal_encode_sensor_frame(const HAL_SensorFrame *sensor_frame, HAL_Frame *frame_out)
{
    uint16_t rpm_encoded;
    uint16_t temp_encoded;
    uint16_t oil_encoded;

    if ((sensor_frame == NULL) || (frame_out == NULL))
    {
        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_encode_sensor_frame", 0U, SEVERITY_ERROR);
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_sensor_frame(sensor_frame) != STATUS_OK)
    {
        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_encode_sensor_frame", 0U, SEVERITY_ERROR);
        return STATUS_INVALID_ARGUMENT;
    }

    rpm_encoded = (uint16_t)(sensor_frame->rpm + 0.5f);
    temp_encoded = (uint16_t)(((sensor_frame->temperature + 50.0f) * 10.0f) + 0.5f);
    oil_encoded = (uint16_t)((sensor_frame->oil_pressure * 100.0f) + 0.5f);

    frame_out->id = HAL_SENSOR_FRAME_ID;
    frame_out->dlc = 8U;
    frame_out->data[0] = (uint8_t)((rpm_encoded >> 8U) & 0xFFU);
    frame_out->data[1] = (uint8_t)(rpm_encoded & 0xFFU);
    frame_out->data[2] = (uint8_t)((temp_encoded >> 8U) & 0xFFU);
    frame_out->data[3] = (uint8_t)(temp_encoded & 0xFFU);
    frame_out->data[4] = (uint8_t)((oil_encoded >> 8U) & 0xFFU);
    frame_out->data[5] = (uint8_t)(oil_encoded & 0xFFU);
    frame_out->data[6] = (uint8_t)sensor_frame->is_running;
    frame_out->data[7] = frame_checksum(frame_out);

    hal_set_error(STATUS_OK, "hal_encode_sensor_frame", 0U, SEVERITY_INFO);
    return STATUS_OK;
}

StatusCode hal_apply_sensors(const HAL_SensorFrame *frame, EngineState *engine)
{
    if (engine == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_sensor_frame(frame) != STATUS_OK)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    engine->is_running = frame->is_running;
    engine->rpm = frame->rpm;
    engine->temperature = frame->temperature;
    engine->oil_pressure = frame->oil_pressure;

    return STATUS_OK;
}

StatusCode hal_receive_bus(const HAL_Frame *frame)
{
    StatusCode queue_status;

    if (frame == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame->dlc > 8U)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    queue_status = queue_push(&g_bus_rx_queue, frame, HAL_MAX_RX_FRAMES);
    if (queue_status != STATUS_OK)
    {
        return queue_status;
    }

    return STATUS_OK;
}

StatusCode hal_transmit_bus(const HAL_Frame *frame)
{
    StatusCode queue_status;

    if (frame == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame->dlc > 8U)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    queue_status = tx_queue_push(&g_bus_tx_queue, frame);
    if (queue_status != STATUS_OK)
    {
        return queue_status;
    }

    return STATUS_OK;
}

StatusCode hal_write_actuators(const HAL_ControlFrame *frame)
{
    if (frame == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (frame->emit_control_line != 0)
    {
        int32_t written;

        written = printf("CTRL | output=%6.2f%%\n", frame->control_output);
        if (written < 0)
        {
            return STATUS_IO_ERROR;
        }
    }

    return STATUS_OK;
}

StatusCode hal_get_last_error(ErrorInfo *error_info)
{
    if (error_info == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    *error_info = g_last_error;
    return STATUS_OK;
}

/* --- Software watchdog implementation --- */

StatusCode hal_watchdog_configure(uint32_t timeout_ticks)
{
    if (timeout_ticks > HAL_WATCHDOG_MAX_TICKS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    g_watchdog_timeout = timeout_ticks;
    g_watchdog_counter = 0U;
    g_watchdog_enabled = (timeout_ticks > 0U) ? 1 : 0;
    return STATUS_OK;
}

StatusCode hal_watchdog_kick(void)
{
    g_watchdog_counter = 0U;
    return STATUS_OK;
}

StatusCode hal_watchdog_check(uint32_t tick)
{
    if (g_watchdog_enabled == 0)
    {
        return STATUS_OK;
    }

    g_watchdog_counter++;
    if (g_watchdog_counter > g_watchdog_timeout)
    {
        hal_set_error(STATUS_TIMEOUT, "hal_watchdog_check", tick, SEVERITY_FATAL);
        return STATUS_TIMEOUT;
    }

    return STATUS_OK;
}

/* --- Sensor voting implementation --- */

StatusCode hal_submit_redundant_temp(float temperature_b, uint32_t tick)
{
    if (!isfinite(temperature_b))
    {
        hal_set_error(STATUS_INVALID_ARGUMENT, "hal_submit_redundant_temp", tick, SEVERITY_ERROR);
        return STATUS_INVALID_ARGUMENT;
    }

    (void)tick; /* reserved for future diagnostics */
    g_redundant_temp_b = temperature_b;
    g_has_redundant_temp = 1;
    return STATUS_OK;
}

StatusCode hal_vote_sensors(float primary_temp, float *voted_temp)
{
    float diff;

    if (voted_temp == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (g_has_redundant_temp == 0)
    {
        /* Single-channel mode - pass through primary */
        *voted_temp = primary_temp;
        return STATUS_OK;
    }

    /* Compute absolute difference */
    diff = primary_temp - g_redundant_temp_b;
    if (diff < 0.0f)
    {
        diff = -diff;
    }

    if (diff > HAL_SENSOR_VOTING_TOLERANCE)
    {
        hal_set_error(STATUS_PARSE_ERROR, "hal_vote_sensors", 0U, SEVERITY_ERROR);
        /* Clear redundant reading for next cycle */
        g_has_redundant_temp = 0;
        return STATUS_PARSE_ERROR;
    }

    /* Average the two channels */
    *voted_temp = (primary_temp + g_redundant_temp_b) / 2.0f;
    g_has_redundant_temp = 0;
    return STATUS_OK;
}
