#pragma once

#include "ShaderGraph.h"
#include "core/RuntimeConfig.h"

namespace CE::Implementation {

CE::Runtime::PipelineExecutionPlan build_execution_plan(const ShaderGraph &graph);

} // namespace CE::Implementation
