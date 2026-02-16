#pragma once

#include <vulkan/vulkan.h>

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace CE {

// Lightweight GPU profiler for monitoring GPU-CPU communication, buffer lifetime,
// transfer times and synchronization waits.
//
// Usage:
//   1. Call init() after device creation with VkDevice and VkPhysicalDevice
//   2. Use begin_event()/end_event() pairs around GPU operations to track
//   3. Use begin_cpu_event()/end_cpu_event() for CPU-side timing
//   4. Call resolve_timestamps() to retrieve GPU timing results
//   5. Call print_report() to output profiling data
//
// Controlled by CE_GPU_TRACE environment flag (see RuntimeConfig.h)
class GpuProfiler {
public:
  struct Event {
    std::string name;
    uint64_t begin_timestamp{0};
    uint64_t end_timestamp{0};
    double duration_ms{0.0};
    bool is_gpu_event{true};
    uint32_t begin_query_index{0};
    uint32_t end_query_index{0};
  };

  static GpuProfiler &instance();

  // Initialize profiler with device handles
  void init(VkDevice device, VkPhysicalDevice physical_device);

  // Cleanup resources
  void cleanup();

  // Check if profiler is enabled (via CE_GPU_TRACE env flag)
  bool is_enabled() const;

  // GPU event tracking (uses timestamp queries)
  void begin_event(VkCommandBuffer cmd_buffer, const std::string &event_name);
  void end_event(VkCommandBuffer cmd_buffer, const std::string &event_name);

  // CPU event tracking (uses chrono)
  void begin_cpu_event(const std::string &event_name);
  void end_cpu_event(const std::string &event_name);

  // Resolve GPU timestamps after command buffer execution
  void resolve_timestamps();

  // Get recorded events
  const std::vector<Event> &get_events() const;

  // Clear all recorded events
  void clear_events();

  // Print profiling report to console
  void print_report() const;

private:
  GpuProfiler() = default;
  ~GpuProfiler() = default;
  GpuProfiler(const GpuProfiler &) = delete;
  GpuProfiler &operator=(const GpuProfiler &) = delete;

  VkDevice device_{VK_NULL_HANDLE};
  VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
  VkQueryPool query_pool_{VK_NULL_HANDLE};
  float timestamp_period_{1.0f};
  bool initialized_{false};
  bool enabled_{false};

  static constexpr uint32_t MAX_QUERIES = 256;
  uint32_t query_count_{0};

  std::unordered_map<std::string, uint32_t> begin_query_indices_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> cpu_begin_times_;
  std::vector<Event> events_;
};

} // namespace CE
