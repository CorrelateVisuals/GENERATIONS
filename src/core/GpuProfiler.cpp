#include "GpuProfiler.h"
#include "Log.h"
#include "RuntimeConfig.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace CE {

GpuProfiler &GpuProfiler::instance() {
  static GpuProfiler profiler;
  return profiler;
}

void GpuProfiler::init(VkDevice device, VkPhysicalDevice physical_device) {
  device_ = device;
  physical_device_ = physical_device;
  enabled_ = Runtime::env_flag_enabled("CE_GPU_TRACE");

  if (!enabled_) {
    return;
  }

  // Get timestamp period for converting to milliseconds
  VkPhysicalDeviceProperties device_properties{};
  vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
  timestamp_period_ = device_properties.limits.timestampPeriod;

  // Create query pool for timestamp queries
  VkQueryPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
  pool_info.queryCount = MAX_QUERIES;

  if (vkCreateQueryPool(device_, &pool_info, nullptr, &query_pool_) != VK_SUCCESS) {
    Log::text("{ GPU }", "Failed to create GPU profiler query pool");
    enabled_ = false;
    return;
  }

  initialized_ = true;
  Log::text("{ GPU }",
            "GPU Profiler initialized (timestamp period:",
            timestamp_period_,
            "ns)");
}

void GpuProfiler::cleanup() {
  if (query_pool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
    vkDestroyQueryPool(device_, query_pool_, nullptr);
    query_pool_ = VK_NULL_HANDLE;
  }
  initialized_ = false;
}

bool GpuProfiler::is_enabled() const {
  return enabled_ && initialized_;
}

void GpuProfiler::begin_event(VkCommandBuffer cmd_buffer, const std::string &event_name) {
  if (!is_enabled() || query_count_ >= MAX_QUERIES - 1) {
    return;
  }

  const uint32_t query_index = query_count_;
  query_count_++;

  vkCmdWriteTimestamp(cmd_buffer,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      query_pool_,
                      query_index);

  begin_query_indices_[event_name] = query_index;
}

void GpuProfiler::end_event(VkCommandBuffer cmd_buffer, const std::string &event_name) {
  if (!is_enabled() || query_count_ >= MAX_QUERIES) {
    return;
  }

  const auto it = begin_query_indices_.find(event_name);
  if (it == begin_query_indices_.end()) {
    return;
  }

  const uint32_t query_index = query_count_;
  query_count_++;

  vkCmdWriteTimestamp(cmd_buffer,
                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                      query_pool_,
                      query_index);

  // Store event info for later resolution
  Event event;
  event.name = event_name;
  event.is_gpu_event = true;
  // Will be filled in resolve_timestamps()
  events_.push_back(event);
}

void GpuProfiler::begin_cpu_event(const std::string &event_name) {
  if (!is_enabled()) {
    return;
  }

  cpu_begin_times_[event_name] = std::chrono::steady_clock::now();
}

void GpuProfiler::end_cpu_event(const std::string &event_name) {
  if (!is_enabled()) {
    return;
  }

  const auto it = cpu_begin_times_.find(event_name);
  if (it == cpu_begin_times_.end()) {
    return;
  }

  const auto end_time = std::chrono::steady_clock::now();
  const auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - it->second);

  Event event;
  event.name = event_name;
  event.is_gpu_event = false;
  event.duration_ms = static_cast<double>(duration.count()) / 1e6;
  events_.push_back(event);

  cpu_begin_times_.erase(it);
}

void GpuProfiler::resolve_timestamps() {
  if (!is_enabled() || query_count_ == 0) {
    return;
  }

  // Retrieve all timestamp query results
  std::vector<uint64_t> timestamps(query_count_);
  const VkResult result = vkGetQueryPoolResults(device_,
                                                query_pool_,
                                                0,
                                                query_count_,
                                                timestamps.size() * sizeof(uint64_t),
                                                timestamps.data(),
                                                sizeof(uint64_t),
                                                VK_QUERY_RESULT_64_BIT |
                                                    VK_QUERY_RESULT_WAIT_BIT);

  if (result != VK_SUCCESS) {
    Log::text("{ GPU }", "Failed to retrieve GPU timestamps");
    return;
  }

  // Update GPU events with actual timestamps
  uint32_t event_index = 0;
  for (const auto &[name, begin_index] : begin_query_indices_) {
    if (event_index < events_.size() && events_[event_index].is_gpu_event) {
      const uint32_t end_index = begin_index + 1;
      if (end_index < timestamps.size()) {
        events_[event_index].begin_timestamp = timestamps[begin_index];
        events_[event_index].end_timestamp = timestamps[end_index];
        const uint64_t delta = timestamps[end_index] - timestamps[begin_index];
        events_[event_index].duration_ms =
            static_cast<double>(delta) * static_cast<double>(timestamp_period_) / 1e6;
      }
      event_index++;
    }
  }

  // Reset query pool for next frame
  vkResetQueryPool(device_, query_pool_, 0, query_count_);
  query_count_ = 0;
  begin_query_indices_.clear();
}

const std::vector<GpuProfiler::Event> &GpuProfiler::get_events() const {
  return events_;
}

void GpuProfiler::clear_events() {
  events_.clear();
}

void GpuProfiler::print_report() const {
  if (!is_enabled() || events_.empty()) {
    return;
  }

  Log::text("┌─────────────────────────────────────────────────────────┐");
  Log::text("│           GPU Profiler Report                           │");
  Log::text("├─────────────────────────────────────────────────────────┤");

  double total_gpu_time = 0.0;
  double total_cpu_time = 0.0;

  for (const auto &event : events_) {
    std::ostringstream line;
    line << "│ " << std::setw(40) << std::left << event.name;
    line << std::setw(10) << std::right << std::fixed << std::setprecision(3)
         << event.duration_ms << " ms";
    line << " [" << (event.is_gpu_event ? "GPU" : "CPU") << "]";

    // Pad to align with box
    const std::string line_str = line.str();
    Log::text(line_str);

    if (event.is_gpu_event) {
      total_gpu_time += event.duration_ms;
    } else {
      total_cpu_time += event.duration_ms;
    }
  }

  Log::text("├─────────────────────────────────────────────────────────┤");

  std::ostringstream summary;
  summary << "│ Total GPU time: " << std::setw(10) << std::fixed << std::setprecision(3)
          << total_gpu_time << " ms";
  Log::text(summary.str());

  summary.str("");
  summary << "│ Total CPU time: " << std::setw(10) << std::fixed << std::setprecision(3)
          << total_cpu_time << " ms";
  Log::text(summary.str());

  Log::text("└─────────────────────────────────────────────────────────┘");
}

} // namespace CE
