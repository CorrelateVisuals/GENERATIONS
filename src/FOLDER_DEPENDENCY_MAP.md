# Source Folder Dependency Map

## Current folders
- `app`: engine entry/orchestration
- `base`: Vulkan primitives/utilities
- `core`: logging/time
- `io`: filesystem + screenshot helpers
- `platform`: window + validation layer plumbing
- `render`: mechanics/pipelines/resources/shader command recording
- `world`: camera/terrain/geometry/world state

## Current dependency directions
- `app` -> `render`, `io`, `platform`, `core`, `base`
- `render` -> `world`, `io`, `core`, `base`
- `world` -> `io`, `core`, `platform`, `base`
- `io` -> `core`, `base`
- `platform` -> `core`
- `base` -> `core`, `io`, `platform`

## Practical rule set (recommended)
- `app` may depend on any runtime layer.
- `render` should not depend on `app`.
- `world` should not depend on `app` or `render`.
- `platform` should not depend on `app`, `world`, or `render`.
- `io` should stay utility-oriented (no `app`/`render`/`world`).

## Notes from cleanup
- Removed stale top-level `src/*.h` and `src/*.cpp` modules into domain folders.
- Removed unnecessary `app` includes from `platform` sources to keep layer direction clean.
- Build/runtime validated after refactor.
