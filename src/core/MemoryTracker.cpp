#include "MemoryTracker.h"
#include "Log.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <vector>

namespace CE {

std::atomic<size_t> MemoryTracker::total_allocated{0};
std::atomic<size_t> MemoryTracker::total_deallocated{0};
std::atomic<size_t> MemoryTracker::peak_usage{0};

std::atomic<size_t> MemoryTracker::vulkan_allocated{0};
std::atomic<size_t> MemoryTracker::vulkan_deallocated{0};
std::atomic<size_t> MemoryTracker::vulkan_peak_usage{0};

std::atomic<uint64_t> MemoryTracker::allocation_counter{0};
std::mutex MemoryTracker::allocation_map_mutex;
std::map<void*, AllocationInfo> MemoryTracker::active_allocations;

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

void MemoryTracker::record_vulkan_allocation(size_t size, const std::string& type, void* address) {
  if (!is_enabled()) return;
  
  vulkan_allocated.fetch_add(size, std::memory_order_relaxed);
  
  // Track this specific allocation
  {
    std::lock_guard<std::mutex> lock(allocation_map_mutex);
    uint64_t id = allocation_counter.fetch_add(1, std::memory_order_relaxed);
    active_allocations[address] = AllocationInfo{
      size,
      type,
      std::chrono::steady_clock::now(),
      id
    };
  }
  
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

void MemoryTracker::record_vulkan_deallocation(void* address) {
  if (!is_enabled()) return;
  
  // Remove from active allocations and get size
  size_t size = 0;
  {
    std::lock_guard<std::mutex> lock(allocation_map_mutex);
    auto it = active_allocations.find(address);
    if (it != active_allocations.end()) {
      size = it->second.size;
      active_allocations.erase(it);
    }
  }
  
  if (size > 0) {
    vulkan_deallocated.fetch_add(size, std::memory_order_relaxed);
  }
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
  
  // Report active allocation count
  size_t active_count = 0;
  {
    std::lock_guard<std::mutex> lock(allocation_map_mutex);
    active_count = active_allocations.size();
  }
  Log::text(Log::Style::char_leader, "Active Allocations:", active_count);
}

void MemoryTracker::log_detailed_leaks() {
  if (!is_enabled()) return;
  
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
  
  std::vector<AllocationInfo> leaks;
  size_t total_leaked = 0;
  
  {
    std::lock_guard<std::mutex> lock(allocation_map_mutex);
    for (const auto& pair : active_allocations) {
      leaks.push_back(pair.second);
      total_leaked += pair.second.size;
    }
  }
  
  if (leaks.empty()) {
    Log::text("{ PERF }", "=== NO MEMORY LEAKS DETECTED ===");
    Log::text(Log::Style::char_leader, "All allocations have been freed");
    return;
  }
  
  // Sort by size (largest first)
  std::sort(leaks.begin(), leaks.end(), 
            [](const AllocationInfo& a, const AllocationInfo& b) {
              return a.size > b.size;
            });
  
  Log::text("{ !!! }", "=== MEMORY LEAK ANALYSIS ===");
  Log::text(Log::Style::char_leader, "Total Leaked:", format_bytes(total_leaked));
  Log::text(Log::Style::char_leader, "Leak Count:", leaks.size());
  
  // Group by type
  std::map<std::string, size_t> type_totals;
  std::map<std::string, size_t> type_counts;
  for (const auto& leak : leaks) {
    type_totals[leak.type] += leak.size;
    type_counts[leak.type]++;
  }
  
  Log::text("{ !!! }", "Leaks by Type:");
  for (const auto& pair : type_totals) {
    Log::text(Log::Style::char_leader, pair.first + ":", 
              format_bytes(pair.second), 
              "(" + std::to_string(type_counts[pair.first]) + " allocations)");
  }
  
  // Show top 10 largest leaks
  Log::text("{ !!! }", "Top 10 Largest Leaks:");
  size_t count = 0;
  for (const auto& leak : leaks) {
    if (count >= 10) break;
    
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - leak.timestamp).count();
    
    std::ostringstream info;
    info << "ID #" << leak.id << " " << leak.type 
         << " " << format_bytes(leak.size)
         << " (age: " << elapsed << "s)";
    
    Log::text(Log::Style::char_leader, info.str());
    count++;
  }
  
  if (leaks.size() > 10) {
    Log::text(Log::Style::char_leader, "... and", (leaks.size() - 10), "more");
  }
}

void MemoryTracker::reset_stats() {
  total_allocated.store(0, std::memory_order_relaxed);
  total_deallocated.store(0, std::memory_order_relaxed);
  peak_usage.store(0, std::memory_order_relaxed);
  
  vulkan_allocated.store(0, std::memory_order_relaxed);
  vulkan_deallocated.store(0, std::memory_order_relaxed);
  vulkan_peak_usage.store(0, std::memory_order_relaxed);
  
  allocation_counter.store(0, std::memory_order_relaxed);
  
  {
    std::lock_guard<std::mutex> lock(allocation_map_mutex);
    active_allocations.clear();
  }
  
  // Note: start_time cannot be reset as it's a const static local in get_start_time()
}

} // namespace CE
