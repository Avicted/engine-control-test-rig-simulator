#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "logger.h"
#include "test_runner.h"

#define MAX_CLI_ARG_LEN 24U
#define MAX_CLI_ARGS 8

static int safe_print(const char *line)
{
    int result;

    if (line == (const char *)0)
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

static int print_usage(const char *program_name)
{
    char usage_line[128];
    int written;

    if (program_name == (const char *)0)
    {
        return ENGINE_ERROR;
    }

    if (safe_print("Usage:\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(usage_line,
                       sizeof(usage_line),
                       "  %s --run-all [--show-sim] [--show-control] [--color]\n",
                       program_name);
    if ((written < 0) || (written >= (int)sizeof(usage_line)))
    {
        return ENGINE_ERROR;
    }
    if (safe_print(usage_line) != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(usage_line,
                       sizeof(usage_line),
                       "  %s --scenario <normal|overheat|pressure_failure> [--show-sim] [--show-control] [--color]\n",
                       program_name);
    if ((written < 0) || (written >= (int)sizeof(usage_line)))
    {
        return ENGINE_ERROR;
    }

    return safe_print(usage_line);
}

static int parse_optional_flags(int argc,
                                char **argv,
                                int start_index,
                                int *show_sim,
                                int *use_color,
                                int *show_control)
{
    int index;
    size_t arg_len;

    if ((argv == (char **)0) || (show_sim == (int *)0) || (use_color == (int *)0) ||
        (show_control == (int *)0))
    {
        return ENGINE_ERROR;
    }

    if (argc > MAX_CLI_ARGS)
    {
        return ENGINE_ERROR;
    }

    *show_sim = 0;
    *use_color = 0;
    *show_control = 0;

    for (index = start_index; (index < argc) && (index < MAX_CLI_ARGS); ++index)
    {
        if (argv[index] == (char *)0)
        {
            return ENGINE_ERROR;
        }

        arg_len = strlen(argv[index]);
        if ((arg_len == 0U) || (arg_len >= MAX_CLI_ARG_LEN))
        {
            return ENGINE_ERROR;
        }

        if (strncmp(argv[index], "--show-sim", MAX_CLI_ARG_LEN) == 0)
        {
            if (*show_sim != 0)
            {
                return ENGINE_ERROR;
            }
            *show_sim = 1;
        }
        else if (strncmp(argv[index], "--show-control", MAX_CLI_ARG_LEN) == 0)
        {
            if (*show_control != 0)
            {
                return ENGINE_ERROR;
            }
            *show_control = 1;
        }
        else if (strncmp(argv[index], "--color", MAX_CLI_ARG_LEN) == 0)
        {
            if (*use_color != 0)
            {
                return ENGINE_ERROR;
            }
            *use_color = 1;
        }
        else
        {
            return ENGINE_ERROR;
        }
    }

    return ENGINE_OK;
}

int main(int argc, char **argv)
{
    size_t arg_len;
    int show_sim;
    int use_color;
    int show_control;

    if ((argv == (char **)0) || (argc < 1))
    {
        return 1;
    }

    if (argc > MAX_CLI_ARGS)
    {
        (void)print_usage(argv[0]);
        return 1;
    }

    if (argc >= 2)
    {
        arg_len = strlen(argv[1]);
        if ((arg_len > 0U) && (arg_len < MAX_CLI_ARG_LEN) &&
            (strncmp(argv[1], "--run-all", MAX_CLI_ARG_LEN) == 0))
        {
            if (parse_optional_flags(argc, argv, 2, &show_sim, &use_color, &show_control) != ENGINE_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }
            return run_all_tests_with_full_options(show_sim, use_color, show_control);
        }
    }

    if (argc >= 3)
    {
        arg_len = strlen(argv[1]);
        if ((arg_len > 0U) && (arg_len < MAX_CLI_ARG_LEN) &&
            (strncmp(argv[1], "--scenario", MAX_CLI_ARG_LEN) == 0))
        {
            int scenario_result;

            if (parse_optional_flags(argc, argv, 3, &show_sim, &use_color, &show_control) != ENGINE_OK)
            {
                (void)print_usage(argv[0]);
                return 1;
            }

            scenario_result = run_named_scenario_with_full_options(argv[2], show_sim, use_color, show_control);
            if (scenario_result == ENGINE_ERROR)
            {
                (void)print_usage(argv[0]);
                return 1;
            }
            return 0;
        }
    }

    if (print_usage(argv[0]) != ENGINE_OK)
    {
        if (log_event("ERROR", "Failed to print usage") != 0)
        {
            return 1;
        }
    }
    return 1;
}
