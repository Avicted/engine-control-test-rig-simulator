#ifndef HAL_H
#define HAL_H

#include "status.h"

typedef struct
{
    float rpm;
    float temperature;
    float oil_pressure;
    int is_running;
} HAL_SensorFrame;

typedef struct
{
    float control_output;
    int emit_control_line;
} HAL_ControlFrame;

StatusCode hal_init(void);
StatusCode hal_shutdown(void);
StatusCode hal_read_sensors(const HAL_SensorFrame *frame);
StatusCode hal_write_actuators(const HAL_ControlFrame *frame);

#endif
