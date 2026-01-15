#pragma once

// Spec traceability hooks. These are no-ops at runtime but keep rule IDs visible in source.
#define SPEC_RULE(id) do { (void)sizeof(id); } while (0)
#define SPEC_DEF(name, section) do { (void)sizeof(name); (void)sizeof(section); } while (0)
#define SPEC_COV(id) do { (void)sizeof(id); } while (0)

