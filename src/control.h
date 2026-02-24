#ifndef CONTROL_H
#define CONTROL_H

#include "engine.h"

#define MAX_TEMP 95.0f
#define MIN_OIL_PRESSURE 2.5f
#define HIGH_RPM_WARNING_THRESHOLD 3500.0f
#define HIGH_TEMP_WARNING_THRESHOLD 85.0f
#define TEMP_PERSISTENCE_TICKS 3U
#define OIL_PRESSURE_PERSISTENCE_TICKS 3U
#define RPM_TEMP_WARNING_PERSISTENCE_TICKS 2U

int evaluate_engine(EngineState *engine);
int compute_control_output(const EngineState *engine, float *control_output);

#endif
