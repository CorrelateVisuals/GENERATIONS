# Agent Procedures

Persistent record of reusable patterns discovered during agent runs.
Agents append new procedures here. Humans curate and prune.

Format:
```
### PROC-<GUILD>-NNN — Title
- **Origin**: agent name, date, task ID
- **Steps**: numbered action list
- **Applies when**: trigger condition
```

---

<!-- Procedures are appended below by agent runs and manual curation -->
<!-- See guilds/*.md for guild-specific seed procedures -->

### From C++ Lead (2026-02-17, FOLLOW-AUDIT-SYNCHRONIZATION-AN)
### PROC-PERF-003 — Synchronization Profiling Hook
1. Add logging hooks to measure the time spent on synchronization primitives.
2. Log barrier types, affected resources, and pipeline stages.
3. Use `Log::measure_elapsed_time()` to track the duration of synchronization operations.
4. Ensure profiling hooks are lightweight and do not introduce significant overhead.
```

### From Vulkan Guru (2026-02-17, FOLLOW-AUDIT-SYNCHRONIZATION-AN)
### PROC-GPU-002 — Barrier Optimization Workflow
1. Identify all pipeline barriers in the render loop.
2. Analyze the resource usage (e.g., image layouts, buffer access patterns).
3. Match the `srcStageMask` and `dstStageMask` to the actual pipeline stages involved.
4. Minimize the `srcAccessMask` and `dstAccessMask` to the specific operations required.
5. Test changes with Vulkan validation layers enabled to ensure correctness.
6. Profile frame time before and after changes to measure performance impact.
```

### From Kernel Expert (2026-02-17, FOLLOW-AUDIT-SYNCHRONIZATION-AN)
### PROC-PERF-003 — Workgroup Size Optimization
1. Profile the current workgroup sizes using GPU profiling tools (e.g., Nsight, RenderDoc).
2. Identify the optimal workgroup size for the target GPU architecture, balancing occupancy and resource usage.
3. Update the `local_size_x`, `local_size_y`, and `local_size_z` values in the compute shader.
4. Validate the changes on both Linux and Windows to ensure cross-platform compatibility.
5. Document the profiling results and the rationale for the chosen workgroup size.
```

### From Refactorer (2026-02-17, FOLLOW-AUDIT-SYNCHRONIZATION-AN)
### PROC-ARCH-003 — Barrier Simplification
- **Origin**: Refactorer Agent, 2023-10-25, Task ID: FOLLOW-AUDIT-SYNCHRONIZATION-AN
- **Steps**:
  1. Identify barriers with overly restrictive pipeline stages or access masks.
  2. Analyze the actual resource usage in shaders and pipeline stages.
  3. Update barriers to use the minimal necessary synchronization scope.
  4. Test changes to ensure no rendering artifacts or synchronization issues.
- **Applies when**: Barriers are causing unnecessary GPU stalls or reduced parallelism.
```

### From HPC Marketeer (2026-02-17, FOLLOW-AUDIT-SYNCHRONIZATION-AN)
### New Reusable Pattern: Documenting Synchronization Changes
When documenting changes to Vulkan synchronization:
1. **Explain Key Concepts**: Include a brief explanation of synchronization primitives, pipeline stages, and resource transitions.
2. **Highlight Changes**: Clearly state what was changed and why (e.g., simplified barriers, resolved mismatches).
3. **Provide Debugging Guidance**: Include steps for debugging synchronization issues using validation layers and logging.
4. **Update Onboarding Materials**: Ensure new contributors can understand and work with the updated synchronization logic.
5. **Align Terminology**: Ensure consistent use of terms across the codebase and documentation.

This pattern has been recorded for future use in the `GUILD:documentation` procedure log.
```
