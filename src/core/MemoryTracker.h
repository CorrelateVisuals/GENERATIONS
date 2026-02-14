#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace CE {

class MemoryTracker {
public:
  static void record_allocation(size_t size);
  static void record_deallocation(size_t size);
  static void record_vulkan_allocation(size_t size);
  static void record_vulkan_deallocation(size_t size);
  
  static size_t get_total_allocated();
  static size_t get_total_deallocated();
  static size_t get_current_usage();
  static size_t get_peak_usage();
  
  static size_t get_vulkan_allocated();
  static size_t get_vulkan_deallocated();
  static size_t get_vulkan_current_usage();
  static size_t get_vulkan_peak_usage();
  
  static void log_memory_stats();
  static void reset_stats();
  
  static bool is_enabled();

private:
  static std::atomic<size_t> total_allocated;
  static std::atomic<size_t> total_deallocated;
  static std::atomic<size_t> peak_usage;
  
  static std::atomic<size_t> vulkan_allocated;
  static std::atomic<size_t> vulkan_deallocated;
  static std::atomic<size_t> vulkan_peak_usage;
  
  static std::chrono::steady_clock::time_point start_time;
};

} // namespace CE
