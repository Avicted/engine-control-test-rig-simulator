/**
 * @file test_config_module.c
 * @brief Unit tests for configuration validation discipline (Section 6).
 *
 * Verifies that malformed, missing, and invalid configuration files
 * are rejected deterministically with structured error messages.
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "test_harness.h"

/* --- Helper: write a config file from string --- */

static int32_t write_test_config(const char *path, const char *content)
{
    FILE *file;

    file = fopen(path, "w");
    if (file == (FILE *)0)
    {
        return 0;
    }
    if (fputs(content, file) < 0)
    {
        (void)fclose(file);
        return 0;
    }
    (void)fclose(file);
    return 1;
}

/* --- Valid config loads successfully --- */

static int32_t test_config_valid_loads(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_valid.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 3\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_OK,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(cal.temperature_limit > 94.0f);
    ASSERT_TRUE(cal.temperature_limit < 96.0f);
    ASSERT_TRUE(cal.oil_pressure_limit > 2.4f);
    ASSERT_TRUE(cal.oil_pressure_limit < 2.6f);
    ASSERT_EQ(3U, cal.temp_persistence_ticks);
    return 1;
}

/* --- Missing required field --- */

static int32_t test_config_missing_temp_limit(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_no_temp.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 3\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "temperature_limit") != (char *)0);
    return 1;
}

static int32_t test_config_missing_oil_limit(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_no_oil.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"persistence_ticks\": 3\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "oil_pressure_limit") != (char *)0);
    return 1;
}

static int32_t test_config_missing_persistence(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_no_persist.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "persistence_ticks") != (char *)0);
    return 1;
}

/* --- Unknown keys rejected --- */

static int32_t test_config_unknown_key_rejected(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_unknown.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 3,\n"
                                  "  \"rogue_key\": 42\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "unsupported") != (char *)0);
    return 1;
}

/* --- Non-existent file --- */

static int32_t test_config_file_not_found(void)
{
    ControlCalibration cal;
    char err[128];

    ASSERT_STATUS(STATUS_IO_ERROR,
                  config_load_calibration_file("build/no_such_file.json",
                                               &cal, err, (uint32_t)sizeof(err)));
    return 1;
}

/* --- NULL argument handling --- */

static int32_t test_config_null_path(void)
{
    ControlCalibration cal;
    char err[128];

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  config_load_calibration_file((const char *)0,
                                               &cal, err, (uint32_t)sizeof(err)));
    return 1;
}

static int32_t test_config_null_calibration(void)
{
    char err[128];

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  config_load_calibration_file("calibration.json",
                                               (ControlCalibration *)0, err, (uint32_t)sizeof(err)));
    return 1;
}

static int32_t test_config_null_error_buffer(void)
{
    ControlCalibration cal;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  config_load_calibration_file("calibration.json",
                                               &cal, (char *)0, 128U));
    return 1;
}

static int32_t test_config_zero_error_size(void)
{
    ControlCalibration cal;
    char err[128];

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  config_load_calibration_file("calibration.json",
                                               &cal, err, 0U));
    return 1;
}

/* --- Physics config: valid, missing section, invalid values --- */

static int32_t test_physics_config_valid(void)
{
    EnginePhysicsConfig physics;
    char err[128];
    const char *path = "build/unit_config_physics.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 3,\n"
                                  "  \"physics\": {\n"
                                  "    \"target_rpm\": 3000.0,\n"
                                  "    \"target_temperature\": 90.0,\n"
                                  "    \"target_oil_pressure\": 3.4,\n"
                                  "    \"rpm_ramp_rate\": 150.0,\n"
                                  "    \"temp_ramp_rate\": 0.6,\n"
                                  "    \"oil_decay_rate\": 0.01\n"
                                  "  }\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_OK,
                  config_load_physics_file(path, &physics, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(physics.target_rpm > 2999.0f);
    ASSERT_TRUE(physics.target_rpm < 3001.0f);
    return 1;
}

static int32_t test_physics_config_missing_section_defaults(void)
{
    EnginePhysicsConfig physics;
    char err[128];
    const char *path = "build/unit_config_no_physics.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 3\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_OK,
                  config_load_physics_file(path, &physics, err, (uint32_t)sizeof(err)));
    /* Should receive factory defaults */
    ASSERT_TRUE(physics.target_rpm == ENGINE_DEFAULT_TARGET_RPM);
    return 1;
}

static int32_t test_config_allowed_keys_and_combined_override(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_allowed_keys.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\" 95.0,\n"
                                  "  \"temperature_limit\"\n"
                                  "\t: 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 3,\n"
                                  "  \"combined_warning_persistence_ticks\": 7,\n"
                                  "  \"physics\": \"escaped \\\"target_rpm\\\" marker\",\n"
                                  "  \"target_rpm\": 3000.0,\n"
                                  "  \"target_temperature\": 90.0,\n"
                                  "  \"target_oil_pressure\": 3.4,\n"
                                  "  \"rpm_ramp_rate\": 150.0,\n"
                                  "  \"temp_ramp_rate\": 0.6,\n"
                                  "  \"oil_decay_rate\": 0.01\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_OK,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_EQ(7U, cal.combined_warning_persistence_ticks);
    return 1;
}

static int32_t test_config_zero_persistence_rejected(void)
{
    ControlCalibration cal;
    char err[128];
    const char *path = "build/unit_config_zero_persistence.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"temperature_limit\": 95.0,\n"
                                  "  \"oil_pressure_limit\": 2.5,\n"
                                  "  \"persistence_ticks\": 0\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_calibration_file(path, &cal, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "persistence_ticks") != (char *)0);
    return 1;
}

static int32_t test_physics_config_no_valid_fields_rejected(void)
{
    EnginePhysicsConfig physics;
    char err[128];
    const char *path = "build/unit_config_physics_no_valid_fields.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"physics\": {\n"
                                  "    \"target_rpm\": \"fast\"\n"
                                  "  }\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_physics_file(path, &physics, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "no valid fields") != (char *)0);
    return 1;
}

static int32_t test_physics_config_non_positive_rejected(void)
{
    EnginePhysicsConfig physics;
    char err[128];
    const char *path = "build/unit_config_physics_non_positive.json";

    ASSERT_TRUE(write_test_config(path,
                                  "{\n"
                                  "  \"physics\": {\n"
                                  "    \"target_rpm\": 0.0,\n"
                                  "    \"target_temperature\": 90.0,\n"
                                  "    \"target_oil_pressure\": 3.4,\n"
                                  "    \"rpm_ramp_rate\": 150.0,\n"
                                  "    \"temp_ramp_rate\": 0.6,\n"
                                  "    \"oil_decay_rate\": 0.01\n"
                                  "  }\n"
                                  "}\n"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  config_load_physics_file(path, &physics, err, (uint32_t)sizeof(err)));
    ASSERT_TRUE(strstr(err, "must be positive") != (char *)0);
    return 1;
}

static int32_t test_physics_config_null_path(void)
{
    EnginePhysicsConfig physics;
    char err[128];

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  config_load_physics_file((const char *)0,
                                           &physics, err, (uint32_t)sizeof(err)));
    return 1;
}

int32_t register_config_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"config_valid_loads", test_config_valid_loads},
        {"config_missing_temp", test_config_missing_temp_limit},
        {"config_missing_oil", test_config_missing_oil_limit},
        {"config_missing_persist", test_config_missing_persistence},
        {"config_unknown_key", test_config_unknown_key_rejected},
        {"config_file_not_found", test_config_file_not_found},
        {"config_null_path", test_config_null_path},
        {"config_null_cal", test_config_null_calibration},
        {"config_null_errbuf", test_config_null_error_buffer},
        {"config_zero_errsize", test_config_zero_error_size},
        {"config_physics_valid", test_physics_config_valid},
        {"config_physics_defaults", test_physics_config_missing_section_defaults},
        {"config_allowed_keys_override", test_config_allowed_keys_and_combined_override},
        {"config_zero_persist_rejected", test_config_zero_persistence_rejected},
        {"config_physics_no_valid_fields", test_physics_config_no_valid_fields_rejected},
        {"config_physics_non_positive", test_physics_config_non_positive_rejected},
        {"config_physics_null", test_physics_config_null_path}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
