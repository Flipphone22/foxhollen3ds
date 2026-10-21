11 October New223 update COMPLETE: normal `9eec94e236e3` / CIA
`208ed7ac4914`, replay `97a579346965` / CIA `e0949e10fa82`. All eight changed
app/identity files staged and byte-readback verified, renamed and final sizes
checked. Ten unchanged original asset/cache files reuse prior completed byte
proof plus current descriptors/topology/sizes. Saves and preferences preserved;
Old229 unchanged. Matching3dsx ready; CIA requires FBI installation at
`SD:/3ds/foxhollow/foxhollow.cia` (replay sibling folder/app name).

Normal rendering telemetry now summarizes every frame at about1Hz, preserving
timestamped >100ms stalls and final totals; replay remains per-frame. Host
serializer test reduces this log's bytes27x, not a claim that total SD traffic
or physical stalls improve by27x. Shared270-instruction material bank avoids
shader-code uploads on eligible entry switches. Optional native diffuse maps,
masks/layers and compact shadow preparation are accumulated local work now in
these apps; LIGHT MAPS and shadows stay OFF by default. Added light-map CPU
cost still exceeds its provisional gate, so no full-feature performance
acceptance. No established physical speedup or fix for disappearing trees,
white Arwing, full fur/day-night/shadow fidelity or all filesystem stalls.

Normal startup/toggle/persistence/Select and palace replay checks pass; captures
inspected. Latest read-only console collection222419 is unchanged from211130,
not new hardware evidence. Detailed scope and validation:
[render logging](NATIVE_FRAME_METRICS.md),
[material shader ownership](NATIVE_MATERIAL_SHADER_BANK.md).
Deployment evidence: `artifacts/console-settings-update/20261010-223515-223/manifest.json`.

11 October New223 deployment COMPLETE: normal `f50343cd2e56` / CIA
`82d3285f6677`, replay `e8eefd5944cf` / CIA `72ee1193db8b`. Layered
secondary-light materials and reduced native memory-log traffic are installed.
Eight changed application/identity files received full staged byte readback,
rename and final-size verification. Ten unchanged original asset/cache files
reuse prior completed proof plus current descriptors/topology/sizes; they were
not bulk-reread. Saves/preferences preserved. Old229 unchanged. Matching3dsx
ready; CIA requires FBI installation. SPECULAR remains opt-in; existing saved
options are preserved. No physical stall, speed or whole-parity claim.
Evidence: `artifacts/console-settings-update/20261010-202815-223/manifest.json`.

11 October New223 deployment COMPLETE: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`. New texture
page caching plus the accumulated environment-normal/unlit-material corrections
are installed. Eight changed application/identity files received full staged
byte readback, rename and final-size checks. Ten original asset/cache files
reuse prior completed byte proof plus current descriptor/topology bytes and
sizes; large asset bytes were not freshly reread. Saves/preferences preserved.
Old229 is unchanged. Matching3dsx ready; CIA requires FBI installation.
Physical stalls, fur and reported visibility/lighting acceptance remain open.
Evidence: `artifacts/console-settings-update/20261010-190758-223/manifest.json`.

# GPU fur and lossless actor batching

The candidate normal build is `626d217324e9` (CIA `762ef1732702`); replay is
`9bcad101a4fe` (CIA `9acc42b6e738`). Both select Old/New capabilities at runtime
and retain the Old 80 MiB / New 124 MiB CIA profiles. Check the deployment record
for installation status; local validation alone does not mean a console changed.

Changes since New normal `70038555df22`:

- FUR can use the source game's admitted layer count: enable SOURCE FUR on
  graphics page 10 as well as FUR on page 9. With SOURCE FUR off, the reduced
  two-shell mode remains. Eligibility, distance and animation are unchanged.
- A separate 214-instruction PICA vertex program expands supported fur meshes
  from persistent base bone palettes. Load-time pivot/growth tables replace
  repeated CPU shell matrix calculations. Other fur models retain the existing
  native rendering; they are not dropped. No CPU vertices or new worker.
- The original-geometry pack uses larger compatible bone palettes to reduce
  draw groups. All ordered corners, global bone weights, UVs, colors, morph
  references and triangles are preserved. This is not more decimation.
- The updated original geometry has a matching lossless New 3DS RAM-cache file.
  It uses the existing 7 MiB reservation. Old 3DS does not allocate that cache.

The generic performance-geometry pack is not changed by the original-asset
installer. Its separate 22-slot candidate remains local. Your original/perf
selection, other preferences and saves are preserved during installation.

## Validation and limits

The source-density cockpit comparison reduces combined rendering CPU time by
0.277 ms; its full fur cost is 0.330 ms versus the same binary with fur off.
Shader, palette, lifetime and program-handoff tests are recorded in
[GPU fur evidence](NATIVE_FUR_GPU_EXPANSION.md). These are local measurements.

A broader original-geometry/original-texture Arwing approach enables the user's
effects plus source fur density (isolated recipe mask 8060929). Serial New/Old
headless runs complete normal input/state/save checks and drain before exit.
The 1650 captures show Fox, textured Arwing panels/blue wings, trees and HUD.

| Azahar profile | FPS | Scene CPU ms | Submission CPU ms | Normal heap used/reserved | Linear free/reserved |
|---|---:|---:|---:|---:|---:|
| New |54.31|2.001|3.743|56,156,328 / 71,303,168 B|16,660,224 / 25,165,824 B|
| Old |47.59|1.974|3.719|41,475,704 / 50,331,648 B|1,980,160 / 10,485,760 B|

Both retain 4,846,592 B free VRAM and measured main-stack usage of 72,860 B
within 524,288 B. Process commitments are 103,198,720 / 130,023,424 B New and
67,547,136 / 83,886,080 B Old. Fur parameter peak is 1,496 B. Old linear RAM
is tight; these checks do not establish worst-case residency across the game.

New executes 5,295 source shells, Old 5,492; neither rejects fur draws. Their
animation/workload phases differ. These are not matching hardware benchmarks,
nor speedup comparisons against previous tests with different effects enabled.
New geometry-cache startup takes about 9.43 emulated seconds; subsequent scene
SD reads on this route are zero. Other files can still perform SD I/O.

Fur still lacks source texture-driven wind/displacement and some projected or
layered material families. Its current appearance is not accepted as parity.
The reported white Arwing, disappearing trees, day/night lighting/shadow errors
and physical filesystem pauses remain open; these captures did not reproduce
the visibility defect. Hardware shader transitions, GPU cost and sustained FPS
also remain unvalidated for this candidate.

Evidence: `artifacts/research/pica-scene-rewrite/fur-gpu-original-test/`,
`fur-gpu-runtime-test/`, `original-palette22-v1/`, and
`artifacts/native/fur-gpu-original-arwing-{new,old}/`.

## Console check

The normal Homebrew Launcher path is `SD:/3ds/foxhollow/foxhollow.3dsx`.
The HOME Menu app updates only after installing
`SD:/3ds/foxhollow/foxhollow.cia` through FBI. Replay uses the corresponding
files under `SD:/3ds/foxhollow-replay/`.

For the unresolved hardware question, ordinary cockpit-to-ThornTail gameplay
with FUR enabled exercises the new shader handoff. SOURCE FUR is optional and
increases geometry workload; it is not forced on by deployment. Check for new
corruption/freezes and inspect the Arwing/trees. Select exits and preserves the
diagnostics. No reboot is needed for a normal launch.

## Deployment

New223 completed: `artifacts/console-settings-update/20261010-181211-223/manifest.json`.
All18 payloads verified; saves and graphics settings preserved. Old229 unchanged.
