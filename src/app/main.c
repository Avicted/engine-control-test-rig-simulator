#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "engine.h"
#include "reporting/logger.h"
#include "test_runner.h"
#include "status.h"
#include "version.h"

#define MAX_CLI_ARG_LEN 24U
#define MAX_CLI_PATH_LEN 192U
#define MAX_CLI_ARGS 12

static int32_t safe_print(const char *line)
{
    int32_t result;

    if (line == NULL)
    {
        return ENGINE_ERROR;
    }

    result = fputs(line, stdout);
    if (result < 0)
    {
        return ENGINE_ERROR;
    }

    return ENGINE_OK;
}

static int32_t print_usage(const char *program_name)
{
    char usage_line[256];
    int32_t written;

    if (program_name == NULL)
    {
        return ENGINE_ERROR;
    }

    if (safe_print("Usage:\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if (safe_print("  ") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }
    if (safe_print(program_name) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }
    if (safe_print(" --version\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    if (safe_print("  Global options: [--config calibration.json] [--log-level DEBUG|INFO|WARN|ERROR]\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(usage_line,
                       sizeof(usage_line),
                       "  %s --run-all [--show-sim] [--show-control] [--show-state] [--color] [--json] [--strict]\n",
                       program_name);
    if ((written < 0) || (written >= (int32_t)sizeof(usage_line)))
    {
        return ENGINE_ERROR;
    }
    if (safe_print(usage_line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(usage_line,
                       sizeof(usage_line),
                       "  %s --scenario <normal|overheat|pressure_failure|cold_start|high_load|oil_drain|thermal_runaway|intermittent_oil> [--show-sim] [--show-control] [--show-state] [--color] [--json] [--strict]\n",
                       program_name);
    if ((written < 0) || (written >= (int32_t)sizeof(usage_line)))
    {
        return ENGINE_ERROR;
    }
    if (safe_print(usage_line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(usage_line,
                       sizeof(usage_line),
                       "  %s --script <path> [--show-sim] [--show-control] [--show-state] [--color] [--json] [--strict]\n",
                       program_name);
    if ((written < 0) || (written >= (int32_t)sizeof(usage_line)))
    {
        return ENGINE_ERROR;
    }

    return safe_print(usage_line);
}

static StatusCode parse_optional_flags(int32_t argc,
                                       char **argv,
                                       int32_t start_index,
                                       int32_t *show_sim,
                                       int32_t *use_color,
                                       int32_t *show_control,
                                       int32_t *show_state,
                                       int32_t *json_output,
                                       int32_t *strict_mode,
                                       const char **config_path,
                                       const char **log_level)
{

    if ((argv == NULL) || (show_sim == NULL) || (use_color == NULL) ||
        (show_control == NULL) || (show_state == NULL) || (json_output == NULL) ||
        (strict_mode == NULL) || (config_path == NULL) || (log_level == NULL))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (argc > MAX_CLI_ARGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    *show_sim = 0;
    *use_color = 0;
    *show_control = 0;
    *show_state = 0;
    *json_output = 0;
    *strict_mode = 0;
    *config_path = NULL;
    *log_level = "INFO";

    {
        int32_t index;
        int32_t skip_next = 0;
        for (index = start_index; (index < argc) && (index < MAX_CLI_ARGS); ++index)
        {
            size_t arg_len;

            if (skip_next != 0)
            {
                skip_next = 0;
                continue;
            }

            if (argv[index] == NULL)
            {
                return STATUS_INVALID_ARGUMENT;
            }

            if (strcmp(argv[index], "--config") == 0)
            {
                if ((*config_path != NULL) || ((index + 1) >= argc))
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                if ((strlen(argv[index + 1]) == 0U) || (strlen(argv[index + 1]) >= MAX_CLI_PATH_LEN))
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *config_path = argv[index + 1];
                skip_next = 1;
                continue;
            }

            if (strcmp(argv[index], "--log-level") == 0)
            {
                if ((index + 1) >= argc)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                if ((strcmp(argv[index + 1], "DEBUG") != 0) && (strcmp(argv[index + 1], "INFO") != 0) &&
                    (strcmp(argv[index + 1], "WARN") != 0) && (strcmp(argv[index + 1], "ERROR") != 0))
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *log_level = argv[index + 1];
                skip_next = 1;
                continue;
            }

            arg_len = strlen(argv[index]);
            if ((arg_len == 0U) || (arg_len >= MAX_CLI_ARG_LEN))
            {
                return STATUS_INVALID_ARGUMENT;
            }

            if (strncmp(argv[index], "--show-sim", MAX_CLI_ARG_LEN) == 0)
            {
                if (*show_sim != 0)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *show_sim = 1;
            }
            else if (strncmp(argv[index], "--show-control", MAX_CLI_ARG_LEN) == 0)
            {
                if (*show_control != 0)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *show_control = 1;
            }
            else if (strncmp(argv[index], "--color", MAX_CLI_ARG_LEN) == 0)
            {
                if (*use_color != 0)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *use_color = 1;
            }
            else if (strncmp(argv[index], "--show-state", MAX_CLI_ARG_LEN) == 0)
            {
                if (*show_state != 0)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *show_state = 1;
            }
            else if (strncmp(argv[index], "--json", MAX_CLI_ARG_LEN) == 0)
            {
                if (*json_output != 0)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *json_output = 1;
            }
            else if (strncmp(argv[index], "--strict", MAX_CLI_ARG_LEN) == 0)
            {
                if (*strict_mode != 0)
                {
                    return STATUS_INVALID_ARGUMENT;
                }
                *strict_mode = 1;
            }
            else
            {
                return STATUS_INVALID_ARGUMENT;
            }
        }
    } /* end skip_next scope */

    return STATUS_OK;
}

static StatusCode apply_runtime_options(const char *config_path, const char *log_level)
{
    ControlCalibration calibration;
    char error_message[256];

    if (logger_set_level_from_string(log_level) != STATUS_OK)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (config_path == NULL)
    {
        return STATUS_OK;
    }

    if (config_load_calibration_file(config_path, &calibration, error_message, (uint32_t)sizeof(error_message)) != STATUS_OK)
    {
        (void)log_event("ERROR", error_message);
        return STATUS_PARSE_ERROR;
    }

    if (control_configure_calibration(&calibration) != STATUS_OK)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

int main(int argc, char **argv)
{
    int32_t show_sim;
    int32_t use_color;
    int32_t show_control;
    int32_t show_state;
    int32_t json_output;
    int32_t strict_mode;
    const char *config_path = NULL;
    const char *log_level = "INFO";

    if ((argv == NULL) || (argc < 1))
    {
        return 1;
    }

    if (argc > MAX_CLI_ARGS)
    {
        (void)print_usage(argv[0]);
        return 1;
    }

    if ((argc == 2) && (strcmp(argv[1], "--version") == 0))
    {
        (void)printf("%s\n", SIM_SOFTWARE_VERSION);
        return 0;
    }

    if (argc >= 2)
    {
        size_t arg_len;

        arg_len = strlen(argv[1]);
        if ((arg_len > 0U) && (arg_len < MAX_CLI_ARG_LEN) &&
            (strncmp(argv[1], "--run-all", MAX_CLI_ARG_LEN) == 0))
        {
            StatusCode run_status;

            if (parse_optional_flags(argc,
                                     argv,
                                     2,
                                     &show_sim,
                                     &use_color,
                                     &show_control,
                                     &show_state,
                                     &json_output,
                                     &strict_mode,
                                     &config_path,
                                     &log_level) != STATUS_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }

            if (apply_runtime_options(config_path, log_level) != STATUS_OK)
            {
                return 1;
            }

            run_status = run_all_tests_with_json(show_sim, use_color, show_control, show_state, json_output);
            return (run_status == STATUS_OK) ? 0 : 1;
        }
    }

    if (argc >= 3)
    {
        size_t arg_len;

        arg_len = strlen(argv[1]);
        if ((arg_len > 0U) && (arg_len < MAX_CLI_ARG_LEN) &&
            (strncmp(argv[1], "--scenario", MAX_CLI_ARG_LEN) == 0))
        {
            StatusCode scenario_status;

            if (parse_optional_flags(argc,
                                     argv,
                                     3,
                                     &show_sim,
                                     &use_color,
                                     &show_control,
                                     &show_state,
                                     &json_output,
                                     &strict_mode,
                                     &config_path,
                                     &log_level) != STATUS_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }

            if (apply_runtime_options(config_path, log_level) != STATUS_OK)
            {
                return 1;
            }

            scenario_status = run_named_scenario_with_json(argv[2],
                                                           show_sim,
                                                           use_color,
                                                           show_control,
                                                           show_state,
                                                           json_output);
            if (scenario_status != STATUS_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }
            return 0;
        }

        if ((arg_len > 0U) && (arg_len < MAX_CLI_ARG_LEN) &&
            (strncmp(argv[1], "--script", MAX_CLI_ARG_LEN) == 0))
        {
            StatusCode script_status;

            if (parse_optional_flags(argc,
                                     argv,
                                     3,
                                     &show_sim,
                                     &use_color,
                                     &show_control,
                                     &show_state,
                                     &json_output,
                                     &strict_mode,
                                     &config_path,
                                     &log_level) != STATUS_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }

            if (apply_runtime_options(config_path, log_level) != STATUS_OK)
            {
                return 1;
            }

            script_status = run_scripted_scenario_with_json(argv[2],
                                                            show_sim,
                                                            use_color,
                                                            show_control,
                                                            show_state,
                                                            json_output,
                                                            strict_mode);
            if (script_status != STATUS_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }
            return 0;
        }
    }

    if (print_usage(argv[0]) != ENGINE_OK)
    {
        if (log_event("ERROR", "Failed to print usage") != STATUS_OK)
        {
            return 1;
        }
    }
    return 1;
}
