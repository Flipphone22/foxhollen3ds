# Load-prepared water distortion prototype

14 October: implemented a native load-time mesh compiler and a separate PICA
vertex program, now connected in the local replay candidate. **Not deployed or
accepted for performance or original visual parity.** See the later sections
for GPU and gameplay evidence; installed rendering is unchanged.

The source wake indirect matrix selects signed B/G from the original tiled
RGB5A3 texture, with scales0.25 and0.125 for the two reflection passes. Original
`newshadows.c:updateReflectionTextures` captures320x240. The proposed shader
receives normalized displacement scales explicitly; native capture resolution
must not silently change the strength of the source effect. Each projected
sample keeps its own Q. Shader interpolation approximates dependent fragment
sampling; it is not a general GX indirect-texturing implementation.

`pica_water_grid.c` prepares clamped bilinear source samples at grid centres
and boundaries. One16-byte, padded SHORT4 vertex stores Q14 UV and signedQ4
displacement. The PICA reconstructs quad positions from live origin/extent,
transforms them, and applies displacement multiplied by the projected Q. No
texture sampling, vertex rebuilding or allocation is proposed per frame.
The31-instruction shader keeps existing mask/fog output semantics. It is separate
from the23-instruction sheet program; no existing shader is replaced yet.

This adapts pinned Melee's persistent geometry and SDK binding discipline,
including its full-fetch/cache concerns. The new compiler owns no GPU memory,
submits no commands and creates no threads. Integration must publish its final
allocation once and retain it until queued readers retire; those lifecycle
requirements are not satisfied merely by a pure compiler test.

The size query reserves the full grid. `compile_visible` then removes only cells
whose complete clamped bilinear alpha support is zero and compacts referenced
vertices. It does not use a low-opacity threshold. Maximum temporary remap is
8712bytes; measured ARM local stack for that function is8880bytes, plus nested
calls. This is load-time work only. No heap allocation occurs in either function.

| Grid step | Untrimmed triangles | Retained triangles | Used vertex/index bytes |
|---|---:|---:|---:|
| 1 | 4290 | 2376 | 36944 |
| 2 | 1122 | 780 | 12088 |
| 4 | 306 | 238 | 3828 |
| 8 | 90 | 78 | 1332 |

Source pixels stay private. The converter/compiler contains no game-derived
pixel table or prepared geometry; it uses the supplied texture at load time.

`tools/test_pica_water_grid.py` runs the actual compiler under ASan/UBSan and
strict ARM compilation. All four grid levels reproduce independently decoded
source samples exactly for this texture; generalQ4 rounding is bounded by1/32
channel level. Indices cover the original domain with consistent winding.
Trimming retains precisely the independently checked nonzero-support cells,
with unchanged vertex values/topology. Empty and opaque masks, capacities,
invalid dimensions and no-output-write failure paths are covered.

`tools/test_pica_water_grid_shader.py` executes65536 vertices in the pinned
PICA interpreter with independently varying projective matrices and per-pass
scales. Worst relative error2.12063e-5. Geometry, opacity, mask and fog outputs
are bit-identical between passes. Missing displacement-Q, wrong UV fraction and
lost fog-depth mutants fail. The program assembles to31/512instructions with
96 declared float registers (including ABI padding), zero booleans. This is
vertex arithmetic evidence, not raster or physical throughput validation.

`tools/audit_water_grid_pixels.py` evaluates actual compiled meshes against a
per-pixel coordinate calculation using a private Thorntail image as reflection
content. The comparison image was inspected. Across8x8 through128x64 footprints,
coarse grids sometimes perform worse than omitting distortion. Do not promote
them as useful LODs based solely on smaller counts. For the128x64 positive-Q
case, mean RGB error /95th-percentile pixel error are:

| Method | Mean channel error | Pixel error p95 |
|---|---:|---:|
| No distortion | 4.406 | 33.362 |
| 90 triangles | 5.042 | 36.232 |
| 306 triangles | 4.704 | 35.870 |
| 1122 triangles | 3.748 | 28.902 |
| 4290 triangles | 0.677 | 5.536 |

These are software byte-level image differences with continuous alpha and
base-level filtering, not original-game captures or proof of live visual quality.
Maximum error in the finest case is still49.79 at isolated pixels. Zero-alpha
trimming preserves this model, but live converted-mask filtering must be checked
before applying it. Filtered five-bit bump alpha, dynamic ripple masks, source
animation/image comparisons, actual PICA rasterization and hardware cost remain
unvalidated. No new FPS claim follows. Current host compilation prevents a
clean matched performance run; no additional long game boot was started.

Evidence directories under `artifacts/research/pica-scene-rewrite`:
`water-grid-test`, `water-grid-shader-test`, `water-grid-pixels`.
The next useful step at this checkpoint was actual GPU pixel validation and a
bounded cost check, not coarse-grid admission or an unmeasured dense rollout.

## GPU owner and captured pixels

The isolated `pica_water_grid_patch.py` candidate now connects the compiler to
the existing two-pass wake owner. It prepares on the CPU once at load, compacts
the mesh into one36992-byte aligned linear allocation and flushes it once.
The original32x64 RGB5A3 mask is required; unsupported formats/layouts reject
the optional effect instead of reinterpreting data. All four preparation/
linear/image allocation failure paths release their partial resources. Runtime
adds three small uniform publications per pass; no per-frame mesh rebuilding,
extra pass or worker. Existing split/state restoration and queued-reader
retirement remain in force. This is a candidate, not performance admission.

Actual-owner ASan/UBSan checks cover128 lifecycles and2562 draws, split poisoning,
clipping, immutable prepared data, retirement and four allocation-pressure
failures. Missing retirement, missing load cleanup and missing grid uniform
mutants fail. The tested owner hash equals the linked replay owner source.

The silent GPU fixture uses the original private mask and actual candidate
owner, compiler,31-instruction shader and snapshot SDK. It runs eight projected
sampling/mix/opacity/occlusion cases, forcing a real split and different prior
shader/material before all16 draws. Captured images were inspected against the
independently evaluated grid. Mean channel error is at most2.159 and p99 pixel
error7.343 byte levels across46756 visible pixels. Zero-opacity/foreground cases
match exactly. Q-scaled and unscaled cases are pixel-identical after excluding
the changing8x8 fixture marker.

The initial test wrongly included that marker in its final Q comparison and
failed at4 levels. The original failure is preserved. Corrected evaluation uses
the already captured images and records a separate `post-validation.json`; no
second good-case emulator run was made. The zero-offset mutant fails as intended,
with p99 errors up to177.679 and mean7.175, demonstrating that this test cannot
pass merely by drawing an undistorted reflection. Both owned emulators exited.
This fixture still stubs fragment fog and does not prove original-game parity
or physical behavior. Pixel quantization and the grid approximation remain
distinct from exact GX indirect sampling.

Evidence: `artifacts/native-water-sheet-device/grid-v1/` (post-validation),
`grid-zero-offset-v1/` (expected failure), and `water-grid-owner-test/result.json`.
Replay source `f342a6e016d6` builds successfully in
`water-grid-v1-console-replay-builds`, retaining the exact game archive and all
17 other shader programs. The linked water shader matches the tested bytes;
Old80/New124 profiles and worktree/archive restoration pass. It is **not
deployed**. Gameplay and cost results must be recorded separately below.

## Real gameplay and remaining cost gate

Serial `water-grid-v1-on-new` and `water-grid-v1-on-old` complete the ordinary
Thorntail movement route, save checks, log drain and owned emulator exit in84.64
and78.75 host seconds. New draws228 wakes/456 passes; Old279/558. Both report
zero unavailable graphics resources and zero CPU-generated runtime vertices.
The1740 images were inspected: world, Fox and HUD remain present, with the Old
touch item pad. Those views do not isolate small wake pixels well enough for
original visual acceptance; use the explicit fixture scope above.

| Observed resource | New | Old |
|---|---:|---:|
| Heap used / reserved | 49210984 /71303168 B | 41674328 /50331648 B |
| Linear minimum free / reserve | 16702848 /25165824 B | 2025856 /10485760 B |
| VRAM minimum free | 4690944 B | 4690944 B |
| Process commit / grant | 103362560 /130023424 B | 67710976 /83886080 B |
| Measured stack / reserve | 73044 /524288 B | 73044 /524288 B |

These are recorded samples, not complete allocation-watermark proofs. The
additional61644-byte preparation buffer is temporary CPU heap storage; final
geometry is36992 linear bytes. Source texture and shared capture costs remain.

Both runs are excluded from timing comparison because another project was
compiling. Guest cadence57.05/51.24FPS is not a physical prediction or accepted
speed measurement. Owner spans0.0872/0.0746ms per wake include scheduling and
exclude shared capture and game-side matrix preparation. No added-effect
CPU/GPU cost gate has passed. WAKES remains OFF in every preset.

The geometry is a real workload:4752 triangles per active wake across two
passes. At the source pool limit30, that is142560 submitted triangles per frame.
Actual vertex execution depends on hardware reuse/cache behavior; the2376
triangle count must not be presented as a negligible GPU cost merely because
the CPU still issues two calls. Redesign or measured admission is required
before enabling it by default. `water-sheet-runtime/grid-result.json` retains
the resource/correctness report without overwriting earlier plain-sheet runs.

## Correction: ripple map lifetime

Earlier notes called the ripple bump mask dynamic. Source inspection shows
`newshadows.c:allocLotsOfTextures` allocates the64x64 RGB5A3
`gNewShadowBumpTexture`, fills it procedurally and flushes it once. Its only
other source use loads that texture in `loadNewShadowBumpTexture`; the full C
source search found no per-frame producer. The dynamically changing ripple
pool supplies transforms and opacity, not a changing bump image.

`setupReflectionBumpDistortTev` uses this generated texture, not the supplied
authored0x56 handle. Its indirect components are A/B and coverage is G
(`GX_ITBA_U`), unlike the wake's B/G offsets and A coverage. Ripple UV endpoints
are127/128, so reusing the wake sampler/domain without adaptation would be
incorrect. A future implementation can consume the original generated pixels
at load, preserving that generator, with no new dynamic render target or game
data shipped. The current compiler is still wake-specific; ripple integration,
coverage quantization, appearance and cost remain incomplete.

Matching normal setup build `7c9b805ef28e` (CIA `fa2219e783de`) is complete,
not deployed. Its606 recorded source hashes,18 shader programs and game archive
match replay `f342a6e016d6` (CIA `a3ffb95fc6cd`). Shared Old80/New124 profiles,
installation recipes and finally restoration checks pass. The normal app was
not rebooted redundantly; this is exact-build identity plus replay runtime
evidence, not a separate normal-app runtime claim. Verification is recorded in
`water-sheet-runtime/grid-build-verification.json`. All owned processes ended;
no FTP, saves or installed files changed.
