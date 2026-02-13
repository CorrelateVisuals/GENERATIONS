# GENERATIONS Architecture & Safety Profile

## Scope

This document describes the current phased hardening plan for the Vulkan base layer while keeping `World` as the composition root and preserving public engine interfaces.

## Design Principles

- Keep `World` as high-level composition (scene assembly, defaults, object wiring).
- Keep existing public API and include behavior centered on `BaseClasses.h`.
- Improve internal structure and runtime determinism under the hood.
- Prioritize real-time safety: bounded latency, fail-fast validation, deterministic lifecycle.

## Phase Status

### Phase 1: Internal base-class split + umbrella compatibility

Implemented:
- Split internal base-layer concerns into headers under `src/base/`:
  - `VulkanCore.h`
  - `VulkanPipelinePresets.h`
  - `VulkanUtils.h`
- `BaseClasses.h` now remains the compatibility umbrella while consuming split internal headers.

Expected impact:
- Better locality for constants/presets/utilities.
- Lower maintenance cost for future deeper file-level splits.

### Phase 2: Hot-path allocation removal + sync cleanup

Implemented:
- Replaced per-frame vector allocations in `CapitalEngine::drawFrame` with fixed-size arrays.
- Replaced several single-item literal counts with named constants.
- Narrowed screenshot sync scope from device-wide idle to graphics-queue idle.

Expected impact:
- Less allocator pressure and lower frame-time variance.
- Reduced global synchronization stall risk during screenshot path.

### Phase 3: Validation/invariants + timing instrumentation

Implemented:
- Added fail-fast checks in command helper lifecycle:
  - Null device/pool/queue detection for single-time command helpers.
  - Invalid command-buffer state detection before submit/free.
- Added pipeline creation invariants:
  - Empty pipeline map check.
  - Empty shader list check.
  - Empty vertex binding/attribute checks for graphics pipelines.
- Added timing instrumentation around per-pipeline and full pipeline creation.

Expected impact:
- Faster diagnosis of invalid runtime states.
- Measurable startup pipeline-cost visibility.

### Phase 4: Documentation update

Implemented:
- This document records architecture/safety intent and current implementation status.

## Safety Profile (Current)

- **Initialization safety:** validates key Vulkan setup contracts before pipeline creation.
- **Command safety:** checks command helper preconditions before command submission path.
- **Runtime determinism improvements:** removes known hot-loop dynamic allocations in draw submission setup.
- **Observability:** startup pipeline timing metrics emitted via logging.

## Next Suggested Steps

1. Move additional large declarations from `BaseClasses.h` into dedicated headers one subsystem at a time (sync, device, descriptors), while preserving umbrella include compatibility.
2. Replace remaining one-off blocking waits in interactive paths with fence-scoped synchronization where feasible.
3. Add build profile toggles (`DEV`, `SAFE_RT`) to control instrumentation verbosity and strictness.
4. Add startup validation summary log block with explicit PASS/FAIL for core invariants.
