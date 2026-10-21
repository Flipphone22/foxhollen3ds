11 October latest local normal`56340ab2a26a` / replay`f1bb348b9800` adds
LIGHT MAPS on page10, second row beside SOURCE FUR. It defaults OFF, can be
toggled during ordinary gameplay without restarting, and persists on Select
exit. Replay/record locks apply. Single-layer native diffuse materials are
supported; masked/layered/specular combinations remain partial. This build is
**not deployed**. Its normal touch/toggle/persistence/exit check passes with
the page inspected. See [scope and performance evidence](NATIVE_DIFFUSE_MAPS.md).

Installed New223 remains normal`f50343cd2e56` / replay`e8eefd5944cf`; Old229
remains normal`697b6b1d9794` / replay`3c0f3e2d7aa7`. Later controls in a local
candidate are not available on hardware until that candidate is installed.
The entries below retain the historical deployment and implementation record.

11 October New223 deployment complete: normal `626d217324e9` / CIA
`762ef1732702`, replay `9bcad101a4fe` / CIA `9acc42b6e738`, with matching
original-geometry22-slot pack and lossless cache. All18 changed payloads staged,
byte-readback verified, renamed and final-size checked. Saves/preferences
preserved; Old229 unchanged. CIA requires FBI installation; matching3dsx ready.
Performance-geometry22-slot pack remains local; original-asset installer does
not replace the generic performance pack. Earlier status entries are historical.
See [changes, tests and unresolved issues](RENDERER_UPDATE_20261011.md).

Current New deployment e783636ad1e8 includes optional soft circular ground
shadows (page 6, default OFF), distance fog (page 7, default ON), and independent
original geometry/textures (page 8, restart required). Old remains 697b6b1d9794.
These are partial native effects, not original rendering parity.

Local candidate: SHADOW SHAPE on page 7 selects an animated, hard planar mesh
silhouette when GROUND SHADOW is also ON. It defaults OFF on both models and
persists independently; it is not yet deployed. See NATIVE_PLANAR_SHADOWS.md
for receiver limitations and measured cost. Existing graphics options and
replay recipes keep their prior meaning.

Local normal576017df2dc6/replay6b4ec0a84e3a now include this selector plus
bounded receiver-result reuse and a cheaper material handoff. The ordinary
console binary's page7 touch control and saved FHGRAPH4 preferences were checked
in Azahar; shared Old/New replays also passed. Physical cost remains unknown,
and the New cave addedCPU~0.58ms still exceeds the0.5ms feature budget. Keep
GROUND SHADOW and SHADOW SHAPE off by default on both models. These builds have
not replaced either console installation.

# Graphics controls

Local morph candidate adds MORPHS on page 8, ON by default on both models.
It switches between live morph positions and the base mesh without a restart;
animation/gameplay continues in either setting. See NATIVE_MORPHS.md for cost,
ownership and validation scope. It is not deployed yet.

Local splash candidate adds SPLASHES as the third control on page 7; original
geometry/textures remain on page 8. SPLASHES defaults OFF on both models and is
independent of SHADOW SHAPE. It restores a partial native splash ring, not water
reflections or all water effects. See NATIVE_SPLASHES.md for validation limits.

Tap GRAPHICS on the bottom screen, then PREV/NEXT to change pages. Preferences
are saved on a clean Select exit. The replay application uses its recipe instead
of normal gameplay preferences.

The deployed texture/performance controls are:

- Page 2: FPS DISPLAY, PERF STATS and GPU COUNTERS. All default off.
- Page 8: ORIGINAL GEO and ORIGINAL TEX, independently selectable. Both require
  Select exit and relaunch. The menu shows RESTART REQUIRED while a requested
  profile differs from the running profile.

Original geometry selects source topology. Original textures select source
resolution, still encoded in PICA-compatible formats. Neither changes movie
resolution. Higher detail can increase rendering time and resident texture use.

With the menu closed, PERF STATS shows heap, linear and VRAM usage; queued audio
milliseconds and underruns; SD read rate and filesystem IPC time. Audio estimates
count whole queued blocks, including the playing block. Read/FS time includes
service and scheduling waits across workers, so it is not exclusive main-thread
blocking time. READ MAX is the largest observed read-call wall time this session.

GPU COUNTERS shows FRAME ms and GPU Q ms, averaged over the latest observation
interval. GPU Q is software-observed time with GPU commands outstanding, including
copies, service and interrupt latency. It overlaps CPU activity: do not add it to
CPU time or treat it as pure raster/shader execution time. Invalid queue events
suppress the timing result. The eight numbered raw hotspot counters below it
belong to a fixed 4096-sample window; they are not per-stage millisecond timings.
Unsupported counter reads are reported explicitly rather than converted to zeros.

Telemetry samples at most once per second. Hidden panels perform no recurring
sampling; unchanged displays do not redraw. Profiling itself has overhead.
Counter acquisition never busy-waits or reads an unfinished FIFO, and restores
the profiler registers on disable, suspend or exit. Recording/replay lock GPU
profiling changes to avoid changing the captured workload.

The subsequent native-fog build adds GROUND SHADOW on page 6 and DISTANCE FOG
on page 7; ORIGINAL GEO/TEX move to page 8. Fog defaults on. Contact shadows
are an optional soft-footprint approximation and default off: their live collision
queries still exceed the cave CPU budget. Enabling them can reduce FPS. This is
not a claim that the shadow performance problem has been solved.

FHGRAPH4 keeps both choices across sessions. Older FHGRAPH2/3 preferences retain
their existing graphics, diagnostics and asset-profile choices while introducing
contact shadows as off. No SD settings file is overwritten during deployment;
the application reads/migrates settings at startup and saves changed preferences
on clean Select exit. Replay recipes retain their explicit masks: GRAPHICS 1
enables the earlier effects but not the new opt-in silhouette/splash bits;
GRAPHICS 16385 includes fog and disables ground shadows. GRAPHICS 147457 also
opts into native splash rings. These masks do not enable missing effects.
