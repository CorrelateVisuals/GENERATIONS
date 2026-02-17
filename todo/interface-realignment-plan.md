# Interface Realignment Plan (main-compatible API + current config)

## Goal
Re-introduce the legacy scene interface surface (`Pipelines.h`, `Resources.h`, `World.h`) while keeping the current runtime scene configuration values and behavior.

## What changed vs `main` (analysis)
- Legacy top-level headers were moved from `src/` to modular paths:
  - `src/Pipelines.h` -> `src/render/Pipelines.h`
  - `src/Resources.h` -> `src/render/Resources.h`
  - `src/World.h` -> `src/world/World.h`
- Runtime scene setup moved from hardcoded constructors to a builder/spec path:
  - `src/implementation/ImplementationSpec.*`
  - `src/implementation/ScriptChainerApp.*`
  - runtime stores in `src/core/RuntimeConfig.*`
- Engine boot now installs runtime config first (`ScriptChainerApp::run()`), then constructs `Resources/Pipelines` from that config.

## Interpreting request
"Scrape all builder code with current config, and use the interfaces we used to have" means:
1. Remove dependency on `ImplementationSpec`/`ScriptChainerApp` as the source of scene wiring.
2. Keep today’s scene defaults and pipeline set (terrain/world/pipeline/render graph behavior).
3. Restore old include/API ergonomics around `Pipelines/Resources/World` for full scene config ownership.

## Recommended strategy (lowest risk)
Do this in two passes so functionality stays stable.

### Pass 1: Compatibility + ownership shift (no behavior changes)
- Add top-level compatibility headers in `src/`:
  - `src/Pipelines.h` forwarding to `render/Pipelines.h`
  - `src/Resources.h` forwarding to `render/Resources.h`
  - `src/World.h` forwarding to `world/World.h`
- Introduce one legacy-style scene config owner (single source of truth), e.g. `SceneConfig`/`LegacySceneConfig`.
  - Move the actual default values currently in `ImplementationSpec::default_spec()` into this owner.
  - Keep data model equivalent to `CE::Runtime::{TerrainSettings, WorldSettings, PipelineDefinition, RenderGraph, draw_ops}`.
- Update startup (`main`) to initialize runtime from this new owner directly, not from builder/spec files.

### Pass 2: Remove builder layer
- Remove or deprecate:
  - `ImplementationSpec.*`
  - `ScriptChainerApp.*`
  - `GraphExecutionPlan.*`
  - `ShaderGraph.*` (if no other use remains)
- Keep `RuntimeConfig` as a plain store/runtime registry, not as a place where scene defaults are authored.
- Ensure `Pipelines::Configuration` continues to consume runtime definitions exactly as now.

## If Plan C is executed (legacy-flat interface shell)

Plan C target:

```text
src/
├── World.h/.cpp
├── Resources.h/.cpp
├── Pipelines.h/.cpp
├── engine/
├── app/
└── io/
```

### What to watch out for

1. Header include cycles and compile-time explosions
- A flat shell tends to increase cross-includes between `World`, `Resources`, and `Pipelines`.
- Watch for cyclic dependencies that force broad recompiles and brittle build order.
- Mitigation: forward-declare aggressively and keep heavy Vulkan includes in `.cpp` files.

2. God-object drift
- In flat layouts, it is easy for `Resources` or `Pipelines` to absorb unrelated concerns.
- This makes behavior harder to reason about and test.
- Mitigation: define hard responsibility boundaries and reject cross-domain additions during review.

3. Hidden coupling to runtime defaults
- Removing builder/spec code can accidentally spread default values across constructors.
- That breaks parity and makes config drift likely.
- Mitigation: keep one canonical config source (single file/module) and wire all defaults from there.

4. Draw-op and render graph parity loss
- Current behavior relies on draw-op mappings + render graph stage logic.
- If this is reimplemented ad hoc, subtle ordering regressions appear.
- Mitigation: preserve current mapping tables and stage logic verbatim first, refactor only after parity checks.

5. Startup mode regressions
- `CE_SCRIPT_ONLY` currently exits after configuration install.
- Rewire can unintentionally skip initialization needed for script-only verification.
- Mitigation: keep startup contract explicit and test both normal and script-only startup paths.

6. Windows project drift
- CMake picks up files with glob; Visual Studio project lists files explicitly.
- Folder moves/removals can build on Linux while breaking VS solution.
- Mitigation: update `src/CAPITAL-engine.vcxproj` in the same PR as file moves.

7. Asset/shader naming assumptions
- Pipeline names are used across runtime definitions, shader loading, GUI stage strip, and command recording.
- Renaming or flattening can break name-based lookups silently.
- Mitigation: freeze pipeline identifiers during migration.

### Decision record (Plan C)

This section turns pre-work decisions into explicit choices so implementation can proceed without churn.

#### Technical decisions (set now)

1. Canonical config authority — **DECIDED**
- Create `src/scene/SceneConfig.h/.cpp` as the single owner of default scene config.
- `SceneConfig::apply_to_runtime()` installs defaults into `CE::Runtime`.
- Env interpretation remains centralized in `CE::Runtime::env_truthy/env_flag_enabled`.

2. RuntimeConfig role — **DECIDED**
- `src/core/RuntimeConfig.*` remains registry/store + parsing utilities.
- It does not own default scene authoring.

3. Deletion strategy — **DECIDED**
- Two-step migration:
  - Step 1: startup rewire + compatibility headers + parity verification.
  - Step 2: delete `src/implementation/*` and stale project references.

4. Naming contract — **DECIDED**
- Freeze class and pipeline identifiers during migration (`World`, `Resources`, `Pipelines`, and runtime pipeline keys).
- No casing/name changes until post-parity cleanup.

5. Validation gate — **DECIDED**
- Merge only if all pass:
  - pipelines present and ordered correctly
  - draw-op mappings identical
  - `CE_RENDER_STAGE` behavior unchanged
  - `CE_WORKLOAD_PRESET` and `CE_COMPUTE_CHAIN` behavior unchanged
  - Linux CMake build succeeds
  - Visual Studio project references updated and compile-ready

6. Boundary rules — **DECIDED**
- `World` owns simulation state, camera, geometry, terrain.
- `Resources` owns descriptors, buffers/images, and `World` instance lifecycle wiring.
- `Pipelines` owns pipeline layout/renderpass/config compilation only.
- `World` must not include `Pipelines`.
- `Pipelines` must not include `World.cpp`-only implementation details.
- `Resources` may depend on `World` and `Pipelines` public APIs, not internal engine helpers.
- Keep Vulkan-heavy includes in `.cpp` files where possible.

#### Executive decisions (locked)

1. API compatibility window — **DECIDED (E2)**
- Immediate hard-cut to legacy-flat API exposure.
- No long transition window for dual include paths.

2. Scope of physical folder moves — **DECIDED (E4)**
- Functional rewire and folder flattening happen in the same phase.

3. Change-management style — **DECIDED (E5)**
- Keep staged rollout with 2 PRs:
  - PR1: rewire + flatten + parity gate pass
  - PR2: cleanup, deletion completion, and follow-up fixes

4. Final API-layer target — **DECIDED (custom)**
- Final public scene API should stay as close to `main` as possible.
- `World`, `Resources`, and `Pipelines` are restored as the primary top-level interface shape and naming.
- During migration, avoid introducing new public-facing API concepts not present in `main` unless strictly required.

## Detailed work plan

### 1) Establish API compatibility layer
- Create forwarding headers to restore legacy include paths.
- Verify both old and new include paths compile.

### 2) Extract current config into legacy-owned module
- Add `src/scene/SceneConfig.h/.cpp` (or similar).
- Implement functions:
  - `SceneConfig::defaults()`
  - `SceneConfig::apply_to_runtime()`
- Copy current effective defaults from `ImplementationSpec::default_spec()` (terrain/world/pipelines/draw_ops/render-graph stage logic).

### 3) Rewire startup path
- Replace `ScriptChainerApp::run()` usage in `src/app/main.cpp` with `SceneConfig::apply_to_runtime()`.
- Keep `CE_SCRIPT_ONLY` behavior by allowing config init without launching render loop.

### 4) Deprecate/remove builder artifacts
- Remove references from CMake and project files.
- Delete `implementation/*` builder files once no references remain.
- Keep any truly reusable utilities only if still consumed elsewhere.

### 5) Validate scene parity
- Validate expected pipelines in runtime (`Engine`, `PostFX`, `Sky`, `Landscape`, `TerrainBox`, `Cells*`, compute chain variants).
- Validate draw-op mappings (`instanced:cells`, `indexed:grid`, `indexed:grid_box`, `sky_dome`).
- Validate render stage env behavior (`CE_RENDER_STAGE`, `CE_WORKLOAD_PRESET`, `CE_COMPUTE_CHAIN`).

## Estimated effort
- Pass 1 (compat + config owner + startup rewire): ~0.5-1.0 day
- Pass 2 (builder removal + cleanup + validation): ~0.5 day
- Total: ~1-1.5 days

## Risks and mitigation
- Risk: subtle render graph behavior drift after moving defaults.
  - Mitigation: snapshot and compare runtime graph/pipeline maps before and after.
- Risk: include-path breakage in external tooling.
  - Mitigation: keep forwarding headers and avoid immediate hard delete of modular headers.
- Risk: env-variable semantics change.
  - Mitigation: preserve current parsing and stage-selection logic exactly during extraction.

## Acceptance criteria
- Project builds with both legacy include paths and current modular paths.
- Scene output and stage behavior match current branch defaults.
- No startup dependency on `ImplementationSpec` / `ScriptChainerApp` remains.
- `Pipelines`, `Resources`, and `World` are again the obvious primary interfaces for scene configuration flow.

## Suggested execution order (PR-sized)
1. PR1: Add forwarding headers + introduce `SceneConfig` + wire startup.
2. PR2: Remove builder files and project references.
3. PR3: Optional cleanup/refactor of runtime config internals after parity confirmation.
