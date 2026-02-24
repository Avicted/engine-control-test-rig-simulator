#ifndef SENSORS_H
#define SENSORS_H

#include "engine.h"

int scenario_normal_ramp_up(EngineState *engine);
int scenario_overheat(EngineState *engine);
int scenario_oil_pressure_failure(EngineState *engine);

#endif
