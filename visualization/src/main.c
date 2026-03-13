#include <stdio.h>
#include <string.h>

#include "visualizer_app.h"
#include "visualizer_loader.h"

static void print_usage(const char *program_name)
{
    (void)fprintf(stderr,
                  "Usage: %s [--theme default|dos|onyx|gruvbox|light] "
                  "<scenarios.json> [more_scenarios.json ...]\n",
                  program_name);
}

int main(int argc, char **argv)
{
    ScenarioSet scenario_set = {0};
    VisualizerThemeId initial_theme = VISUALIZER_THEME_DEFAULT;
    int arg_index = 0;
    int scenario_argc = 1;
    const char *program_name = "visualizer";

    if ((argv != NULL) && (argc > 0) && (argv[0] != NULL)) {
        program_name = argv[0];
    }

    if ((argc < 2) || (argv == NULL)) {
        print_usage(program_name);
        return 1;
    }

    for (arg_index = 1; arg_index < argc; ++arg_index) {
        if ((strcmp(argv[arg_index], "--theme") == 0) && ((arg_index + 1) < argc)) {
            if (visualizer_parse_theme_id(argv[arg_index + 1], &initial_theme) == 0) {
                (void)fprintf(stderr,
                              "Unknown theme '%s'. Supported "
                              "themes: default, dos, "
                              "onyx, gruvbox, light.\n",
                              argv[arg_index + 1]);
                return 1;
            }
            ++arg_index;
            continue;
        }

        if (strcmp(argv[arg_index], "--theme") == 0) {
            (void)fprintf(stderr, "Missing value after --theme.\n");
            return 1;
        }

        argv[scenario_argc] = argv[arg_index];
        ++scenario_argc;
    }

    if (scenario_argc < 2) {
        print_usage(program_name);
        return 1;
    }

    if (visualizer_load_scenarios_from_files(scenario_argc, argv, &scenario_set) == 0) {
        (void)fprintf(stderr, "Failed to load scenario JSON file(s).\n");
        return 1;
    }

    visualizer_run(&scenario_set, initial_theme);
    return 0;
}
