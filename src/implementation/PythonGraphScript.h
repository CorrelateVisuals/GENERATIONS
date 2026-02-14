#pragma once

#include "ShaderGraph.h"

#include <filesystem>

namespace CE::Implementation {

class PythonGraphScript {
public:
  static ShaderGraph load(const std::filesystem::path &script_path);
};

} // namespace CE::Implementation
