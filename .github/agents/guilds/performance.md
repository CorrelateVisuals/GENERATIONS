# Performance Guild

## Members
- **C++ Lead** — CPU-side bottlenecks, allocation patterns, cache-friendly layout
- **Vulkan Guru** — GPU resource reuse, synchronization overhead, pipeline caching
- **Kernel Expert** — shader occupancy, workgroup sizing, memory access patterns

## Shared Vocabulary
| Term | Meaning |
|---|---|
| **hot path** | Code executed every frame (render loop, compute dispatch) |
| **cold path** | Code executed once or rarely (init, resize, shutdown) |
| **budget** | Frame-time allocation: 16.6ms target at 60fps |
| **stall** | CPU waiting on GPU or vice versa |
| **occupancy** | GPU ALU utilization vs theoretical max |
| **reuse** | Using an existing resource/pipeline/descriptor instead of creating new |

## Decision Protocol
When members disagree on a performance approach:
1. Measure first — no optimization without a profiling baseline
2. GPU-bound → Kernel Expert leads, Vulkan Guru advises
3. CPU-bound → C++ Lead leads, Vulkan Guru advises on API overhead
4. Mixed → C++ Lead sequences the work, each member owns their layer

## Common Procedures
<!-- Agents append discovered patterns here using ## PROC-PERF-NNN format -->

### PROC-PERF-001 — Frame-Time Regression Check
1. Identify the hot path touched by the change
2. Estimate frame-time impact (add/remove/neutral)
3. Flag if change moves work from cold path to hot path
4. If uncertain, mark as "needs profiling" in TODO

### PROC-PERF-002 — Resource Reuse Audit
1. Before creating a new buffer/image/pipeline, check if an existing one serves
2. If reuse is possible, Vulkan Guru documents the sharing contract
3. Kernel Expert validates the layout/format is compatible with shader expectations
4. C++ Lead verifies lifetime ownership is unambiguous
