# Prompt Catalog

Ready-to-use `task_command` prompts for the agent macros.  
Copy the command line or use the GitHub Actions **Run workflow** button.

> **How to read this file:**  
> Each prompt is filed under the macro it's designed for.  
> The `fish` command is ready to paste into your terminal.  
> Difficulty, scope, and which agents run are listed per prompt.

---

## Quick Reference

| Macro | Agents | Purpose | Produces patch? |
|---|---|---|---|
| `Charge` | C++ Lead → Vulkan Guru | Surgical fix, one cross-check | Yes |
| `Pull` | Lead → Guru → Expert → Refactorer | Recon, no patches | No |
| `Follow` | All 5 agents | Full implementation cadence | Yes |
| `Think` | Guru, Expert, Refactorer (isolated) | Independent strategy | No |
| `Guild` | Guild Master | Governance assessment | No |

---

## Charge Prompts

Small, surgical changes. You know what you want — just do it safely.

---

### C01 — Enable Conway birth/death rules

The simulation loop in `Engine.comp` has Conway neighbour logic commented out.  
Re-enable birth (3 neighbours) and survival (2–3 neighbours) gating with a  
guard so the existing movement logic still applies to alive cells.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge \
  -f task_command="Re-enable Conway birth/death rules in Engine.comp. The neighbour-count logic is commented out — uncomment and integrate it with the existing movement/growth system so dead cells can be born (3 neighbours) and alive cells die (<2 or >3). Keep the size-transfer and follow-motion paths intact."
```

**Scope:** `shaders/Engine.comp`  
**Difficulty:** S — one file, logic is already written  

---

### C02 — Add pause/step/rewind to Timer

Timer only runs forward. Add pause (Space), single-step (N), and speed  
controls (+/−) to the keyboard handler + Timer class.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge \
  -f task_command="Add pause, single-step, and speed control to the simulation timer. Space = toggle pause, N = advance one hour while paused, +/- = adjust timer_speed (clamped 1–100). Modify src/control/Window.cpp key callback and src/control/Timer.h/.cpp. Log state changes."
```

**Scope:** `src/control/Timer.*`, `src/control/Window.cpp`  
**Difficulty:** S — keyboard + timer state  

---

### C03 — Improve PostFX: add vignette + saturation

PostFX.comp currently only does contrast + gamma. Add a subtle vignette  
(darken edges) and saturation control using the existing storage-image path.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge \
  -f task_command="Enhance PostFX.comp: add vignette effect (smooth radial darkening from center, configurable radius+strength) and saturation adjustment (luminance-based desaturation/boost, default 1.1). Keep existing contrast and gamma. All values as constants at the top of the shader."
```

**Scope:** `shaders/PostFX.comp`  
**Difficulty:** S — one shader, purely additive  

---

### C04 — Wire Water pipeline into the render graph

Water.vert/Water.frag exist but aren't in the active pipeline set.  
Register them in SceneConfig for stages 3–4 and add a `PipelineDefinition`.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge \
  -f task_command="Wire the existing Water.vert/Water.frag shaders into the active render graph. Add a Water PipelineDefinition in SceneConfig, register it in stages 3 and 4 (after Landscape, before Cells). Use draw_op indexed:grid with alpha blending enabled. Compile the SPIR-V files."
```

**Scope:** `src/world/SceneConfig.*`, `shaders/Water.vert`, `shaders/Water.frag`  
**Difficulty:** S-M — pipeline plumbing  

---

### C05 — Add F5 screenshot with timestamp overlay

The screenshot system captures raw frames. Add a text overlay (date + frame  
number) burned into the image before saving.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge \
  -f task_command="When F12 triggers a screenshot, burn a small timestamp overlay (YYYY-MM-DD HH:MM:SS, frame number) into the bottom-left corner of the captured image before writing to PNG. Use stb or direct pixel writes — no external font library. Keep the existing Screenshot::capture flow."
```

**Scope:** `src/library/Screenshot.*`  
**Difficulty:** S — pixel-level text rendering  

---

### C06 — Cell type differentiation via states.x

All cells currently behave identically. Use `states.x` as a cell-type  
index (0–3) to assign different colors and growth rates per type.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge \
  -f task_command="Differentiate cells into 4 types using states.x (0=gatherer, 1=scout, 2=builder, 3=dormant). In Engine.comp: type affects follow-radius (scout=6, gatherer=3, builder=2, dormant=1), growth multiplier (builder 1.5x, gatherer 1.0x, others 0.8x), and color palette (assign distinct hues per type). In SeedCells.comp: assign types round-robin during initialization."
```

**Scope:** `shaders/Engine.comp`, `shaders/SeedCells.comp`  
**Difficulty:** M — GPU logic, multiple behavioral paths  

---

## Pull Prompts

Recon missions. Map the terrain before committing the party.

---

### P01 — Shadow mapping feasibility

No shadow system exists. Map what it would take to add cascaded shadow maps  
or a simple depth-pass shadow for the directional light.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull \
  -f task_command="Map the feasibility of adding shadow mapping to GENERATIONS. Assess: what render pass changes are needed, where does the shadow depth pass fit in the frame sequence, what descriptor/UBO additions are required, which shaders need shadow sampling, and what's the performance budget impact. Do NOT write patches — recon only."
```

**Scope:** `src/vulkan_pipelines/*`, `src/vulkan_base/*`, `shaders/Landscape*`, `shaders/Cells*`  
**Difficulty:** L — cross-cutting  

---

### P02 — LOD system for terrain and cells

No LOD exists. Map what distance-based detail reduction would require for  
the terrain mesh and cell instancing.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull \
  -f task_command="Map the requirements for a LOD system. Terrain: can terrain_render_subdivisions be made distance-adaptive per chunk? Cells: can instance data be culled or reduced by distance? Shapes: can simpler meshes substitute at distance? Map all files touched, pipeline changes needed, and GPU-side vs CPU-side tradeoffs. Recon only."
```

**Scope:** `src/world/*`, `src/vulkan_resources/*`, `shaders/Cells*.vert`, `shaders/Landscape.vert`  
**Difficulty:** L — architecture question  

---

### P03 — Terrain CPU readback options

Terrain height is GPU-only. Map how to get terrain data back to the CPU  
for placement validation, pathfinding, or physics.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull \
  -f task_command="Map options for CPU-side terrain height queries. Currently TerrainField.glsl is GPU-only. Options: (A) port the FBM noise to C++, (B) GPU readback via staging buffer, (C) compute shader that writes a height-map texture readable on CPU. For each: files touched, sync implications, frame-time cost, and accuracy guarantees. Recon only."
```

**Scope:** `shaders/TerrainField.glsl`, `src/world/*`, `src/vulkan_resources/*`  
**Difficulty:** M — research  

---

### P04 — Multi-light and ambient occlusion readiness

Single light source, no AO. Map what the UBO, shaders, and descriptor  
layout need to support multiple lights and screen-space AO.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull \
  -f task_command="Map what's needed for multi-light support (up to 4 lights) and screen-space ambient occlusion. Assess: UBO layout changes in ParameterUBO.glsl/ShaderInterface.h, descriptor set additions, which fragment shaders need modification, SSAO compute pass integration into frame sequence, and performance budget. Recon only."
```

**Scope:** `shaders/ParameterUBO.glsl`, `src/vulkan_pipelines/ShaderInterface.h`, `shaders/Landscape.frag`, `shaders/Cells.frag`  
**Difficulty:** L — cross-cutting  

---

### P05 — Save/load simulation state

No persistence exists. Map what a save/load system needs — which buffers  
to serialize, file format, and resume flow.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull \
  -f task_command="Map requirements for saving and loading simulation state. What needs serializing: cell SSBO data (positions, states, sizes), timer state (passedHours, dayFraction), camera position, grid dimensions, scene config. What file format (binary, JSON header + binary blob)? How to pause simulation during save, read back GPU buffers, and restore them on load. Map all files involved. Recon only."
```

**Scope:** `src/engine/*`, `src/world/*`, `src/vulkan_resources/*`  
**Difficulty:** M — architecture  

---

### P06 — Dynamic grid resizing

Grid is fixed at construction. Map what it would take to resize the grid  
at runtime without restarting the engine.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull \
  -f task_command="Map the feasibility of runtime grid resizing. Currently grid_width/grid_height are set once in World/Grid construction. What buffers need reallocation (SSBOs, vertex/index, terrain mesh), what descriptor sets need updating, what UBO fields change, which shaders have hardcoded assumptions? Can it be done without full pipeline recreation? Recon only."
```

**Scope:** `src/world/World.*`, `src/vulkan_resources/*`, `src/vulkan_pipelines/*`, `shaders/Engine.comp`  
**Difficulty:** L — deep infrastructure  

---

## Follow Prompts

Full implementation cadence. All 5 agents, sequential, produce patches.

---

### F01 — Water rendering with basic animation

Bring the water pipeline to life: register it, add wave animation in the  
vertex shader, improve the fragment shader with Fresnel-like transparency.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow \
  -f task_command="Implement proper water rendering. (1) Register Water pipeline in SceneConfig for stages 3-4 with alpha blending. (2) Water.vert: add time-based sine wave displacement using passedHours/dayFraction push constants. (3) Water.frag: add view-angle-dependent transparency (Fresnel approximation), subtle blue-green tint, and specular highlight from ubo.light. (4) Ensure depth testing works correctly with terrain. (5) Update docs."
```

**Scope:** `shaders/Water.*`, `src/world/SceneConfig.*`, `shaders/ParameterUBO.glsl`  
**Difficulty:** M — multi-file, cross-domain  

---

### F02 — Bloom post-processing pass

Add a bloom effect using the existing compute post-processing path.  
Bright-pass extraction → Gaussian blur → additive composite.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow \
  -f task_command="Add bloom post-processing via compute shaders. (1) New Bloom.comp that extracts bright pixels above a threshold from the storage image, applies a separable Gaussian blur (9-tap, two passes: horizontal then vertical), and composites additively back. (2) Register as post-compute pipeline in SceneConfig. (3) Use a second storage image as scratch buffer for the blur ping-pong. (4) Make bloom intensity and threshold configurable as push constants or UBO fields. (5) Update ParameterUBO if needed."
```

**Scope:** `shaders/Bloom.comp` (new), `src/world/SceneConfig.*`, `src/vulkan_resources/*`, `shaders/ParameterUBO.glsl`  
**Difficulty:** M-L — new pipeline + resource allocation  

---

### F03 — Terrain self-shadow enable and improve

The self-shadow system exists behind a disabled `#define`. Enable it,  
improve the ray-march quality, and make it configurable.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow \
  -f task_command="Enable and improve terrain self-shadows. (1) Set CE_ENABLE_TERRAIN_SELF_SHADOW=1 in LandscapeShared.glsl. (2) Improve the ray-march from 4 to 12 samples with exponential step sizing. (3) Add shadow softness via penumbra estimation (use closest-blocker distance). (4) Make shadow intensity a UBO parameter (add to ParameterUBO.glsl + ShaderInterface.h). (5) Test with directional light at oblique angles — ensure no self-shadow acne."
```

**Scope:** `shaders/LandscapeShared.glsl`, `shaders/Landscape.frag`, `shaders/ParameterUBO.glsl`, `src/vulkan_pipelines/ShaderInterface.h`  
**Difficulty:** M — shader + interface  

---

### F04 — Wireframe + tessellation terrain mode

LandscapeWireFrame shaders exist but aren't in the active graph. Wire them  
up as a stage-strip tile with proper tessellation pipeline state.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow \
  -f task_command="Wire the existing LandscapeWireFrame shaders (vert + tesc + tese + frag) into the live render graph. (1) Create a WireFrame PipelineDefinition with tessellation pipeline state and polygon mode line. (2) Register as a new render stage (stage 5) or as a stage-strip tile. (3) Tessellation control: distance-adaptive subdivision (close=high, far=low). (4) Tessellation evaluation: apply TerrainField displacement. (5) Make it toggle-able via the stage strip."
```

**Scope:** `shaders/LandscapeWireFrame.*`, `src/world/SceneConfig.*`, `src/vulkan_pipelines/Pipelines.*`  
**Difficulty:** M-L — tessellation pipeline state  

---

### F05 — Particle system using existing instance infrastructure

The cell instancing architecture can be reused for a particle system.  
Add an emitter compute shader and a particle rendering pipeline.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow \
  -f task_command="Add a GPU particle system reusing the cell instancing architecture. (1) New ParticleEmit.comp: emit particles from alive cell positions, give each particle velocity (upward + random spread), lifetime, and color inherited from parent cell. (2) New ParticleUpdate.comp: integrate position, apply gravity, fade alpha by remaining lifetime, kill expired particles. (3) New Particles.vert/frag: render as billboard quads or points with additive blending. (4) New SSBO for particle buffer (binding 5). (5) Register in SceneConfig stage 4. Max 10000 particles."
```

**Scope:** New shaders, `src/world/SceneConfig.*`, `src/vulkan_resources/*`, `shaders/ParameterUBO.glsl`  
**Difficulty:** L — new subsystem  

---

### F06 — Day/night cycle with sky color transitions

The sky dome uses static colors. Use the existing `dayFraction` clock to  
drive sky color, light direction, and ambient intensity through a full cycle.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow \
  -f task_command="Implement a day/night cycle driven by dayFraction from push constants. (1) Sky.frag: interpolate horizon/zenith colors between dawn, noon, dusk, and night palettes based on dayFraction. Move sun position along an arc. (2) Landscape.frag: modulate ambient and diffuse intensity by time-of-day. (3) Cells.frag: same ambient modulation. (4) Add light direction as a UBO field that rotates with the cycle (update in UniformBuffer::update). (5) PostFX.comp: adjust gamma curve for night scenes."
```

**Scope:** `shaders/Sky.frag`, `shaders/Landscape.frag`, `shaders/Cells.frag`, `shaders/PostFX.comp`, `shaders/ParameterUBO.glsl`, `src/vulkan_pipelines/ShaderInterface.h`  
**Difficulty:** M-L — multi-shader coordination  

---

## Think Prompts

Independent theorycrafting. Three specialists argue in isolation.

---

### T01 — How to approach terrain chunking for large worlds

The terrain is a single mesh. Think about how to chunk it for larger  
worlds with streaming, LOD, and culling.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think \
  -f task_command="How should GENERATIONS approach terrain chunking for worlds larger than the current single 25x25 grid? Consider: chunk size vs draw call overhead, GPU-side frustum culling, LOD per chunk, seamless stitching at chunk boundaries, streaming chunks in/out, and interaction with the cellular simulation grid. Each specialist proposes their own approach."
```

**Difficulty:** Strategy — architecture question  

---

### T02 — Cell intelligence: emergent behavior strategies

Cells currently just follow neighbours. Think about how to create  
genuinely emergent behavior — flocking, resource competition, evolution.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think \
  -f task_command="Design emergent cell behavior beyond simple neighbour-following. Consider: flocking rules (separation, alignment, cohesion as GPU compute), resource competition (cells compete for terrain resources, winners grow, losers shrink/die), evolutionary pressure (cell types that perform well reproduce more), pheromone trails (cells leave data in a grid texture that influences others). Propose your approach independently."
```

**Difficulty:** Strategy — simulation design  

---

### T03 — Real-time parameter editing: ImGui vs custom UI

No runtime parameter editing exists beyond camera tuning keys. Think  
about how to add it — full ImGui integration vs extending the stage strip.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think \
  -f task_command="How should GENERATIONS add real-time parameter editing? Options: (A) Integrate Dear ImGui with Vulkan backend for full slider/panel UI. (B) Extend the existing stage-strip tile system into a custom control panel. (C) Use env vars + hot-reload config file. Consider: build complexity, Vulkan descriptor set impact, frame-time cost, development speed, and what parameters matter most (grid size, water threshold, cell size, noise params, light, timer speed, PostFX settings)."
```

**Difficulty:** Strategy — tooling/UX  

---

### T04 — Compute pipeline architecture for multi-pass simulation

The engine has a configurable compute chain but only uses Engine.comp.  
Think about how to architect a multi-pass simulation with data dependencies.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think \
  -f task_command="Design a multi-pass compute simulation architecture. The compute chain infrastructure exists (ComputeInPlace, ComputeJitter, ComputeCopy) but isn't fully utilized. Consider: what simulation passes should exist (physics, AI/behavior, resource transfer, terrain interaction, spatial indexing), how data flows between passes (barriers, separate SSBOs, shared buffers), how to specify dependencies, and how the render graph integrates with compute ordering."
```

**Difficulty:** Strategy — compute architecture  

---

### T05 — Procedural audio from simulation state

No audio exists. Think about generating sound from the simulation — cell  
density, movement patterns, time-of-day — as procedural ambient audio.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think \
  -f task_command="Design a procedural audio system driven by simulation state. Consider: what simulation data maps to sound (cell density→volume, average cell velocity→pitch, time-of-day→ambient mood, birth/death events→stingers), how to read GPU simulation state for audio (readback vs CPU-side proxy), which audio library to use (miniaudio, OpenAL, SDL_audio), and how to mix multiple procedural layers without latency spikes."
```

**Difficulty:** Strategy — new domain  

---

### T06 — Network multiplayer: shared simulation state

Think about how multiple clients could observe or interact with the same  
simulation — what needs to be synchronized and how.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think \
  -f task_command="Design a networked multiplayer approach for GENERATIONS. Consider: is the simulation deterministic enough for lockstep? If not, what state needs server-authoritative sync (cell positions, births, deaths)? What bandwidth does cell SSBO delta compression need at 25x25 (625 cells × 5 vec4)? Can clients run independent cameras on shared simulation? What interaction model — spectator, environmental influence, competitive territory? Network library: ENet, GameNetworkingSockets, raw UDP?"
```

**Difficulty:** Strategy — systems architecture  

---

## Guild Prompts

Governance assessment by the Guild Master.

---

### G01 — Weekly performance assessment

Standard weekly review. Run after accumulating a few agent runs.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Guild \
  -f task_command="Weekly governance assessment. Review trailing metrics, agent profiles, guild policies, and recent run reports. Produce class performance scorecards, identify any specialization drift or underperformance, and issue CLASS-CHANGE or GUILD-POLICY directives if warranted."
```

---

### G02 — Post-incident review

Run after a HALT or high-dissent run to diagnose what went wrong.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Guild \
  -f task_command="Post-incident review. A recent run triggered a pipeline HALT or had >50%% dissent. Review the metrics and run report to diagnose: which agent caused the halt, was the dissent justified, should quality gate thresholds be adjusted, and are agent directives clear enough to prevent recurrence? Issue GUILD-POLICY directives to address root cause."
```

---

### G03 — Agent alignment audit

Deeper audit of whether agent profiles still match their actual outputs.

```fish
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Guild \
  -f task_command="Agent alignment audit. Compare each agent's profile (stated responsibilities, owned files, guild membership) against their actual outputs in recent runs. Are agents staying in lane? Are any agents consistently doing work outside their profile scope? Are there coverage gaps — areas no agent claims? Issue CLASS-CHANGE directives for any misalignment found."
```

---

## Combo Sequences

Multi-step workflows using macros in sequence.

---

### Combo 1 — New feature lifecycle

```fish
# Step 1: Understand the terrain
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull -f task_command="<describe the feature area>"

# Step 2: Compete on strategy
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think -f task_command="<design question from Pull findings>"

# Step 3: Implement the winner
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow -f task_command="<implement based on Think convergence>"

# Step 4: Review the aftermath
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Guild -f task_command="Post-implementation review of <feature>"
```

### Combo 2 — Bug investigation

```fish
# Step 1: Surgical fix attempt
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Charge -f task_command="<describe the bug>"

# Step 2: If Charge dissented — deeper recon
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull -f task_command="<same bug, deeper investigation>"

# Step 3: Full fix with all perspectives
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow -f task_command="<implement the fix>"
```

### Combo 3 — Architecture decision

```fish
# Step 1: Three independent strategies
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Think -f task_command="<architecture question>"

# Step 2: Recon the winning approach
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Pull -f task_command="<scout the chosen approach in detail>"

# Step 3: Execute
gh workflow run "Agent Autonomous Run" --ref dev-linux \
  -f macro=Follow -f task_command="<implement the architecture>"
```
