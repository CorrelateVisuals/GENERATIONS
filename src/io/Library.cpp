#include <glm/glm.hpp>

#include "Library.h"

std::string Lib::path(const std::string &linux_path) {
#ifdef _WIN32
  std::string converted_windows_path = "..\\" + linux_path;
  for (char &c : converted_windows_path) {
    if (c == '/') {
      c = '\\';
    }
  }
  if (converted_windows_path.starts_with("..\\")) {
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
  const std::string glslang_validator = "glslangValidator.exe -V ";
#else
    const std::string glslang_validator = "glslangValidator -V ";
#endif
    shader_path.insert(0, glslang_validator);
    return shader_path;
  }
}
