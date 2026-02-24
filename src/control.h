#ifndef CONTROL_H
#define CONTROL_H

#include "engine.h"

#define MAX_TEMP 95.0f
#define MIN_OIL_PRESSURE 2.5f
#define HIGH_RPM_WARNING_THRESHOLD 3500.0f
#define HIGH_TEMP_WARNING_THRESHOLD 85.0f

int evaluate_engine(const EngineState *engine);

#endif
