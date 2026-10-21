# Native fullscreen colour filter

The original `doColorFilter` captures the scene and computes a clamped bias plus
three weighted texture channels. The native implementation uses four PICA
texture-combiner stages; it initializes the remaining two stages as pass-through.
It replaces the entire source function body, preserving its callers and their
ordering. It does not interpret GX state at runtime.

`SCREEN COLOR` is an independent effects-word option, off in all four presets.
When disabled, unavailable, globally suppressed, or called before renderer
initialization, the owner returns before allocation, synchronization or drawing.

Each enabled invocation captures the current framebuffer into a dedicated image
and draws the filtered image back. This matters because the source can call it
twice in one frame, with viewfinder rendering between the calls. It cannot reuse
a once-per-frame capture or overwrite the particle reflection image.

The owner follows the existing native motion-capture coordinate mapping and
ordered command-list splits. The input and output attachments never alias.
Persistent geometry, attributes, buffers and material descriptions are prepared
once. Only three material constants change per invocation. The existing auxiliary
boundary retires producers and invalidates cached scene/material ownership;
this follows the pinned Melee renderer's explicit invalidation practice.

Resource costs when enabled:

- 512 KiB VRAM for a 512×256 RGBA8 snapshot.
- 512 bytes linear memory for two immutable six-vertex quads.
- 608 bytes of named static owner storage in the linked normal ELF, including
  240 bytes of texture-combiner descriptions. This excludes linker padding and
  the SDK render-target allocation.
- Two draws and twelve vertices per source invocation; existing UI shader only.
- Admission requires an additional 1 MiB VRAM and 15% linear-heap reserve.

No per-pixel CPU work, geometry reconstruction, worker, CPU framebuffer readback
or game-state change is introduced. The source's 320×240 capture and this native
snapshot are not proven sample-for-sample equivalent. Frame cost, allocation
headroom alongside other enabled effects, and physical-console behaviour remain
unvalidated. No performance improvement is claimed.

Evidence:

- `screen-color-test/result.json`: executes original GX setup and checks
  1,572,864 ideal RGB comparisons, opaque alpha, all modulation groups, actual
  SDK descriptors and strict ARM11 compilation; four incorrect mutants rejected.
- `diagnostic/screen-color-gpu-v1`: 96 actual GPU material cases, maximum error
  1.302 byte levels; RGB images inspected.
- `diagnostic/screen-color-owner-v1` and `v2`: actual native owner with distinct
  quadrants, single and repeated captures, OFF before allocation and after use.
  Maximum interior-pixel error 1.239 byte levels. V2 additionally checks the
  unavailable, no-render and uninitialized paths. All owned processes exited.
- `screen-color-integration/result.json`: archived adapter pipeline replaces the
  complete original function; native owner compiles against the ARM SDK.
- `diagnostic/screen-color-owner-v3`: injected insufficient VRAM/linear budgets,
  failed texture/linear allocations and failed target creation. All five paths
  release their unpublished resources and suppress further attempts until the
  unavailable mask is cleared. The successful GPU cases still pass afterward.

The menu path is gated by the original `CHEAT_SEPIA_MODE` unlock; sequence
commands 46/47 independently enable/disable monochrome. No unlock is modified
for testing, and an ordinary cave boot does not establish effect coverage.

These are local synthetic checks. Application runtime acceptance, source-scene
images, coexistence with other post effects and
hardware timing remain open. The cave-rescue crash is a separate unresolved issue.
