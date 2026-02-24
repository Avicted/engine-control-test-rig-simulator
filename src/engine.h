#ifndef ENGINE_H
#define ENGINE_H

#define ENGINE_OK 0
#define ENGINE_WARNING 1
#define ENGINE_SHUTDOWN 2
#define ENGINE_ERROR -1

typedef struct
{
    float rpm;
    float temperature;
    float oil_pressure;
    int is_running;
} EngineState;

int engine_init(EngineState *engine);
int engine_reset(EngineState *engine);
int engine_start(EngineState *engine);
int engine_update(EngineState *engine);

#endif
