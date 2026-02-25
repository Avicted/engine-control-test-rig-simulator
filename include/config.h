#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#include "control.h"
#include "engine.h"
#include "status.h"

#define CONFIG_FILE_MAX_BYTES 2048U

/**
 * @brief Load control calibration from a JSON configuration file.
 * @param[in]  path              Path to calibration JSON file.
 * @param[out] calibration_out   Receives parsed calibration values.
 * @param[out] error_message     Receives error description on failure.
 * @param[in]  error_message_size Size of error_message buffer.
 * @retval STATUS_OK              Success.
 * @retval STATUS_PARSE_ERROR     Invalid or missing fields.
 * @retval STATUS_IO_ERROR        File I/O failure.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or zero buffer size.
 */
StatusCode config_load_calibration_file(const char *path,
                                        ControlCalibration *calibration_out,
                                        char *error_message,
                                        uint32_t error_message_size);

/**
 * @brief Load engine physics configuration from a JSON configuration file.
 *
 * If the "physics" section is absent from the file, physics_out is populated
 * with factory defaults and STATUS_OK is returned.
 *
 * @param[in]  path              Path to calibration JSON file.
 * @param[out] physics_out       Receives parsed physics configuration.
 * @param[out] error_message     Receives error description on failure.
 * @param[in]  error_message_size Size of error_message buffer.
 * @retval STATUS_OK              Success (or defaults when physics section absent).
 * @retval STATUS_PARSE_ERROR     Invalid physics field values.
 * @retval STATUS_IO_ERROR        File I/O failure.
 * @retval STATUS_INVALID_ARGUMENT NULL pointer or zero buffer size.
 */
StatusCode config_load_physics_file(const char *path,
                                    EnginePhysicsConfig *physics_out,
                                    char *error_message,
                                    uint32_t error_message_size);

#endif
