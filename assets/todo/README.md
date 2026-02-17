# TODO

## Current open todo

### Add new geometry shape (minimal path)

Shared facts:

- `Shape(...)` only needs a `GEOMETRY_SHAPE` enum value; loading/fallback is centralized in `Geometry::Geometry(...)`.
- Runtime selection is resolved in `resolve_shape(...)` in `src/resources/World.cpp`.
- Asset lookup is name-based (`assets/3D/<Name>.obj`), and missing assets fall back safely.

Minimal flow:

1. Add enum + model mapping
   - `src/world/Geometry.h`: add enum in `GEOMETRY_SHAPE`
   - `src/world/Geometry.cpp`: map enum -> model name in `Geometry::Geometry(...)`
2. Add assets (optional)
   - place `assets/3D/<ModelName>.obj` (+ `.mtl` if needed)
3. Make selectable at runtime (optional)
   - `src/resources/World.cpp`: extend `resolve_shape(...)`
   - if needed, update defaults in `src/world/RuntimeConfig.h` and `src/world/SceneConfig.cpp`
4. Validate
   - `cmake -S . -B build`
   - `cmake --build build -j`

Decision shortcut:

- new mesh type only -> steps 1 + 4
- new mesh file too -> steps 1 + 2 + 4
- runtime-selectable option -> steps 1 + 3 + 4
