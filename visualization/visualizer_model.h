#ifndef VISUALIZER_MODEL_H
#define VISUALIZER_MODEL_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAX_FILE_SIZE (1024U * 1024U)
#define MAX_TICKS 512U
#define MAX_SCENARIO_NAME 64U
#define MAX_MODE_NAME 16U
#define MAX_REQUIREMENT_ID 32U
#define MAX_EXPECTED_VALUE 16U
#define MAX_SCENARIOS 32U

#define MAX_RPM 10000.0f
#define MIN_TEMP -50.0f
#define MAX_TEMP 200.0f
#define MIN_OIL 0.0f
#define MAX_OIL 10.0f
#define MIN_CONTROL 0.0f
#define MAX_CONTROL 100.0f

#define TEMP_SHUTDOWN_THRESHOLD 95.0f
#define TEMP_WARNING_THRESHOLD 85.0f
#define OIL_SHUTDOWN_THRESHOLD 2.5f
#define RPM_WARNING_THRESHOLD 3500.0f

#define DEFAULT_TICKS_PER_SECOND 6.0f
#define SHORT_SCENARIO_TICKS_PER_SECOND 2.5f

#define FONT_PATH "visualization/PxPlus_IBM_EGA_8x14.ttf"

typedef enum
{
    LEVEL_OK = 0,
    LEVEL_WARNING = 1,
    LEVEL_SHUTDOWN = 2
} SeverityLevel;

typedef struct
{
    unsigned int tick;
    float rpm;
    float temp;
    float oil;
    int run;
    char result[12];
    float control;
    char engine_mode[MAX_MODE_NAME];
} TickData;

typedef struct
{
    char scenario[MAX_SCENARIO_NAME];
    char requirement_id[MAX_REQUIREMENT_ID];
    char expected[MAX_EXPECTED_VALUE];
    TickData ticks[MAX_TICKS];
    unsigned int tick_count;
} ScenarioData;

typedef struct
{
    ScenarioData scenarios[MAX_SCENARIOS];
    unsigned int count;
    unsigned int active_index;
} ScenarioSet;

#endif
