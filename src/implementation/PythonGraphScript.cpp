#include "PythonGraphScript.h"

#include <array>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string trim_copy(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::vector<std::string> split_tokens(const std::string &line) {
  std::istringstream stream(line);
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string read_command_stdout(const std::string &command, int &exit_code) {
  std::array<char, 256> buffer{};
  std::string output;

  FILE *pipe = popen(command.c_str(), "r");
  if (!pipe) {
    throw std::runtime_error("failed to start command: " + command);
  }

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    output += buffer.data();
  }

  exit_code = pclose(pipe);
  return output;
}

} // namespace

namespace CE::Implementation {

ShaderGraph PythonGraphScript::load(const std::filesystem::path &script_path) {
  const std::string command = "python3 \"" + script_path.string() + "\" --emit-graph";

  int exit_code = 0;
  const std::string output = read_command_stdout(command, exit_code);
  if (exit_code != 0) {
    throw std::runtime_error("graph script failed with exit code " + std::to_string(exit_code) +
                             ": " + script_path.string());
  }

  ShaderGraph graph;
  std::istringstream lines(output);
  std::string line;
  std::string error;

  std::size_t line_number = 0;
  while (std::getline(lines, line)) {
    line_number++;
    const std::string trimmed = trim_copy(line);
    if (trimmed.empty() || trimmed.starts_with('#')) {
      continue;
    }

    const std::vector<std::string> tokens = split_tokens(trimmed);
    if (tokens.empty()) {
      continue;
    }

    const std::string &record = tokens[0];
    if (record == "NODE") {
      if (tokens.size() != 4) {
        throw std::runtime_error("invalid NODE record at line " + std::to_string(line_number));
      }
      const ShaderNode node{.id = tokens[1],
                            .shader_name = tokens[2],
                            .stage = shader_stage_from_extension(tokens[3])};
      if (!graph.add_node(node, error)) {
        throw std::runtime_error("invalid node at line " + std::to_string(line_number) +
                                 ": " + error);
      }
      continue;
    }

    if (record == "EDGE") {
      if (tokens.size() != 3) {
        throw std::runtime_error("invalid EDGE record at line " + std::to_string(line_number));
      }
      if (!graph.add_edge(GraphEdge{.from = tokens[1], .to = tokens[2]}, error)) {
        throw std::runtime_error("invalid edge at line " + std::to_string(line_number) +
                                 ": " + error);
      }
      continue;
    }

    if (record == "INPUT") {
      if (tokens.size() != 3) {
        throw std::runtime_error("invalid INPUT record at line " + std::to_string(line_number));
      }
      graph.set_input(GraphEndpoint{.node_id = tokens[1], .resource = tokens[2]});
      continue;
    }

    if (record == "OUTPUT") {
      if (tokens.size() != 3) {
        throw std::runtime_error("invalid OUTPUT record at line " + std::to_string(line_number));
      }
      graph.set_output(GraphEndpoint{.node_id = tokens[1], .resource = tokens[2]});
      continue;
    }

    if (record == "DRAW") {
      if (tokens.size() != 3) {
        throw std::runtime_error("invalid DRAW record at line " + std::to_string(line_number));
      }
      if (!graph.add_graphics_draw_binding(
              GraphicsDrawBinding{.pipeline_name = tokens[1], .draw_op = tokens[2]}, error)) {
        throw std::runtime_error("invalid DRAW record at line " +
                                 std::to_string(line_number) + ": " + error);
      }
      continue;
    }

    if (record == "SETTING") {
      if (tokens.size() != 3) {
        throw std::runtime_error("invalid SETTING record at line " +
                                 std::to_string(line_number));
      }
      if (!graph.add_setting(tokens[1], tokens[2], error)) {
        throw std::runtime_error("invalid SETTING record at line " +
                                 std::to_string(line_number) + ": " + error);
      }
      continue;
    }

    throw std::runtime_error("unknown record type at line " + std::to_string(line_number) +
                             ": " + record);
  }

  if (!graph.validate(error)) {
    throw std::runtime_error("graph validation failed: " + error);
  }

  return graph;
}

} // namespace CE::Implementation
