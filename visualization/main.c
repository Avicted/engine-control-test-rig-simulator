#include <stdio.h>

#include "visualizer_app.h"
#include "visualizer_loader.h"

int main(int argc, char **argv)
{
    ScenarioSet scenario_set;

    if ((argc < 2) || (argv == NULL) || (argv[1] == NULL))
    {
        (void)fprintf(stderr, "Usage: %s <scenario.json> [more_scenarios.json ...]\n", (argc > 0) ? argv[0] : "visualizer");
        return 1;
    }

    if (visualizer_load_scenarios_from_files(argc, argv, &scenario_set) == 0)
    {
        (void)fprintf(stderr, "Failed to load scenario JSON file(s).\n");
        return 1;
    }

    visualizer_run(&scenario_set);
    return 0;
}