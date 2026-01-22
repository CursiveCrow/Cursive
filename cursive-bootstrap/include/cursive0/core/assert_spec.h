#pragma once

#include "cursive0/core/spec_trace.h"

// Spec traceability hooks. These are no-ops unless tracing is enabled.
#define SPEC_RULE(id) do { (void)sizeof(id); cursive0::core::SpecTrace::Record(id); } while (0)
#define SPEC_DEF(name, section) do { (void)sizeof(name); (void)sizeof(section); } while (0)
#define SPEC_COV(id) do { (void)sizeof(id); } while (0)

