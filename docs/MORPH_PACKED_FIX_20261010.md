# Physical morph layout failure and packed-stream repair

The saved New 3DS probe run in `artifacts/console-morph-results/20261010-145208`
completed all 36 comparisons. There is no newer ARM11 dump than the already
diagnosed probe v2 null-texture fault. Probe v3's CPU crash repair is confirmed
for this run.

The current game layout, an eight-byte position stream plus a 40-byte resident
stream beginning with an eight-byte padding component, mismatched the reference
in **12 of 12 cases** (6,870–10,036 pixels each). The separate, tightly packed
32-byte attribute stream matched in **12 of 12 cases**, ordinary and fur modes.
The six saved captures confirm exact reference/packed equality and visible
spurious gradients in the padded layout. Azahar reported equality for all three
layouts; its result did not predict physical attribute fetching.

This identifies a concrete defect in the production layout. It does not yet
establish that every gameplay deformation, eye, lighting or fur defect shares
that cause. No physical FPS measurement is provided by this diagnostic.

## Repair

`fh_pica_morph_pack_attributes` copies the unchanged 32 bytes after each position
once at model admission. The owner retains this immutable lane beside its three
position slots. Per-frame work still updates only affected eight-byte positions;
there are no new draws, shader instructions, workers, live mesh rebuilds or
simulation changes. Actors, fur, planar shadows and holograms use the same binder.

The existing 1 MiB total morph linear allocation limit now includes the static
lane, with the original normal/linear headroom gates retained. Allocation size is
seven position-lane units: three mutable positions and four immutable attributes.
Publication and retirement cover the complete allocation. Storage can be shared
between instances in a later change if measured residency requires it; it is
currently charged per instance and never hidden from the budget.

Pinned Melee `renderer.c:matrix_vertex_pointer` uses a different padded layout.
Its general technique was insufficient evidence for this particular loader.
This repair departs from that padding pattern based on the console comparison,
while retaining its explicit publication and retirement discipline.

## Checks

- Actual SDK binding: 10,000 address/stride/config cases; restoring the rejected
  padding fails the regression. Physical log and capture identities are gated.
- Actual owner: 3,169 updates, 192 retirements plus shutdown, delayed position
  **and attribute** readers, byte-exact static attributes after every update,
  and zero update allocations. Missing packing or retirement mutants fail.
- Serial original-model/original-texture cockpit replays with morphs, fur and
  specular enabled pass on New/Old Azahar: state checks, preserved saves, drained
  logs and owned exit. Captures at retrace 900 inspected on both.
- New before/after scene CPU: 0.769581/0.773008 ms; native submission:
  1.938074/1.935936 ms. Combined change +0.001288 ms, no meaningful regression
  established. Cadence remains 59.831 FPS. Old candidate 59.846 emulator FPS
  is **not** an estimate of Old console performance.
- Added resident linear memory in this scene: 124,416 bytes (121.5 KiB).
  New minimum free 18,500,864/25,165,824 bytes; Old 3,820,800/10,485,760.
  Heap peaks unchanged: 47,854,184/71,303,168 New, 40,514,152/50,331,648 Old.
  VRAM free 4,846,592; measured stack 72,844/524,288 bytes.

Build/runtime records: `morph-packed-v1-builds`, `morph-packed-test` and
`artifacts/native/morph-packed-v1-after-{new,old}`. The historical probe is
archived under `morph-probe-v3-physical` and its local diagnostic header is frozen
to preserve the comparison. The earlier padding-only test is archived and its
entry point now runs the physical packed-layout regression.

Fur coverage-mask work is shelved separately under `fur-mask-local-v1/applied`;
it is not part of this repair or build. Hardware gameplay confirmation remains
required, and fur coverage/depth/lighting parity remains incomplete.

## Deployment

New223 updated and readback-verified in `20261010-150058-223`: normal
`913f01dfaa2a` / CIA `0e00f0405428`, replay `d5f2e028e258` / CIA `4ed770e85176`.
Saves and graphics preferences preserved. Matching normal and replay 3DSX files
are ready; CIA files require FBI installation. Old229 remains unchanged.
Normal startup/Select check passed in 25.36 host seconds, 721 retraces; capture540
inspected. Exact 511-instruction shader, game archive and66 renderer sources
match the replay. Both CIA memory profiles remain Old80/New124 MiB.

Next hardware check is ordinary gameplay with MORPHS ON, initially FUR OFF to
isolate morph deformation, then FUR ON to identify any separate shell artifacts.
No further synthetic probe rerun is needed for this layout decision.
