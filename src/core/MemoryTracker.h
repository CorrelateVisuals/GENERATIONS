#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace CE {

struct AllocationInfo {
  size_t size;
  std::string type;  // "Buffer" or "Image"
  std::chrono::steady_clock::time_point timestamp;
  uint64_t id;
};

class MemoryTracker {
public:
  static void record_allocation(size_t size);
  static void record_deallocation(size_t size);
  static void record_vulkan_allocation(size_t size, const std::string& type, void* address);
  static void record_vulkan_deallocation(void* address);
  
  static size_t get_total_allocated();
  static size_t get_total_deallocated();
  static size_t get_current_usage();
  static size_t get_peak_usage();
  
  static size_t get_vulkan_allocated();
  static size_t get_vulkan_deallocated();
  static size_t get_vulkan_current_usage();
  static size_t get_vulkan_peak_usage();
  
  static void log_memory_stats();
  static void log_detailed_leaks();
  static void reset_stats();
  
  static bool is_enabled();
  static std::chrono::steady_clock::time_point get_start_time();

private:
  static std::atomic<size_t> total_allocated;
  static std::atomic<size_t> total_deallocated;
  static std::atomic<size_t> peak_usage;
  
  static std::atomic<size_t> vulkan_allocated;
  static std::atomic<size_t> vulkan_deallocated;
  static std::atomic<size_t> vulkan_peak_usage;
  
  static std::atomic<uint64_t> allocation_counter;
  static std::mutex allocation_map_mutex;
  static std::map<void*, AllocationInfo> active_allocations;
};

} // namespace CE
