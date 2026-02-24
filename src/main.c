#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "logger.h"
#include "test_runner.h"

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
    char usage_line[96];
    int written;

    if (program_name == (const char *)0)
    {
        return ENGINE_ERROR;
    }

    if (safe_print("Usage:\n") != ENGINE_OK)
    {
        return ENGINE_ERROR;
    }

    written = snprintf(usage_line, sizeof(usage_line), "  %s --run-all\n", program_name);
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
                       "  %s --scenario <normal|overheat|pressure_failure>\n",
                       program_name);
    if ((written < 0) || (written >= (int)sizeof(usage_line)))
    {
        return ENGINE_ERROR;
    }

    return safe_print(usage_line);
}

int main(int argc, char **argv)
{
    size_t arg_len;

    if ((argv == (char **)0) || (argc < 1))
    {
        return 1;
    }

    if (argc == 2)
    {
        arg_len = strlen(argv[1]);
        if ((arg_len > 0U) && (arg_len < 24U) && (strncmp(argv[1], "--run-all", 24U) == 0))
        {
            return run_all_tests();
        }
    }

    if (argc == 3)
    {
        arg_len = strlen(argv[1]);
        if ((arg_len > 0U) && (arg_len < 24U) && (strncmp(argv[1], "--scenario", 24U) == 0))
        {
            int scenario_result = run_named_scenario(argv[2]);
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
