#include "MemoryTracker.h"
#include "Log.h"

#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace CE {

std::atomic<size_t> MemoryTracker::total_allocated{0};
std::atomic<size_t> MemoryTracker::total_deallocated{0};
std::atomic<size_t> MemoryTracker::peak_usage{0};

std::atomic<size_t> MemoryTracker::vulkan_allocated{0};
std::atomic<size_t> MemoryTracker::vulkan_deallocated{0};
std::atomic<size_t> MemoryTracker::vulkan_peak_usage{0};

bool MemoryTracker::is_enabled() {
  static const bool enabled = [] {
    const char *value = std::getenv("CE_DEEPTEST_DURATION");
    return value != nullptr;
  }();
  return enabled;
}

std::chrono::steady_clock::time_point MemoryTracker::get_start_time() {
  static const std::chrono::steady_clock::time_point start_time = 
      std::chrono::steady_clock::now();
  return start_time;
}

void MemoryTracker::record_allocation(size_t size) {
  if (!is_enabled()) return;
  
  total_allocated.fetch_add(size, std::memory_order_relaxed);
  
  // Note: There's a minor race between these two loads - current usage calculation
  // may be slightly inaccurate. This is acceptable for peak tracking as:
  // 1. The error is temporary and small
  // 2. We prioritize low overhead over perfect accuracy
  // 3. A mutex would add unnecessary performance cost
  size_t allocated = total_allocated.load(std::memory_order_acquire);
  size_t deallocated = total_deallocated.load(std::memory_order_acquire);
  size_t current = allocated - deallocated;
  
  size_t expected = peak_usage.load(std::memory_order_relaxed);
  while (current > expected && 
         !peak_usage.compare_exchange_weak(expected, current, 
                                          std::memory_order_relaxed)) {
  }
}

void MemoryTracker::record_deallocation(size_t size) {
  if (!is_enabled()) return;
  total_deallocated.fetch_add(size, std::memory_order_relaxed);
}

void MemoryTracker::record_vulkan_allocation(size_t size) {
  if (!is_enabled()) return;
  
  vulkan_allocated.fetch_add(size, std::memory_order_relaxed);
  
  // Note: Same minor race as in record_allocation - acceptable for peak tracking
  size_t allocated = vulkan_allocated.load(std::memory_order_acquire);
  size_t deallocated = vulkan_deallocated.load(std::memory_order_acquire);
  size_t current = allocated - deallocated;
  
  size_t expected = vulkan_peak_usage.load(std::memory_order_relaxed);
  while (current > expected && 
         !vulkan_peak_usage.compare_exchange_weak(expected, current,
                                                  std::memory_order_relaxed)) {
  }
}

void MemoryTracker::record_vulkan_deallocation(size_t size) {
  if (!is_enabled()) return;
  vulkan_deallocated.fetch_add(size, std::memory_order_relaxed);
}

size_t MemoryTracker::get_total_allocated() {
  return total_allocated.load(std::memory_order_relaxed);
}

size_t MemoryTracker::get_total_deallocated() {
  return total_deallocated.load(std::memory_order_relaxed);
}

size_t MemoryTracker::get_current_usage() {
  size_t allocated = total_allocated.load(std::memory_order_acquire);
  size_t deallocated = total_deallocated.load(std::memory_order_acquire);
  return allocated - deallocated;
}

size_t MemoryTracker::get_peak_usage() {
  return peak_usage.load(std::memory_order_relaxed);
}

size_t MemoryTracker::get_vulkan_allocated() {
  return vulkan_allocated.load(std::memory_order_relaxed);
}

size_t MemoryTracker::get_vulkan_deallocated() {
  return vulkan_deallocated.load(std::memory_order_relaxed);
}

size_t MemoryTracker::get_vulkan_current_usage() {
  size_t allocated = vulkan_allocated.load(std::memory_order_acquire);
  size_t deallocated = vulkan_deallocated.load(std::memory_order_acquire);
  return allocated - deallocated;
}

size_t MemoryTracker::get_vulkan_peak_usage() {
  return vulkan_peak_usage.load(std::memory_order_relaxed);
}

void MemoryTracker::log_memory_stats() {
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      now - get_start_time()).count();
  
  auto format_bytes = [](size_t bytes) -> std::string {
    std::ostringstream oss;
    if (bytes >= 1024 * 1024 * 1024) {
      oss << std::fixed << std::setprecision(2) 
          << (static_cast<double>(bytes) / (1024 * 1024 * 1024)) << " GiB";
    } else if (bytes >= 1024 * 1024) {
      oss << std::fixed << std::setprecision(2) 
          << (static_cast<double>(bytes) / (1024 * 1024)) << " MiB";
    } else if (bytes >= 1024) {
      oss << std::fixed << std::setprecision(2) 
          << (static_cast<double>(bytes) / 1024) << " KiB";
    } else {
      oss << bytes << " B";
    }
    return oss.str();
  };
  
  Log::text("{ PERF }", "=== DEEP TEST MEMORY REPORT ===");
  Log::text(Log::Style::char_leader, "Runtime:", elapsed, "seconds");
  Log::text(Log::Style::char_leader, "Vulkan Current:", 
            format_bytes(get_vulkan_current_usage()));
  Log::text(Log::Style::char_leader, "Vulkan Peak:", 
            format_bytes(get_vulkan_peak_usage()));
  Log::text(Log::Style::char_leader, "Vulkan Allocated:", 
            format_bytes(get_vulkan_allocated()));
  Log::text(Log::Style::char_leader, "Vulkan Deallocated:", 
            format_bytes(get_vulkan_deallocated()));
}

void MemoryTracker::reset_stats() {
  total_allocated.store(0, std::memory_order_relaxed);
  total_deallocated.store(0, std::memory_order_relaxed);
  peak_usage.store(0, std::memory_order_relaxed);
  
  vulkan_allocated.store(0, std::memory_order_relaxed);
  vulkan_deallocated.store(0, std::memory_order_relaxed);
  vulkan_peak_usage.store(0, std::memory_order_relaxed);
  
  // Note: start_time cannot be reset as it's a const static local in get_start_time()
}

} // namespace CE
