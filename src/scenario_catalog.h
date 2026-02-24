#ifndef SCENARIO_CATALOG_H
#define SCENARIO_CATALOG_H

#include "test_runner.h"

const TestCase *scenario_catalog_tests(void);
uint32_t scenario_catalog_count(void);
const TestCase *scenario_catalog_find_named(const char *name);

#endif
