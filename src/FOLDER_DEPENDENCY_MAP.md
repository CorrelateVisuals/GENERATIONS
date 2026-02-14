# Source Folder Dependency Map

## Current folders
- `app`: engine entry/orchestration
- `base`: Vulkan primitives/utilities
- `core`: logging/time
- `io`: filesystem + screenshot helpers
- `implementation`: script/app-level graph declaration and loading
- `platform`: window + validation layer plumbing
- `render`: mechanics/pipelines/resources/shader command recording
- `world`: camera/terrain/geometry/world state

## Current dependency directions
- `app` -> `render`, `io`, `platform`, `core`, `base`
- `app` -> `implementation`
- `render` -> `world`, `io`, `core`, `base`
- `implementation` -> `core`
- `world` -> `io`, `core`, `platform`, `base`
- `io` -> `core`, `base`
- `platform` -> `core`
- `base` -> `core`, `io`, `platform`

## Practical rule set (recommended)
- `app` may depend on any runtime layer.
- `implementation` should stay orchestration-focused and avoid Vulkan ownership.
- `render` should not depend on `app`.
- `world` should not depend on `app` or `render`.
- `platform` should not depend on `app`, `world`, or `render`.
- `io` should stay utility-oriented (no `app`/`render`/`world`).

## Notes from cleanup
- Removed stale top-level `src/*.h` and `src/*.cpp` modules into domain folders.
- Removed unnecessary `app` includes from `platform` sources to keep layer direction clean.
- Build/runtime validated after refactor.

## Automated enforcement
- Build now enforces these boundaries through `tools/check_folder_dependencies.py`.
- Manual run: `python3 tools/check_folder_dependencies.py`
