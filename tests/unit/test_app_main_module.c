#include <stdio.h>
#include <string.h>

#include "control.h"
#include "test_harness.h"

static int32_t g_mock_fputs_enabled = 0;
static int32_t g_mock_fputs_fail_after = -1;

static int mock_main_fputs(const char *line, FILE *stream)
{
    size_t line_len;
    size_t written;

    if (g_mock_fputs_enabled != 0)
    {
        if (g_mock_fputs_fail_after <= 0)
        {
            return -1;
        }
        g_mock_fputs_fail_after -= 1;
    }

    line_len = strlen(line);
    written = fwrite(line, 1U, line_len, stream);
    if (written != line_len)
    {
        return -1;
    }

    return 1;
}

#define fputs mock_main_fputs
#define main app_main_entry
#include "../../src/app/main.c"
#undef main
#undef fputs

static StatusCode stub_run_all_status = STATUS_OK;
static StatusCode stub_run_named_status = STATUS_OK;
static StatusCode stub_run_script_status = STATUS_OK;

static int32_t last_show_sim = 0;
static int32_t last_use_color = 0;
static int32_t last_show_control = 0;
static int32_t last_show_state = 0;
static int32_t last_json_output = 0;
static int32_t last_strict_mode = 0;
static char last_name[128];

StatusCode run_all_tests_with_json(int32_t show_sim,
                                   int32_t use_color,
                                   int32_t show_control,
                                   int32_t show_state,
                                   int32_t json_output)
{
    last_show_sim = show_sim;
    last_use_color = use_color;
    last_show_control = show_control;
    last_show_state = show_state;
    last_json_output = json_output;
    return stub_run_all_status;
}

StatusCode run_named_scenario_with_json(const char *name,
                                        int32_t show_sim,
                                        int32_t use_color,
                                        int32_t show_control,
                                        int32_t show_state,
                                        int32_t json_output)
{
    (void)snprintf(last_name, sizeof(last_name), "%s", (name == NULL) ? "" : name);
    last_show_sim = show_sim;
    last_use_color = use_color;
    last_show_control = show_control;
    last_show_state = show_state;
    last_json_output = json_output;
    return stub_run_named_status;
}

StatusCode run_scripted_scenario_with_json(const char *script_path,
                                           int32_t show_sim,
                                           int32_t use_color,
                                           int32_t show_control,
                                           int32_t show_state,
                                           int32_t json_output,
                                           int32_t strict_mode)
{
    (void)snprintf(last_name, sizeof(last_name), "%s", (script_path == NULL) ? "" : script_path);
    last_show_sim = show_sim;
    last_use_color = use_color;
    last_show_control = show_control;
    last_show_state = show_state;
    last_json_output = json_output;
    last_strict_mode = strict_mode;
    return stub_run_script_status;
}

static int32_t write_calibration_file(const char *path)
{
    FILE *file;

    file = fopen(path, "w");
    if (file == (FILE *)0)
    {
        return 0;
    }

    if (fputs("{\n"
              "  \"temperature_limit\": 95.0,\n"
              "  \"oil_pressure_limit\": 2.5,\n"
              "  \"persistence_ticks\": 3\n"
              "}\n",
              file) < 0)
    {
        (void)fclose(file);
        return 0;
    }

    return (fclose(file) == 0) ? 1 : 0;
}

static int32_t test_safe_print_null_fails(void)
{
    ASSERT_EQ(ENGINE_ERROR, safe_print((const char *)0));
    return 1;
}

static int32_t test_safe_print_fputs_error_fails(void)
{
    g_mock_fputs_enabled = 1;
    g_mock_fputs_fail_after = 0;
    ASSERT_EQ(ENGINE_ERROR, safe_print("x"));
    g_mock_fputs_enabled = 0;
    g_mock_fputs_fail_after = -1;
    return 1;
}

static int32_t test_print_usage_null_fails(void)
{
    ASSERT_EQ(ENGINE_ERROR, print_usage((const char *)0));
    return 1;
}

static int32_t test_print_usage_long_name_fails(void)
{
    char long_name[300];
    uint32_t i;

    for (i = 0U; i < ((uint32_t)sizeof(long_name) - 1U); ++i)
    {
        long_name[i] = 'A';
    }
    long_name[sizeof(long_name) - 1U] = '\0';

    ASSERT_EQ(ENGINE_ERROR, print_usage(long_name));
    return 1;
}

static int32_t test_print_usage_each_safe_print_failure_path(void)
{
    int32_t fail_position;

    for (fail_position = 0; fail_position < 8; ++fail_position)
    {
        g_mock_fputs_enabled = 1;
        g_mock_fputs_fail_after = fail_position;
        ASSERT_EQ(ENGINE_ERROR, print_usage("prog"));
    }

    g_mock_fputs_enabled = 0;
    g_mock_fputs_fail_after = -1;
    return 1;
}

static int32_t test_parse_optional_flags_null_args(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(2,
                                       (char **)0,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));
    return 1;
}

static int32_t test_parse_optional_flags_success_all(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char *argv[] = {(char *)"prog",
                    (char *)"--show-sim",
                    (char *)"--show-control",
                    (char *)"--color",
                    (char *)"--show-state",
                    (char *)"--json",
                    (char *)"--strict"};

    ASSERT_STATUS(STATUS_OK,
                  parse_optional_flags(7,
                                       argv,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_EQ(1, show_sim);
    ASSERT_EQ(1, use_color);
    ASSERT_EQ(1, show_control);
    ASSERT_EQ(1, show_state);
    ASSERT_EQ(1, json_output);
    ASSERT_EQ(1, strict_mode);
    ASSERT_TRUE(config_path == (const char *)0);
    ASSERT_TRUE(strcmp(log_level, "INFO") == 0);
    return 1;
}

static int32_t test_parse_optional_flags_config_and_log_level(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char *argv[] = {(char *)"prog", (char *)"--config", (char *)"calibration.json", (char *)"--log-level", (char *)"WARN"};

    ASSERT_STATUS(STATUS_OK,
                  parse_optional_flags(5,
                                       argv,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_TRUE(strcmp(config_path, "calibration.json") == 0);
    ASSERT_TRUE(strcmp(log_level, "WARN") == 0);
    return 1;
}

static int32_t test_parse_optional_flags_duplicate_flag_rejected(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char *argv[] = {(char *)"prog", (char *)"--show-sim", (char *)"--show-sim"};

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));
    return 1;
}

static int32_t test_parse_optional_flags_invalid_tokens_rejected(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char *argv_bad_level[] = {(char *)"prog", (char *)"--log-level", (char *)"TRACE"};
    char *argv_missing_level[] = {(char *)"prog", (char *)"--log-level"};
    char *argv_missing_config[] = {(char *)"prog", (char *)"--config"};
    char *argv_unknown[] = {(char *)"prog", (char *)"--what"};
    char *argv_null_item[] = {(char *)"prog", (char *)"--json", (char *)0};

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_bad_level,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(2,
                                       argv_missing_level,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(2,
                                       argv_missing_config,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(2,
                                       argv_unknown,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_null_item,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));
    return 1;
}

static int32_t test_parse_optional_flags_config_path_validation(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char long_path[220];
    uint32_t i;
    char *argv_empty[] = {(char *)"prog", (char *)"--config", (char *)""};
    char *argv_dup[] = {(char *)"prog", (char *)"--config", (char *)"a.json", (char *)"--config", (char *)"b.json"};

    for (i = 0U; i < ((uint32_t)sizeof(long_path) - 1U); ++i)
    {
        long_path[i] = 'x';
    }
    long_path[sizeof(long_path) - 1U] = '\0';

    {
        char *argv_long[] = {(char *)"prog", (char *)"--config", long_path};
        ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                      parse_optional_flags(3,
                                           argv_long,
                                           1,
                                           &show_sim,
                                           &use_color,
                                           &show_control,
                                           &show_state,
                                           &json_output,
                                           &strict_mode,
                                           &config_path,
                                           &log_level));
    }

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_empty,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(5,
                                       argv_dup,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));
    return 1;
}

static int32_t test_parse_optional_flags_more_invalid_branches(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char *argv_many[] = {(char *)"prog", (char *)"--show-sim", (char *)"--show-control", (char *)"--color", (char *)"--show-state",
                         (char *)"--json", (char *)"--strict", (char *)"--config", (char *)"a", (char *)"--log-level", (char *)"INFO",
                         (char *)"--show-sim", (char *)"--color"};
    char *argv_empty_token[] = {(char *)"prog", (char *)""};
    char long_token[40];
    uint32_t i;

    for (i = 0U; i < ((uint32_t)sizeof(long_token) - 1U); ++i)
    {
        long_token[i] = 'z';
    }
    long_token[sizeof(long_token) - 1U] = '\0';

    {
        char *argv_long_token[] = {(char *)"prog", long_token};
        ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                      parse_optional_flags(2,
                                           argv_long_token,
                                           1,
                                           &show_sim,
                                           &use_color,
                                           &show_control,
                                           &show_state,
                                           &json_output,
                                           &strict_mode,
                                           &config_path,
                                           &log_level));
    }

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(13,
                                       argv_many,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(2,
                                       argv_empty_token,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));
    return 1;
}

static int32_t test_parse_optional_flags_duplicate_secondary_flags(void)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path;
    const char *log_level;
    char *argv_dup_control[] = {(char *)"prog", (char *)"--show-control", (char *)"--show-control"};
    char *argv_dup_color[] = {(char *)"prog", (char *)"--color", (char *)"--color"};
    char *argv_dup_state[] = {(char *)"prog", (char *)"--show-state", (char *)"--show-state"};
    char *argv_dup_strict[] = {(char *)"prog", (char *)"--strict", (char *)"--strict"};

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_dup_control,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_dup_color,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_dup_state,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  parse_optional_flags(3,
                                       argv_dup_strict,
                                       1,
                                       &show_sim,
                                       &use_color,
                                       &show_control,
                                       &show_state,
                                       &json_output,
                                       &strict_mode,
                                       &config_path,
                                       &log_level));
    return 1;
}

static int32_t test_apply_runtime_options_paths(void)
{
    const char *path = "build/unit_main_runtime_calibration.json";

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  apply_runtime_options((const char *)0, "NOT_A_LEVEL"));

    ASSERT_STATUS(STATUS_OK,
                  apply_runtime_options((const char *)0, "INFO"));

    ASSERT_STATUS(STATUS_PARSE_ERROR,
                  apply_runtime_options("build/no_such_config_for_main.json", "INFO"));

    ASSERT_TRUE(write_calibration_file(path) != 0);
    ASSERT_STATUS(STATUS_OK,
                  apply_runtime_options(path, "DEBUG"));

    ASSERT_STATUS(STATUS_INVALID_ARGUMENT,
                  apply_runtime_options(path, "DEBUG"));

    ASSERT_STATUS(STATUS_OK, control_reset_calibration());
    return 1;
}

static int32_t test_app_main_version_and_invalid_inputs(void)
{
    char *argv_version[] = {(char *)"prog", (char *)"--version"};
    char *argv_unknown[] = {(char *)"prog", (char *)"--unknown"};
    char *argv_too_many[] = {(char *)"prog", (char *)"a", (char *)"b", (char *)"c", (char *)"d", (char *)"e", (char *)"f",
                             (char *)"g", (char *)"h", (char *)"i", (char *)"j", (char *)"k", (char *)"l"};

    ASSERT_EQ(1, app_main_entry(0, (char **)0));
    ASSERT_EQ(0, app_main_entry(2, argv_version));
    ASSERT_EQ(1, app_main_entry(2, argv_unknown));
    ASSERT_EQ(1, app_main_entry(13, argv_too_many));
    return 1;
}

static int32_t test_app_main_run_all_paths(void)
{
    char *argv_ok[] = {(char *)"prog", (char *)"--run-all", (char *)"--show-sim", (char *)"--color", (char *)"--show-control",
                       (char *)"--show-state", (char *)"--json", (char *)"--strict", (char *)"--log-level", (char *)"ERROR"};
    char *argv_bad_flags[] = {(char *)"prog", (char *)"--run-all", (char *)"--json", (char *)"--json"};
    char *argv_bad_runtime[] = {(char *)"prog", (char *)"--run-all", (char *)"--config", (char *)"build/no_such_file.json"};

    stub_run_all_status = STATUS_OK;
    ASSERT_EQ(0, app_main_entry(10, argv_ok));
    ASSERT_EQ(1, last_show_sim);
    ASSERT_EQ(1, last_use_color);
    ASSERT_EQ(1, last_show_control);
    ASSERT_EQ(1, last_show_state);
    ASSERT_EQ(1, last_json_output);

    stub_run_all_status = STATUS_INTERNAL_ERROR;
    ASSERT_EQ(1, app_main_entry(10, argv_ok));

    ASSERT_EQ(1, app_main_entry(4, argv_bad_flags));
    ASSERT_EQ(1, app_main_entry(4, argv_bad_runtime));
    return 1;
}

static int32_t test_app_main_scenario_paths(void)
{
    char *argv_ok[] = {(char *)"prog", (char *)"--scenario", (char *)"normal", (char *)"--show-sim", (char *)"--color",
                       (char *)"--show-control", (char *)"--show-state", (char *)"--json"};
    char *argv_fail[] = {(char *)"prog", (char *)"--scenario", (char *)"bad_name"};

    stub_run_named_status = STATUS_OK;
    ASSERT_EQ(0, app_main_entry(8, argv_ok));
    ASSERT_TRUE(strcmp(last_name, "normal") == 0);

    stub_run_named_status = STATUS_PARSE_ERROR;
    ASSERT_EQ(1, app_main_entry(3, argv_fail));

    {
        char *argv_bad_flags[] = {(char *)"prog", (char *)"--scenario", (char *)"normal", (char *)"--json", (char *)"--json"};
        char *argv_bad_runtime[] = {(char *)"prog", (char *)"--scenario", (char *)"normal", (char *)"--config", (char *)"build/no_such_cfg_main.json"};
        ASSERT_EQ(1, app_main_entry(5, argv_bad_flags));
        ASSERT_EQ(1, app_main_entry(5, argv_bad_runtime));
    }

    return 1;
}

static int32_t test_app_main_script_paths(void)
{
    char *argv_ok[] = {(char *)"prog", (char *)"--script", (char *)"scenarios/normal_operation.txt", (char *)"--strict", (char *)"--json"};
    char *argv_fail[] = {(char *)"prog", (char *)"--script", (char *)"scenarios/nope.txt"};

    stub_run_script_status = STATUS_OK;
    ASSERT_EQ(0, app_main_entry(5, argv_ok));
    ASSERT_TRUE(strstr(last_name, "normal_operation") != (char *)0);
    ASSERT_EQ(1, last_strict_mode);

    stub_run_script_status = STATUS_PARSE_ERROR;
    ASSERT_EQ(1, app_main_entry(3, argv_fail));

    {
        char *argv_bad_flags[] = {(char *)"prog", (char *)"--script", (char *)"scenarios/normal_operation.txt", (char *)"--strict", (char *)"--strict"};
        char *argv_bad_runtime[] = {(char *)"prog", (char *)"--script", (char *)"scenarios/normal_operation.txt", (char *)"--config", (char *)"build/no_such_cfg_script.json"};
        ASSERT_EQ(1, app_main_entry(5, argv_bad_flags));
        ASSERT_EQ(1, app_main_entry(5, argv_bad_runtime));
    }

    return 1;
}

static int32_t test_app_main_unknown_three_args_and_null_prog_name(void)
{
    char *argv_unknown_three[] = {(char *)"prog", (char *)"--not-a-command", (char *)"x"};
    char *argv_null_prog[] = {(char *)0, (char *)"--not-a-command", (char *)"x"};

    ASSERT_EQ(1, app_main_entry(3, argv_unknown_three));
    ASSERT_EQ(1, app_main_entry(3, argv_null_prog));
    return 1;
}

int32_t register_app_main_tests(const UnitTestCase **tests_out, uint32_t *count_out)
{
    static const UnitTestCase tests[] = {
        {"main_safe_print_null", test_safe_print_null_fails},
        {"main_safe_print_io", test_safe_print_fputs_error_fails},
        {"main_print_usage_null", test_print_usage_null_fails},
        {"main_print_usage_long", test_print_usage_long_name_fails},
        {"main_print_usage_io_paths", test_print_usage_each_safe_print_failure_path},
        {"main_parse_flags_null", test_parse_optional_flags_null_args},
        {"main_parse_flags_success", test_parse_optional_flags_success_all},
        {"main_parse_flags_cfg_level", test_parse_optional_flags_config_and_log_level},
        {"main_parse_flags_duplicate", test_parse_optional_flags_duplicate_flag_rejected},
        {"main_parse_flags_invalid", test_parse_optional_flags_invalid_tokens_rejected},
        {"main_parse_flags_cfg_path", test_parse_optional_flags_config_path_validation},
        {"main_parse_flags_more_invalid", test_parse_optional_flags_more_invalid_branches},
        {"main_parse_flags_dup_secondary", test_parse_optional_flags_duplicate_secondary_flags},
        {"main_runtime_options", test_apply_runtime_options_paths},
        {"main_entry_basic", test_app_main_version_and_invalid_inputs},
        {"main_entry_run_all", test_app_main_run_all_paths},
        {"main_entry_scenario", test_app_main_scenario_paths},
        {"main_entry_script", test_app_main_script_paths},
        {"main_entry_unknown_three", test_app_main_unknown_three_args_and_null_prog_name}};

    if ((tests_out == (const UnitTestCase **)0) || (count_out == (uint32_t *)0))
    {
        return 0;
    }

    *tests_out = tests;
    *count_out = (uint32_t)(sizeof(tests) / sizeof(tests[0]));
    return 1;
}
