# Deep Test - Memory Leak Detection

## Overview

The GENERATIONS engine includes a runtime deep test mode designed to detect memory leaks during extended operation (5+ hours). This feature tracks Vulkan memory allocations and deallocations, providing periodic reports and final statistics.

## How It Works

When `CE_DEEPTEST_DURATION` environment variable is set, the engine:

1. **Tracks all Vulkan memory allocations/deallocations** - Monitors buffer and image memory
2. **Runs for a specified duration** - Automatically terminates after the configured time
3. **Reports periodically** - Logs memory statistics every hour
4. **Provides final statistics** - Shows total allocations, deallocations, current usage, and peak usage

## Running the Deep Test

### Using the Script (Recommended)

```bash
# Run for 5 hours (default)
./tools/deeptest_memory.sh

# Run for custom duration (in seconds)
./tools/deeptest_memory.sh 10800  # 3 hours
./tools/deeptest_memory.sh 18000  # 5 hours
./tools/deeptest_memory.sh 36000  # 10 hours
```

### Manual Execution

```bash
# Set duration in seconds (e.g., 18000 = 5 hours)
CE_DEEPTEST_DURATION=18000 ./bin/CapitalEngine
```

## Reading the Results

The deep test logs memory statistics in the following format:

```
{ PERF } === DEEP TEST MEMORY REPORT ===
         : Runtime: 3600 seconds
         : Vulkan Current: 512.50 MiB
         : Vulkan Peak: 515.75 MiB
         : Vulkan Allocated: 2.25 GiB
         : Vulkan Deallocated: 1.75 GiB
         : Active Allocations: 42
```

At the end of the test, if any memory leaks are detected, you'll see a detailed analysis:

```
{ !!! } === MEMORY LEAK ANALYSIS ===
         : Total Leaked: 128.00 MiB
         : Leak Count: 15
{ !!! } Leaks by Type:
         : Buffer: 64.00 MiB (8 allocations)
         : Image: 64.00 MiB (7 allocations)
{ !!! } Top 10 Largest Leaks:
         : ID #1234 Buffer 16.00 MiB (age: 18000s)
         : ID #1235 Image 12.50 MiB (age: 17950s)
         : ...
```

### Key Metrics

- **Runtime** - How long the test has been running
- **Vulkan Current** - Current memory usage (should be stable, not growing)
- **Vulkan Peak** - Maximum memory usage observed
- **Vulkan Allocated** - Total memory allocated (will increase)
- **Vulkan Deallocated** - Total memory deallocated (should track allocations)
- **Active Allocations** - Number of allocations that haven't been freed yet

### Leak Analysis Details

When leaks are detected, the detailed analysis provides:

- **Total Leaked** - Sum of all unfreed memory
- **Leak Count** - Number of individual allocations still active
- **Leaks by Type** - Breakdown showing whether Buffers or Images are leaking
- **Top Largest Leaks** - Individual leak entries with:
  - Unique ID number for each allocation
  - Type (Buffer or Image)
  - Size of the leaked allocation
  - Age (how long ago it was allocated)

This information helps pinpoint exactly which allocations are not being freed.

### Detecting Memory Leaks

A memory leak is indicated by:

1. **Growing Current Usage** - If "Vulkan Current" continuously increases over time
2. **Allocation/Deallocation Mismatch** - If allocated memory significantly exceeds deallocated memory and the gap keeps growing
3. **Active Allocations at Shutdown** - Non-zero active allocations in the final report

Expected behavior:
- Initial allocations during startup
- Stable current usage during runtime
- Minor fluctuations are normal due to swapchain recreation and resource updates
- Final report should show current usage similar to peak usage (indicating stable state)
- **Zero or near-zero active allocations at shutdown** (some may remain due to static resources)

## Integration with CI/CD

For automated testing, the script returns the application's exit code and can be integrated into test pipelines:

```bash
# Run short test for CI (30 seconds)
./tools/deeptest_memory.sh 30

# Check exit code
if [ $? -eq 0 ]; then
    echo "Deep test passed"
else
    echo "Deep test failed"
    exit 1
fi
```

## Technical Details

### Implementation

- **Memory Tracking** - Implemented in `src/core/MemoryTracker.h` and `MemoryTracker.cpp`
- **Instrumentation** - Vulkan allocations tracked in `src/base/VulkanResources.cpp`
- **Main Loop Integration** - Time-limited execution in `src/app/CapitalEngine.cpp`

### Thread Safety

The memory tracker uses atomic operations for thread-safe counter updates, making it safe to use with Vulkan's multi-queue submissions.

### Performance Impact

When `CE_DEEPTEST_DURATION` is not set, the memory tracker has zero overhead (all tracking is disabled via inline checks).

## Troubleshooting

### Test Terminates Early

- Check for Vulkan errors in the log
- Ensure the display server is available (or use headless mode if configured)
- Verify sufficient system memory is available

### No Memory Reports in Log

- Ensure `CE_DEEPTEST_DURATION` is set
- Check that the application ran for at least the report interval (1 hour)
- Verify log.txt file is writable

### Inconsistent Memory Values

- This is normal during swapchain recreation (window resize)
- Allow the application to stabilize for a few minutes before analyzing trends
- Focus on long-term trends rather than momentary spikes

## Future Enhancements

Potential improvements to the deep test infrastructure:

- GPU memory heap statistics via `vkGetPhysicalDeviceMemoryProperties`
- CPU memory tracking with system allocator hooks
- Automated leak detection with configurable thresholds
- Export to structured formats (JSON, CSV) for analysis
- Integration with Vulkan Memory Allocator (VMA) statistics
