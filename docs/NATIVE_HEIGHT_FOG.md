11 October local publication-cache revision (not deployed): unchanged source fog
planes now remain resident across adjacent world draws. The native owner clears
its plane cache on camera/plane changes and every scene shader/owner handoff.
A separate stage3 cache is invalidated at the actual material SDK mutation sites,
including constant-only edits. Texture1 is deliberately rebound: ordinary material
binding still clears that slot, so caching it independently would be incorrect.
Boolean publication remains explicit. No geometry, shader, texture, game state,
asset, worker or per-update allocation changed. The pinned Melee renderer's
write-site raster-cache invalidation is the ownership reference; native shader
switches and material stage3 writes are the adaptations.

Actual-owner ASan/UBSan tests retain the million-selection resource-lifetime
coverage and add 10,000 unchanged draws plus 10,000 mixed parameter/material/
shader handoffs. They check actual mock SDK call counts and compile the actual
material mutation wrappers. Missing stage invalidation and missing uniform
invalidation mutants are rejected. The first mutant test changed fog color on
every draw and failed to detect missing stage invalidation; it was strengthened
with same-color clobbers before acceptance. Initial ARM compilation also rejected
a const qualifier incompatible with the SDK's SetTexEnv declaration; corrected
in v2. These failure records are retained, not claimed as passing evidence.

New serial baseline/candidate cave ON: scene 1.823245/1.833014 ms, native submit
5.663215/5.565165 ms; combined CPU difference -0.088280 ms. Same script/assets/
settings, but animation phases differ; this is a small short-run relative result,
not a physical throughput prediction. Both ~59.8312 Azahar FPS. Both frame1410
images inspected: world, Fox, HUD and visible fog retained; no pixel-exact claim.
The candidate publishes 715 plane pairs for 75,075 admitted draws (formerly one
pair per draw), and 43,615 stage updates. Actual remaining material handoffs are
not hidden. The same candidate OFF measures scene1.816845/submit5.340837 ms:
ON adds0.240497 ms combined CPU, still above the provisional0.20 ms budget.
HEIGHT FOG stays opt-in/OFF; this is not full fog-family acceptance.

Old candidate ON also completes (46.24 host seconds), state/save/drain/owned-exit
checks pass. Old Azahar cadence57.2134 FPS, scene1.817485/submit5.581146 ms;
no paired Old baseline or physical prediction. Its frame1410 capture retains
world/Fox/HUD but shows less haze than the New captures. Both report height
intensity0 and camera Y around-979, with almost identical final distance-fog
ranges. This repeats the earlier unresolved haze difference; neither source
phase nor a renderer fault is established as its cause. Do not claim cross-model
image parity. Old height publications682 pairs/71,610 draws,41,602 stage updates.
Old heap40,846,584/50,331,648 B; linearfree3,833,472/10,485,760 B; VRAMfree4,822,016 B;
stack73,008/524,288 B; process67,575,808/83,886,080 B.

New ON/OFF state assertions, isolated saves, log drain and owned-process exit pass.
ON heap48,383,224/71,303,168 B; minimum sampled free linear18,513,536/25,165,824 B;
free VRAM4,822,016 B; stack73,008/524,288 B; process103,227,392/130,023,424 B.
Same sampled heap/linear/VRAM/stack as baseline; executable commitment +4KiB.
Owner adds24B static cache/counters and32B temporary plane comparison storage.
No claim that this repairs physical SD stalls, white Arwing, disappearing trees,
fur appearance or complete lighting/shadow parity. All remain open.

11 October New223 normal deployment COMPLETE: 46baff465715/CIAced987c20ce7,
console-settings-update/20261011-023844-223. Four changed application/identity
payloads staged, byte-readback verified, renamed and final-size checked. Original
geometry/cache reused through prior complete byte proof plus current descriptors/
topology/sizes; no fresh bulk readback. Saves/settings preserved. Matching3dsx
ready at SD:/3ds/foxhollow/foxhollow.3dsx; CIA requires FBI installation at
SD:/3ds/foxhollow/foxhollow.cia. PendingVRAMreplay8e4bbc1c9563 andOld229 unchanged.

This normal update includes reduced duplicate memory reporting plus the previously
local opt-in HEIGHT FOG world family/immutable atlas. HEIGHT FOG stays OFF by
default; source camera-intensity motion and physical GPU cost remain unaccepted.
No claim that SD stalls, white Arwing, invisible trees or full fur/shadow/lighting
parity are fixed. Physical targets and overall goal remain INCOMPLETE.

FinalnormalNewheadless check memory-cadence-normal-v2-new PASS38.947hostsec,
startup/Start/cockpit/Select/drain/ownedexit, no native test hooks,900image inspected.
44memorysamples->14reports/28983B; finalstack watermark retained. Heap46277472/
71303168,linearfree18518656/25165824,VRAM4822016,process103186432/130023424,
stack72956/524288. Shared341native sources/archive/shader identity verified,
Old80/New124CIA. Native source changes vs latestatlasbaseline only platform_3ds.c
and fh_memory_cadence.h; game/renderer unchanged versus that LOCAL baseline.

# Native height fog: first world-material family

11 October local implementation, not deployed. This restores the original
height-fog lookup on single-texture and untextured world materials. It does not
resolve the reported missing Arwing/trees, SD pauses, actor lighting or fur.
`HEIGHT FOG` is a persistent bottom-screen option, off by default on both models.
Existing option bits, saves, gameplay and source asset bytes are unchanged.
Replay `8d85e463f74d` / CIA `2db50a013258`; normal `a33c15095766` /
CIA `b3bd3d7353c9`. Both variants share all 340 native source files, the
same game archive and verified shaders. CIA grants remain Old 80 MiB /
New 124 MiB. No new normal startup run: unchanged UI/startup evidence and
the shared renderer checks are reused.

## Source and ownership

The original `newshadows.c:updateHeavyFogTexture` produces a 64×64 I8 lookup
for 65 integer camera-height states. The native owner converts those actual
bytes from GX 8×4 tiles into PICA A8 8×8 tiles with the established vertical
orientation. It samples alpha; it does not substitute RGB luminance or a
different fog equation. Linear filtering and clamp-to-edge match the original
texture allocation. Updates occur only when the source intensity changes.

Three fixed texture slots consume at most 12,288 linear bytes, allocated on
the first admitted draw. The v2 owner has 200 bytes of ARM static state. The
existing frame owner retires a slot before overwriting it; explicit cache
publication precedes sampler publication. Shutdown frees after queue retirement.
There is no new worker, texture readback, VRAM transfer or per-update allocation.
The pinned Melee `command_cache.c` publication rule and `texture_vram.h` queued
transfer failure were consulted; this implementation uses retained linear
storage and the already verified Foxhollow fence owner.

The source `renderHeavyFog` first applies **PNMTX0**, then its fog texture
matrix. The world path supplies the block camera matrix; this must not be
generalized to a vertex's blended bone pose for actors. Two live plane uniforms
preserve source depth scale/offset, height range and inverse-view row. No CPU
vertex transformation or GX/TEV interpreter is introduced. Necessary CPU work
is a small block-boundary parameter snapshot, optional 4 KiB format conversion,
and material/command binding. These costs are included in rendering telemetry.

The new 26-instruction world entry lives in the shared material shader bank:
197 actor + 73 diffuse-map + 26 world = 296 instructions. The main 511-instruction
and fur 214-instruction programs remain unchanged. World geometry and draw
ordering remain resident and unchanged; there is no extra draw pass. A boolean
selects height-fog UVs or ordinary second-layer UVs without switching programs
between world materials. Stage 3 mixes source fog RGB with previous RGB using
the lookup alpha, preserving surface alpha. Colour fade remains stage 4 and
distance fog stage 5.

## Coverage and limitations

- Supported: untextured/single-texture world materials, original live fog colour,
  source lookup intensity, camera/height coordinates and distance-fog coexistence.
- Unsupported world families continue their ordinary native draw. The first
  consumer excludes multiple layers, masks and cloud/ring precedence. They are
  counted rather than omitted. Actors are not yet consumers.
- Original indirect caustic displacement is missing. Source mode nonzero is
  counted as partial; it currently uses the direct lookup. The tested cave uses
  mode zero, so this is not acceptance of the distorted-fog family.
- Physical GPU sampling, console cost, and matched original-game image
  comparison remain open. Height fog does not establish complete fog parity.

## Checks and measured cost

`artifacts/research/pica-scene-rewrite/height-fog-test` contains reproducible
source, lifetime, shader, ARM, build and runtime reports. Commands:

```
python3 tools/test_pica_height_fog.py
python3 tools/test_pica_height_fog_owner.py
python3 tools/test_pica_height_fog_shader.py
python3 tools/test_pica_height_fog_arm.py
python3 tools/test_graphics_options.py
python3 tools/test_graphics_settings.py
```

The exact source producer matches 266,240 texels across all 65 intensities.
ASan/UBSan owner checks cover 100 lifecycles, 13,101 updates, reuse, delayed GPU
readers, publication, wrap and shutdown. Missing fence/publication mutants fail.
Azahar's shader interpreter checks 32,768 vertices across both UV branches;
512 captured original matrices match exactly. These are not hardware precision
or timing proofs. Actual SDK ARM compilation checks the owner ABI/storage.
Graphics tests cover the added bit, touch ownership, recording locks, settings
persistence and idle no-flush behavior.

The first implementation switched programs per material. It was rejected on
cost: submission rose from 5.341 to 7.316 ms/frame. The replacement reduces
pipeline publications from 52,703 to 3,368 with identical whole-run world and
actor draw/index counts in those two ON runs. Both render 75,075 height-fog
draws; 37,180 world draws remain unsupported. Only one lookup conversion occurs.
That update spans 117,393 emulator ticks (about 0.438 ms, nested in submission).
The idle route therefore does **not** establish cost during repeated camera-height
changes. Reducing conversion work and testing that workload remain necessary.

Same-binary serial v2 cave comparisons:

| Azahar configuration | FPS | Scene CPU ms | Native submit ms |
|---|---:|---:|---:|
| New, OFF | 59.8312 | 1.83148 | 5.33449 |
| New, ON | 59.8312 | 1.84481 | 5.66422 |
| Old, OFF | 57.8175 | 1.81157 | 5.34840 |
| Old, ON | 56.8618 | 1.80769 | 5.68237 |

Added measured rendering CPU is approximately 0.3431 ms New / 0.3301 ms Old.
The ongoing incremental gate for this optional world subset is 0.5 ms New /
1.0 ms Old, subject to the overall frame budget. The Old cadence loss is retained
in the record, not described as unchanged performance. These are short emulator
comparisons with differing animation phase/workload; no universal physical FPS
conversion applies. No final performance target has been established.

ON memory peaks: New heap 48,383,272 / 71,303,168 bytes; Old heap 40,846,632 /
50,331,648. Linear minimum free: New 18,513,536 / 25,165,824; Old 3,833,472 /
10,485,760. VRAM minimum free 4,822,016. Measured main stack 73,008 / 524,288.
Process peaks: New 103,219,200 / 130,023,424; Old 67,567,616 / 83,886,080.
The legacy `resident_bytes=12288` log field denotes this owner's capacity,
including when OFF; `updates=0` means it never allocated. Use memory telemetry
and update counts, not that constant field, for actual residency.

All six runs passed state assertions, save preservation, log drain and owned
exit. Captures at retrace 1410 were inspected for New ON/OFF and Old ON: cave,
Fox, HUD and fogged world surfaces remain visible. Unsupported families are
visible too. This inspection does not validate every affected material.

Build helpers isolate and restore the six unrelated VRAM work-in-progress
files and game archive. `pica_height_fog_patch.py` has already been applied to
the working copies and the new stable snapshot; do not apply it twice there.
The new game archive recompiles `main/tex_dolphin.c` only. No console files or
saves were changed for this feature, and the installed VRAM replay is retained.

The world shader first comment is a leftover prototype label in these frozen
builds; the linked consumer and runtime counters establish its actual use.


## Immutable atlas revision (11 October, local)

The new sampler owner replaces the three mutable lookup slots with eight
64 x 128 A8 phase pages (65,536 linear bytes). Each intensity 0..64 has a
64 x 64 descriptor referencing a 512-byte-aligned interior tile row. The
sampler keeps its own 64 x 64 dimensions, so linear filtering and clamp edges
remain identical to the original. This is not UV cropping inside a larger
sampled image. Source texels, quantization and the existing planes are unchanged.

Preparation writes and publishes the atlas once. Subsequent native intensity
changes select an immutable descriptor: no pixel conversion, cache flush,
resource fence, allocation or clock call. Queued GPU readers may retain any of
the 65 descriptors until joined shutdown, which frees only the atlas owner.
The original game intensity producer remains intact; this change removes native
conversion work, not original simulation or the original source texture update.
The memory tradeoff is +53,248 linear bytes. ARM static owner storage is 1,732
bytes including bounded camera/intensity coverage counters. Atlas residency is
lazy and retained if the user switches the effect off during play.

`height-fog-atlas-test/source.json` compares 266,240 texels and 1,219,985
bilinear/clamp samples against the actual original producer, including all view
edges. ASan/UBSan owner tests retain all 65 delayed readers through one million
selections, cover 100 lifecycles, and reject missing publication, incorrect view
dimensions and freeing an interior pointer. The tests use SDK mocks for resource
ownership; actual SDK ARM compilation separately checks ABI and static storage.
These checks do not establish physical sampling or throughput.

A ladder route now moves the camera vertically with ordinary gameplay input.
The first old/new-owner runs both completed and preserved saves, and the new
owner's frame-1500 image was inspected (Fox on ladder, textured room and HUD).
An external read-only observer failed its executable mapping checks and captured
no state; its guards were not relaxed. That run is diagnostic, excluded from
performance acceptance. A subsequent build adds bounded camera/intensity min/max
coverage counters at shutdown rather than repeated per-frame file writes.
The generated original `newShadowsBeginFrame` and its visible-object caller still
contain the original live intensity update. No omitted update is established.

No console deployment or physical speedup claim accompanies this local revision.
The New console logs collected in `console-visual/20261011-014222-223` are identical
to `20261011-003756-223`; the installed SD-burst tracing build still lacks a new
hardware run. Missing Arwing/trees, physical stalls, fur and full lighting/shadow
parity remain open.


The final Old ladder check (`height-fog-motion-v2-after-old`) completed in
49.59 host seconds with state, save, log-drain and owned-exit checks passing.
Coverage reports camera Y -999.824..-973.144 while the fog top is -1048.191 and
bottom -1083.191. Intensity therefore correctly remains 0; the camera stays
above the volume. Repeated live intensity changes remain uncovered by this route.
First-use atlas generation/publication spans 1,878,979 emulator ticks (~7.01 ms),
once per owner; it is not hidden as free preparation. It replaces the older
single-view initial conversion of ~0.438 ms. There are no native update fences
or pixel writes after preparation. Initial cost and the extra resident storage
are explicit tradeoffs, not a claimed improvement in this idle-intensity route.

Serial Old ladder baseline/candidate: scene 1.605708/1.601239 ms; native submit
5.310797/5.311515 ms; cadence 58.3083/58.4061 Azahar FPS. Differing animation phase
and workload prevent a speedup claim. The approximately 6.91 ms CPU rendering
cost is unchanged within this short comparison. The failed external-observer
run is excluded. New profile timing is not repeated for unchanged shaders; no
physical-console speed or sampling acceptance is inferred.

Old candidate heap peak 40,846,584/50,331,648 B, minimum sampled free linear
3,833,472/10,485,760 B, free VRAM 4,822,016 B, measured main-stack 73,008/524,288 B,
process commitment 67,571,712/83,886,080 B. The equal sampled linear minimum does
not mean the atlas is free: its explicit owner peak is 65,536 B and startup/load
phases determine the overall minimum. All 65 view descriptors share that storage.

Final baseline/candidate frame-1500 captures both show the ladder, textured room,
Fox and HUD with similar dark cave appearance. The earlier v1 diagnostic capture
has visibly stronger haze despite the same nominal route. Its cause is unresolved
(animation/environment phase is not captured by the current evidence); do not
treat this as pixel parity or dismiss the difference as timing without proof.

Follow-up diagnostic observer: `tools/azahar_native_draw_capture.h` now supports
opt-in start-frame and all-textured capture. Original default filtering remains.
At most64 draws/512 input vertices each; broad capture reserves at most16 draws
per entry so world draws cannot consume every slot before actors. Optional `.fog`
sidecars retain actual procedural tables and bound64x64 A8 height image bytes.
No guest writes, skipped game work, hidden windows or audible output. GS draws
remain outside this observer. No timing claims from either diagnostic emulator.

V1 New/Old runs completed,64 captures each, replay/save/drain/owned-exit passed.
The captures again differ visibly in haze (New little haze, Old more), so this
is not established as an Old/New capability difference. Captured world shader
inputs differ only slightly with camera phase; distance scale matches and fog
constant differs by only one green level. V1 did not retain lookup/image bytes
or sufficient actor draws; it cannot identify the haze cause.

V2 New/Old runs completed (47.64/46.89hostseconds),48 captures each, all state/
save/drain/owned-exit checks pass. Both1410 screenshots inspected and both show
little haze this time; they do NOT reproduce the differing-haze pair. Offline
actual float24 VM outputs are finite for all captures. Broad capture includes
several shaders, so the old ordinary-actor UV oracle is deliberately not applied;
this is state inspection, not a UV-correctness claim. Fourteen GPU height-image
snapshots per run match the actual native C atlas's intensity0 bytes exactly.
All captured procedural tables match byte for byte across the two v2 runs.
`tools/inspect_native_fog_capture.py` and `fog-native-state-test/lookup-comparison.json`
record that proof. This rules out corrupted atlas bytes in those sampled draws,
not all physical sampling/lifetimes or the earlier haze. GS particles and later
passes remain unobserved; no particle or fog cause is established. No additional
unchanged cave runs are warranted without a more discriminating observation.

Emulator builder source/IR/PICA and baseline restored after each build. All owned
builds/runtimes/inspectors terminal, including50610/19796/60882/69343/7779/97637/
11901. Source shader/game archive unchanged from checked height-cache candidate.
No FTP or remote changes for this local revision; New installed5dd6/bd33 and
pendingVRAMreplay, Old builds, saves and settings remain unchanged.


12 October source producer replacement (local V10): the original per-selector
4 KiB CPU rasterization and copy are removed. Native rendering already selects
an exact immutable atlas view and never reads the source image pixels. Original
intensity, camera/scroll/phase state and allocations remain. HEIGHT FOG OFF now
avoids source snapshot reads and clears stale block state. See
EFFECT_TOGGLE_WORK.md and height-fog-work-test/{result,arm-frame}.json for exact
source, ARM and runtime evidence. This does not repair the earlier variable-haze
observation or establish physical-console sampling/performance parity.
