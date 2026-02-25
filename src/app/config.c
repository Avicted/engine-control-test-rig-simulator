#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "engine.h"

static int32_t key_allowed(const char *key)
{
    if (strcmp(key, "temperature_limit") == 0)
    {
        return 1;
    }
    if (strcmp(key, "oil_pressure_limit") == 0)
    {
        return 1;
    }
    if (strcmp(key, "persistence_ticks") == 0)
    {
        return 1;
    }
    if (strcmp(key, "physics") == 0)
    {
        return 1;
    }
    if (strcmp(key, "target_rpm") == 0)
    {
        return 1;
    }
    if (strcmp(key, "target_temperature") == 0)
    {
        return 1;
    }
    if (strcmp(key, "target_oil_pressure") == 0)
    {
        return 1;
    }
    if (strcmp(key, "rpm_ramp_rate") == 0)
    {
        return 1;
    }
    if (strcmp(key, "temp_ramp_rate") == 0)
    {
        return 1;
    }
    if (strcmp(key, "oil_decay_rate") == 0)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Find a JSON key in the buffer, respecting string boundaries and brace depth.
 *
 * Scans the buffer tracking whether we are inside a JSON string value
 * (handling backslash-escaped quotes) and counting brace depth. Only returns
 * a match when the key appears as a bare key token (not inside another string
 * value) and is followed (after optional whitespace) by a colon.
 *
 * @param[in]  buffer  NUL-terminated JSON text.
 * @param[in]  key     Key name to find (without surrounding quotes).
 * @return Pointer to the colon after the matched key, or NULL on failure.
 */
static const char *find_json_key(const char *buffer, const char *key)
{
    char pattern[64];
    size_t pattern_len;
    const char *cursor;
    int32_t in_string = 0;

    if ((buffer == NULL) || (key == NULL))
    {
        return NULL;
    }

    (void)snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    pattern_len = strlen(pattern);

    for (cursor = buffer; *cursor != '\0'; ++cursor)
    {
        if (in_string != 0)
        {
            if ((*cursor == '\\') && (*(cursor + 1) != '\0'))
            {
                ++cursor; /* skip escaped character */
            }
            else if (*cursor == '"')
            {
                in_string = 0;
            }
            continue;
        }

        /* Outside any string value */
        if (*cursor == '"')
        {
            /* Check whether this quoted token matches our pattern */
            if (strncmp(cursor, pattern, pattern_len) == 0)
            {
                const char *after = cursor + pattern_len;
                /* Skip whitespace between key and colon */
                while ((*after == ' ') || (*after == '\t') ||
                       (*after == '\n') || (*after == '\r'))
                {
                    ++after;
                }
                if (*after == ':')
                {
                    return after; /* return pointer to the colon */
                }
            }
            /* Not our key — enter the string so we skip its contents */
            in_string = 1;
        }
        /* Ignore braces/brackets — we only need string-awareness for correctness
         * since the config schema is validated separately by validate_only_known_keys(). */
    }

    return NULL;
}

static int32_t parse_float_field(const char *buffer, const char *key, float *value_out)
{
    const char *colon_pos;
    char *end_ptr;
    float parsed;

    if ((buffer == NULL) || (key == NULL) || (value_out == NULL))
    {
        return 0;
    }

    colon_pos = find_json_key(buffer, key);
    if (colon_pos == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtof(colon_pos + 1, &end_ptr);
    if ((errno != 0) || (end_ptr == (colon_pos + 1)))
    {
        return 0;
    }

    *value_out = parsed;
    return 1;
}

static int32_t parse_uint_field(const char *buffer, const char *key, uint32_t *value_out)
{
    const char *colon_pos;
    char *end_ptr;
    unsigned long parsed;

    if ((buffer == NULL) || (key == NULL) || (value_out == NULL))
    {
        return 0;
    }

    colon_pos = find_json_key(buffer, key);
    if (colon_pos == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtoul(colon_pos + 1, &end_ptr, 10);
    if ((errno != 0) || (end_ptr == (colon_pos + 1)) || (parsed == 0UL) || (parsed > UINT_MAX))
    {
        return 0;
    }

    *value_out = (uint32_t)parsed;
    return 1;
}

static int32_t validate_only_known_keys(const char *buffer)
{
    const char *cursor;

    if (buffer == NULL)
    {
        return 0;
    }

    cursor = strchr(buffer, '"');
    while (cursor != NULL)
    {
        char key[64];
        const char *end_quote;
        size_t key_len;

        end_quote = strchr(cursor + 1, '"');
        if (end_quote == NULL)
        {
            return 0;
        }

        key_len = (size_t)(end_quote - (cursor + 1));
        if (key_len >= sizeof(key))
        {
            return 0;
        }

        (void)memcpy(key, cursor + 1, key_len);
        key[key_len] = '\0';

        if (key_allowed(key) == 0)
        {
            const char *after_quote = end_quote + 1;
            while ((*after_quote == ' ') || (*after_quote == '\t') || (*after_quote == '\n') || (*after_quote == '\r'))
            {
                ++after_quote;
            }
            if (*after_quote == ':')
            {
                return 0;
            }
        }

        cursor = strchr(end_quote + 1, '"');
    }

    return 1;
}

StatusCode config_load_calibration_file(const char *path,
                                        ControlCalibration *calibration_out,
                                        char *error_message,
                                        uint32_t error_message_size)
{
    FILE *config_file;
    char buffer[CONFIG_FILE_MAX_BYTES + 1U];
    size_t bytes_read;
    ControlCalibration calibration;

    if ((path == NULL) || (calibration_out == NULL) || (error_message == NULL) ||
        (error_message_size == 0U))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    error_message[0] = '\0';

    config_file = fopen(path, "r");
    if (config_file == NULL)
    {
        (void)snprintf(error_message, error_message_size, "Unable to open config file: %s", path);
        return STATUS_IO_ERROR;
    }

    bytes_read = fread(buffer, 1U, CONFIG_FILE_MAX_BYTES, config_file);
    if (ferror(config_file) != 0)
    {
        (void)fclose(config_file);
        (void)snprintf(error_message, error_message_size, "Unable to read config file: %s", path);
        return STATUS_IO_ERROR;
    }

    if (bytes_read >= CONFIG_FILE_MAX_BYTES)
    {
        (void)fclose(config_file);
        (void)snprintf(error_message, error_message_size, "Config file exceeds %u bytes", CONFIG_FILE_MAX_BYTES);
        return STATUS_PARSE_ERROR;
    }

    buffer[bytes_read] = '\0';

    if (fclose(config_file) != 0)
    {
        (void)snprintf(error_message, error_message_size, "Unable to close config file: %s", path);
        return STATUS_IO_ERROR;
    }

    if (validate_only_known_keys(buffer) == 0)
    {
        (void)snprintf(error_message, error_message_size, "Config file contains unsupported keys");
        return STATUS_PARSE_ERROR;
    }

    if (control_get_default_calibration(&calibration) != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    if (parse_float_field(buffer, "temperature_limit", &calibration.temperature_limit) == 0)
    {
        (void)snprintf(error_message, error_message_size, "Missing or invalid temperature_limit");
        return STATUS_PARSE_ERROR;
    }

    if (parse_float_field(buffer, "oil_pressure_limit", &calibration.oil_pressure_limit) == 0)
    {
        (void)snprintf(error_message, error_message_size, "Missing or invalid oil_pressure_limit");
        return STATUS_PARSE_ERROR;
    }

    if (parse_uint_field(buffer, "persistence_ticks", &calibration.temp_persistence_ticks) == 0)
    {
        (void)snprintf(error_message, error_message_size, "Missing or invalid persistence_ticks");
        return STATUS_PARSE_ERROR;
    }

    calibration.oil_persistence_ticks = calibration.temp_persistence_ticks;
    calibration.combined_warning_persistence_ticks = calibration.temp_persistence_ticks;

    *calibration_out = calibration;
    return STATUS_OK;
}

StatusCode config_load_physics_file(const char *path,
                                    EnginePhysicsConfig *physics_out,
                                    char *error_message,
                                    uint32_t error_message_size)
{
    FILE *config_file;
    char buffer[CONFIG_FILE_MAX_BYTES + 1U];
    size_t bytes_read;
    EnginePhysicsConfig physics;
    int32_t found_any;

    if ((path == NULL) || (physics_out == NULL) ||
        (error_message == NULL) || (error_message_size == 0U))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    error_message[0] = '\0';

    /* Start with defaults */
    if (engine_get_default_physics(&physics) != STATUS_OK)
    {
        return STATUS_INTERNAL_ERROR;
    }

    config_file = fopen(path, "r");
    if (config_file == NULL)
    {
        (void)snprintf(error_message, error_message_size, "Unable to open config file: %s", path);
        return STATUS_IO_ERROR;
    }

    bytes_read = fread(buffer, 1U, CONFIG_FILE_MAX_BYTES, config_file);
    if (ferror(config_file) != 0)
    {
        (void)fclose(config_file);
        (void)snprintf(error_message, error_message_size, "Unable to read config file: %s", path);
        return STATUS_IO_ERROR;
    }

    if (bytes_read >= CONFIG_FILE_MAX_BYTES)
    {
        (void)fclose(config_file);
        (void)snprintf(error_message, error_message_size, "Config file exceeds %u bytes", CONFIG_FILE_MAX_BYTES);
        return STATUS_PARSE_ERROR;
    }

    buffer[bytes_read] = '\0';
    (void)fclose(config_file);

    /* If "physics" key not found at all, return defaults (optional section) */
    if (strstr(buffer, "\"physics\"") == NULL)
    {
        *physics_out = physics;
        return STATUS_OK;
    }

    /* Parse each physics field - all required when physics section present */
    found_any = 0;

    if (parse_float_field(buffer, "target_rpm", &physics.target_rpm) != 0)
    {
        found_any = 1;
    }
    if (parse_float_field(buffer, "target_temperature", &physics.target_temperature) != 0)
    {
        found_any = 1;
    }
    if (parse_float_field(buffer, "target_oil_pressure", &physics.target_oil_pressure) != 0)
    {
        found_any = 1;
    }
    if (parse_float_field(buffer, "rpm_ramp_rate", &physics.rpm_ramp_rate) != 0)
    {
        found_any = 1;
    }
    if (parse_float_field(buffer, "temp_ramp_rate", &physics.temp_ramp_rate) != 0)
    {
        found_any = 1;
    }
    if (parse_float_field(buffer, "oil_decay_rate", &physics.oil_decay_rate) != 0)
    {
        found_any = 1;
    }

    if (found_any == 0)
    {
        (void)snprintf(error_message, error_message_size,
                       "Physics section present but no valid fields found");
        return STATUS_PARSE_ERROR;
    }

    /* Validate all values are positive */
    if ((physics.target_rpm <= 0.0f) || (physics.target_temperature <= 0.0f) ||
        (physics.target_oil_pressure <= 0.0f) || (physics.rpm_ramp_rate <= 0.0f) ||
        (physics.temp_ramp_rate <= 0.0f) || (physics.oil_decay_rate <= 0.0f))
    {
        (void)snprintf(error_message, error_message_size,
                       "Physics parameters must be positive");
        return STATUS_PARSE_ERROR;
    }

    *physics_out = physics;
    return STATUS_OK;
}
