# Native packed GPU lights and normal demand — 14 October 2026

Local candidates, not deployed or physically validated. This changes shader
execution and its small native uniform publisher, not game lighting selection,
geometry, material equations, simulation, or source animation.

## Why this implementation

The Oct12 directional specialization and signed-colour normal-demand candidates
remain rejected. The former penalized point lights (the cave had no directional
lights); the latter increased instruction counts for common lit materials.
Those experiments were reviewed before this implementation, not repeated.

The native actor shader contained eight copies of identical lighting arithmetic,
each behind its own boolean. The replacement packs active light slots in ascending
source order once per lighting snapshot, then iterates them on PICA. Point and
directional lights still execute the same normalization, dot products, MADs,
RGBA accumulation and clamps in the same order. No illumination approximation
or per-vertex CPU work is introduced.

Integer uniform i0 supplies `(count-1,0,1,0)`; a boolean guards zero lights.
The CPU publisher restricts count to 0..8. The assembler's normal boundary NOP
handling remains enabled. The linker now relocates LOOP targets; the contract
checker only admits a straight-line arithmetic body with no output writes,
MOVA, nested control, or escape from an enclosing IF. It does not silently
admit arbitrary shader loops.

The combined candidate reuses freed boolean slot5 for normal demand. Bone/view
normal transforms are skipped only when lighting is disabled or neither an
active light nor environment/specular mapping consumes the normal. Vertex versus
constant material colour remains on its original boolean and decode scale.
The separate cloud shader retains its old slot5 light-mask meaning, explicitly
restored after any ordinary material. World/UI/shadow programs retain their ABI.

## Hardware/SDK adaptation

Consulted pinned Melee `native-3ds/port/3ds/citro3d_fix.c` for preserving SDK
cached-state/dirty-bit ownership, and pinned Citro3D `uniforms.c`, `uniforms.h`
and `types.h`. Integer constants can be installed by program binding; loop
parameters are therefore published after binding and material selection.
An already-correct cached value retains its pending dirty bit. New scene/frame
publication forces the value to be written. There are no raw GPU register writes,
new command queues, fences, workers, allocations, or texture/geometry copies.

Assembler syntax and boundary caveats are documented in the
[Picasso manual](https://github.com/devkitPro/picasso/blob/master/Manual.md).
Interpreter equivalence does not prove physical loop throughput or correctness.

## Evidence

`artifacts/research/pica-scene-rewrite/light-loop-test` contains standalone
shader prototypes, all256 sparse-mask combinations, relocation/contract tests
and actual native owner/SDK publication tests. `light-loop-linked-test` uses
the actual executable's shader binaries. The combined implementation is in
`light-normals-test` and `light-normals-linked-test`.

- Actual native packer: 256,000 snapshots, original ordering/values/other state
  retained; 1,024,000 integer handoffs pass ASan/UBSan. Mutants omitting packing,
  poisoned-state repair or forced publication fail.
- Actual normal-demand publication: 524,288 combinations, including full boolean
  masks and cloud restoration. Missing environment/specular/restore mutants fail.
- Actual linked ordinary/layered/fur entries: 131,072 vertices each, all28 output
  lanes exactly equal to the prefetch parent in float24. Coverage includes zero
  through eight lights, sparse masks, single/blended bones, alpha/material modes,
  unlit/world cases, environment mapping and specular UVs. This is sampled input
  equivalence, not proof over every possible float input.
- Missing zero guard, incorrect loop step, and omitted map normals are rejected.
- All10,240 sampled traces per family have no instruction-count regression
  against the original unrolled shader. This is not a GPU cycle measurement.
- Shared scene bank: **511 → 445** instructions; material bank **304 → 238**;
  fur **216 → 150**. The loop-only intermediate is443/236/148. Static capacity
  savings must not be presented as the same percentage speedup.

The loop-only implementation saves five executed instructions per lit vertex,
or seven with no active lights. The combined path adds two control instructions
to a lit vertex that still needs normals (net three fewer than the parent),
while ambient-only cases also avoid normal transformations. Unlit cases do not
take the new nested branch. Float24 traces are retained for exact sample counts.

## Builds and tests

Use `tools/build_light_normals.py console-replay --revision v1` and
`console-setup` for the archived candidate. They isolate the immutable
`specular-prefetch-v1-console-replay-builds` sources, verify Old80/New124 CIA
profiles and unchanged game archive, then restore unrelated VRAM work in
`finally`. Do not build the ordinary worktree blindly. Candidate sources live
in the build's `source-snapshot.zip`; the old VRAM worktree is not a promoted
rendering snapshot.

The loop-only New palace replay completed with state assertions, original saves,
log drain and owned-process exit intact; image1410 inspected. It is an
intermediate correctness check, not the final candidate's performance evidence.
Final New/Old replay metrics and memory are recorded by
`tools/analyze_light_normals.py` in `light-normals-test/integration.json`. Both completed with replay status1,
state assertions, unchanged normal saves, log drain and owned exit; both1410
images were inspected. The same17 shader binaries and game archive are in
normal and replay variants. External SMG compilation overlapped the final
candidate check, so timing is excluded and redundant paired baseline runs are
deferred. The `--correctness-only` analyzer records that explicitly. Matched
recipes are not identical animation/RNG/audio
instruction streams. Emulator GPU pending spans are not pure physical GPU time.

No remote files, private assets, settings or saves have been changed by this
work. The installed New223 shadow-depth correction remains physically
unconfirmed; previous collected logs predate it. This lighting work does not
establish a fix for rapid whole-shadow flicker or complete visual parity.

## Final local resource samples

| Resource | New profile | Old profile |
|---|---:|---:|
| Heap used / reserved | 48,598,456 / 71,303,168 B | 41,061,816 / 50,331,648 B |
| Linear free / reserved | 18,036,224 / 25,165,824 B | 3,356,160 / 10,485,760 B |
| VRAM free | 4,723,712 B | 4,723,712 B |
| Measured main stack / reserved | 72,972 / 524,288 B | 72,984 / 524,288 B |
| Process commitment / grant | 103,354,368 / 130,023,424 B | 67,702,784 / 83,886,080 B |

These are replay samples, not continuous hardware memory measurements. No new
normal-app startup or ISO conversion test was run for this shader-only change;
their unchanged recipes are checked by build identity. Replay source7a4abce630df,
normal/setup898b0229c65f remain local. Physical cost, dynamic-loop acceptance,
the rapid shadow flicker report and full rendering/progression remain open.

All owned build/test/emulator sessions are terminal: 7485,14801,86641,73775,
95920,69207,12401,17323,84479. No FTP session or remote mutation this turn.
