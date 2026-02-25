#include <stdio.h>
#include <string.h>

#include "config.h"
#include "test_harness.h"

static const char *g_cfg_mock_content = "";
static int32_t g_cfg_mock_open_fail = 0;
static int32_t g_cfg_mock_read_error = 0;
static int32_t g_cfg_mock_close_error = 0;
static StatusCode g_cfg_mock_default_cal_status = STATUS_OK;
static StatusCode g_cfg_mock_default_physics_status = STATUS_OK;

static FILE *config_ut_fopen(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    if (g_cfg_mock_open_fail != 0)
    {
        return (FILE *)0;
    }
    return (FILE *)1;
}

static size_t config_ut_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t max_bytes;
    size_t src_len;
    size_t copy_len;

    (void)stream;
    max_bytes = size * nmemb;
    src_len = strlen(g_cfg_mock_content);
    copy_len = (src_len < max_bytes) ? src_len : max_bytes;
    if (copy_len > 0U)
    {
        (void)memcpy(ptr, g_cfg_mock_content, copy_len);
    }
    return copy_len;
}

static int config_ut_ferror(FILE *stream)
{
    (void)stream;
    return (g_cfg_mock_read_error != 0) ? 1 : 0;
}

static int config_ut_fclose(FILE *stream)
{
    (void)stream;
    return (g_cfg_mock_close_error != 0) ? -1 : 0;
}

static StatusCode config_ut_control_get_default_calibration(ControlCalibration *calibration_out)
{
    if (g_cfg_mock_default_cal_status != STATUS_OK)
    {
        return g_cfg_mock_default_cal_status;
    }
    return control_get_default_calibration(calibration_out);
}

static StatusCode config_ut_engine_get_default_physics(EnginePhysicsConfig *config_out)
{
    if (g_cfg_mock_default_physics_status != STATUS_OK)
    {
        return g_cfg_mock_default_physics_status;
    }
    return engine_get_default_physics(config_out);
}

#define fopen config_ut_fopen
#define fread config_ut_fread
#define ferror config_ut_ferror
#define fclose config_ut_fclose
#define control_get_default_calibration config_ut_control_get_default_calibration
#define engine_get_default_physics config_ut_engine_get_default_physics
#define config_load_calibration_file config_load_calibration_file_internal_ut
#define config_load_physics_file config_load_physics_file_internal_ut
#include "../../src/app/config.c"
#undef config_load_physics_file
#undef config_load_calibration_file
#undef engine_get_default_physics
#undef control_get_default_calibration
#undef fclose
#undef ferror
#undef fread
#undef fopen

static void config_ut_reset_mocks(void)
{
    g_cfg_mock_content = "";
    g_cfg_mock_open_fail = 0;
    g_cfg_mock_read_error = 0;
    g_cfg_mock_close_error = 0;
    g_cfg_mock_default_cal_status = STATUS_OK;
    g_cfg_mock_default_physics_status = STATUS_OK;
}

static int32_t test_config_internal_find_json_key_null_args(void)
{
    ASSERT_TRUE(find_json_key((const char *)0, "x") == (const char *)0);
    ASSERT_TRUE(find_json_key("{}", (const char *)0) == (const char *)0);
    return 1;
}

static int32_t test_config_internal_find_json_key_escaped_char_path(void)
{
    const char *pos;
    pos = find_json_key("{\"note\":\"a\\\\b\",\"temperature_limit\":95}", "temperature_limit");
    ASSERT_TRUE(pos != (const char *)0);
    ASSERT_TRUE(*pos == ':');
    return 1;
}

static int32_t test_config_internal_parse_helpers_null_args(void)
{
    float fvalue = 0.0f;
    uint32_t uvalue = 0U;
    ASSERT_EQ(0, parse_float_field((const char *)0, "k", &fvalue));
    ASSERT_EQ(0, parse_uint_field((const char *)0, "k", &uvalue));
    ASSERT_EQ(0, validate_only_known_keys((const char *)0));
    return 1;
}

static int32_t test_config_internal_validate_malformed_quote_rejected(void)
{
    ASSERT_EQ(0, validate_only_known_keys("\"dangling"));
    return 1;
}

static int32_t test_config_internal_calibration_read_and_close_errors(void)
{
    ControlCalibration calibration;
    char err[128];

    config_ut_reset_mocks();
    g_cfg_mock_content = "{\"temperature_limit\":95,\"oil_pressure_limit\":2.5,\"persistence_ticks\":3}";
    g_cfg_mock_read_error = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  config_load_calibration_file_internal_ut("mock", &calibration, err, (uint32_t)sizeof(err)));

    config_ut_reset_mocks();
    g_cfg_mock_content = "{\"temperature_limit\":95,\"oil_pressure_limit\":2.5,\"persistence_ticks\":3}";
    g_cfg_mock_close_error = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  config_load_calibration_file_internal_ut("mock", &calibration, err, (uint32_t)sizeof(err)));
    return 1;
}

static int32_t test_config_internal_default_provider_internal_errors(void)
{
    ControlCalibration calibration;
    EnginePhysicsConfig physics;
    char err[128];

    config_ut_reset_mocks();
    g_cfg_mock_content = "{\"temperature_limit\":95,\"oil_pressure_limit\":2.5,\"persistence_ticks\":3}";
    g_cfg_mock_default_cal_status = STATUS_INTERNAL_ERROR;
    ASSERT_STATUS(STATUS_INTERNAL_ERROR,
                  config_load_calibration_file_internal_ut("mock", &calibration, err, (uint32_t)sizeof(err)));

    config_ut_reset_mocks();
    g_cfg_mock_default_physics_status = STATUS_INTERNAL_ERROR;
    ASSERT_STATUS(STATUS_INTERNAL_ERROR,
                  config_load_physics_file_internal_ut("mock", &physics, err, (uint32_t)sizeof(err)));
    return 1;
}

static int32_t test_config_internal_physics_open_and_read_errors(void)
{
    EnginePhysicsConfig physics;
    char err[128];

    config_ut_reset_mocks();
    g_cfg_mock_open_fail = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  config_load_physics_file_internal_ut("mock", &physics, err, (uint32_t)sizeof(err)));

    config_ut_reset_mocks();
    g_cfg_mock_content = "{\"physics\":{\"target_rpm\":3000}}";
    g_cfg_mock_read_error = 1;
    ASSERT_STATUS(STATUS_IO_ERROR,
                  config_load_physics_file_internal_ut("mock", &physics, err, (uint32_t)sizeof(err)));
    return 1;
}

int32_t register_config_internal_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"config_internal_find_null", test_config_internal_find_json_key_null_args},
        {"config_internal_find_escape", test_config_internal_find_json_key_escaped_char_path},
        {"config_internal_parse_null", test_config_internal_parse_helpers_null_args},
        {"config_internal_bad_quote", test_config_internal_validate_malformed_quote_rejected},
        {"config_internal_io_cal", test_config_internal_calibration_read_and_close_errors},
        {"config_internal_defaults_err", test_config_internal_default_provider_internal_errors},
        {"config_internal_io_phys", test_config_internal_physics_open_and_read_errors}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
