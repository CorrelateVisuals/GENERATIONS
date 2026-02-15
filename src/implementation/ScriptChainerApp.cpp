#include "ScriptChainerApp.h"

#include "ImplementationSpec.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"

namespace CE::Implementation {

void ScriptChainerApp::run() {
  Log::text(Log::Style::header_guard);
  Log::text("{ C++ Graph Interface }");

  const ImplementationSpec spec = default_spec();
  CE::Runtime::set_terrain_settings(spec.terrain);
  CE::Runtime::set_world_settings(spec.world);
  CE::Runtime::set_pipeline_definitions(spec.pipelines);
  CE::Runtime::set_render_graph(spec.render_graph);
  CE::Runtime::set_graphics_draw_ops(spec.draw_ops);

  Log::text("Runtime implementation config installed (pure C++).");
  Log::text(Log::Style::header_guard);
}

} // namespace CE::Implementation
