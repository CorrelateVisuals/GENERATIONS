#pragma once

#include <filesystem>

namespace CE::Implementation {

class ScriptChainerApp {
public:
  static void run(const std::filesystem::path &script_path);
};

} // namespace CE::Implementation
