#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#include "control.h"
#include "status.h"

#define CONFIG_FILE_MAX_BYTES 2048U

StatusCode config_load_calibration_file(const char *path,
                                        ControlCalibration *calibration_out,
                                        char *error_message,
                                        uint32_t error_message_size);

#endif
