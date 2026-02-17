#include <glm/glm.hpp>

#include "Library.h"
#include "base/RuntimeConfig.h"

std::string Lib::path(const std::string &linux_path) {
#ifdef _WIN32
  std::string converted_windows_path = "..\\" + linux_path;
  for (char &c : converted_windows_path) {
    if (c == '/') {
      c = '\\';
    }
  }
  if (converted_windows_path.rfind("..\\", 0) == 0) {
    converted_windows_path = converted_windows_path.substr(3);
  }
  return if_shader_compile(converted_windows_path);
#else
  return if_shader_compile(linux_path);
#endif
}

std::string Lib::if_shader_compile(std::string shader_path) {
  if (shader_path.find("shaders") == std::string::npos) {
    return shader_path;
  } else {
#ifdef _WIN32
    std::string glslang_validator = "glslangValidator.exe -V -Ishaders ";
#else
    std::string glslang_validator = "glslangValidator -V -Ishaders ";
#endif
    if (CE::Runtime::env_flag_enabled("CE_DEBUG_ENABLE_CELL_INSTANCE_VERTEX_SANITIZATION_GUARDS")) {
      glslang_validator += "-DCE_DEBUG_ENABLE_CELL_INSTANCE_VERTEX_SANITIZATION_GUARDS=1 ";
    }
    shader_path.insert(0, glslang_validator);
    return shader_path;
  }
}
