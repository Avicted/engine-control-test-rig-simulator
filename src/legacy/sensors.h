#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

#include "engine.h"

int32_t scenario_normal_ramp_up(EngineState *engine);
int32_t scenario_overheat(EngineState *engine);
int32_t scenario_oil_pressure_failure(EngineState *engine);

#endif