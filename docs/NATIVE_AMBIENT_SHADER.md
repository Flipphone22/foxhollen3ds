# Exact ambient-only actor specialization, 14 October

A local native shader candidate removes normal transformation and local-light
branches when the current actor has no active local lights and its material
does not consume normals. Game simulation, geometry, skinning, morph positions,
ambient colour, opacity, texture sampling and material effects are preserved.
This is an exact specialization of the current PICA actor path, not an unlit
approximation or a restored GX renderer.

The owner selects it only with lighting enabled, no active light bits, and no
shadow/fur/cloud/UI/layered-program owner. Actual prepared material flags reject
active environment or specular maps and custom movie/moon/UI/cloud coordinates.
Runtime graphics toggles participate in admission. Changing light or material
requirements immediately selects the existing general native shader. No source
material interpretation, CPU vertices, asset copying or new worker is involved.

The new58-instruction program preserves the existing float/boolean ABI and all
seven output registers. It retains GPU bone blending, fixed-point decode,
ambient RGBA clamp, vertex/constant material selection, alpha-lighting behavior,
both texture planes and distance fog. The program is initialized with the scene
owner and freed after the existing GPU retirement boundary. Pinned Melee's
retained-state and texture-cache publication rules were consulted; this candidate
keeps the existing conservative full pipeline invalidation on program changes.

## Evidence

- Actual float24 comparison against the linked combined-build actor program:
  65,536 vertices /1,835,008 output lanes exactly equal, including negative and
  over-one ambient inputs,22 bones, blend endpoints, both UV planes and both
  material/alpha modes. All output-path checks pass.
- Actual interpreter instruction traces for1,024 vertices:79,360 instructions
  before versus50,176 after, a36.8% reduction for these admitted vertices. Ranges
  are69–86 versus44–54. This is executed shader instruction count, not hardware
  cycles or total-frame work. Stored sizes alone are not the speedup argument.
- Actual material selector and live-owner guard under ASan/UBSan pass runtime
  toggles, custom-material exclusions and each local-light bit. Removing local
  light, environment or specular exclusions is rejected by the tests.
- The linked candidate hash`8ac1bc55214e` matches the tested shader. All18 prior
  GPU programs and the game archive are unchanged. No added linear or VRAM
  geometry/texture resource; the shader blob is1,108 executable bytes plus its
  small normal-memory SDK descriptor and owner fields.

The first shader assembly failed because a constant occupied an unsupported
second operand. Restoring the original move-to-temporary clamp sequence fixed
assembly without altering equations. The first instruction-trace harness tried
to assign a read-only emulator BitField; using its input configuration word
fixed the harness. No failing game binary was run for either error.

## Game coverage and limits

New Thorntail with specular and FAST ripples completes85.69 host seconds;
Old palace completes55.38 seconds. Both pass state/save/log-drain/owned-exit
checks. New1500/1740 andOld1410 captures were inspected: world, characters,
textures, particles and HUD remain visible. These images do not prove whole-game
parity or physical-console correctness.

| Whole-run work | New Thorntail | Old palace |
|---|---:|---:|
| Specialized draws / all actor draws |65,217 /101,651|10,263 /38,924|
| Specialized indices / actor indices |21,779,472 /65,159,361|1,881,534 /25,417,764|
| Pipeline state publications |19,949|11,219|
| Specular draws / unready |28,263 /0|22,991 /0|

The previous combined New route had12,779 pipeline publications. The increase
is a real concern even though animation/workload counts differ: selecting more
programs can increase CPU submission and code/state upload costs. Do not claim
a net FPS win from the instruction reduction. Both current timing runs are
excluded due observed foreign compilation; the normal build also overlapped
part of the correctness work. No uncontended or physical cost gate has passed.
This candidate is **not promoted or deployed**. Next reduce or measure ordinary/
ambient handoff costs under the exact shared ABI before accepting it.

| Sparse resource samples | New | Old |
|---|---:|---:|
| Heap used/reserved |49,230,536/71,303,168 B|41,079,912/50,331,648 B|
| Linear free/reserved |16,624,000/25,165,824 B|3,234,176/10,485,760 B|
| VRAM free |4,690,944 B|4,723,712 B|
| Main stack/reserved |73,044/524,288 B|72,984/524,288 B|
| Process commitment/grant |103,419,904/130,023,424 B|67,768,320/83,886,080 B|

Sparse samples are not continuous peak proof. Prior Old Thorntail linear
headroom remains tighter than this palace run.

Shared local replay`c2c0f7150692`/3dsx`bc071f2edefb`/CIA`273247f9ab6d` and
normal setup`56292372736a`/3dsx`13e23acf3280`/CIA`ad3606c9a729` share616 source
entries,19 linked programs and archive`c996af790c5a`. Old80/New124 profiles,
installation recipes and full worktree/archive restoration pass. No fresh normal
startup or ISO conversion is claimed; unchanged evidence is reused. The root
still contains unrelated WIP; use `build_ambient.py` to isolate this candidate.

Reproduce with `test_pica_ambient_shader.py`, `test_pica_ambient_admission.py`,
`build_ambient.py`, `run_ambient.py`, `analyze_ambient.py` and
`verify_water_sheet_builds.py --family ambient --revision v1`. Evidence is in
`ambient-shader-test`, `ambient-admission-test`, `ambient-runtime/integration.json`
and `water-sheet-runtime/ambient-build-verification.json`.

No FTP, console installations, private assets, saves or preferences changed.
Hardware shadow flicker, full visual parity, sustained30/60FPS and final normal
progression acceptance remain open. The full goal remains active/incomplete.

## Retained handoff v2

The ordinary and ambient programs now retain their common fragment/raster state
when switching directly between them. Every external invalidation and every other
shader family still takes the complete publication path. SDK program binding is
retained, including its code, attribute, constant and dirty-bit handling. Startup
checks reject differing constants, defaults or geometry stages; constants cannot
overlap live camera, bone, lighting or material registers. This follows the pinned
Melee rule that cached ownership must preserve pending SDK publications.

The actual extracted scene selectors and SDK BindProgram/LoadShaderUniforms pass
131,072 handoffs with all boolean patterns under ASan/UBSan. Five ABI guard cases
and mutants omitting SDK binding, retaining external state, or admitting unrelated
owners are rejected. The generated SDK BindProgram body matches the tested source.
Raster calls are stubs in this test; real game runs provide separate integration
evidence. All 19 shader binaries and the game archive remain unchanged.

New Thorntail and Old palace complete, with state/save/log-drain/owned-exit checks.
New captures1500/1740 and Old1410 were inspected: characters, terrain, textures,
particles, HUD and the New water route remain visible. New executes6,925 retained
handoffs and12,822 full publications; Old2,857 and8,307. These are avoided resets
in the candidate, not a fixed-workload FPS measurement. Both timing comparisons
are excluded by foreign compilation. There is no physical acceptance.

New sampled heap49,230,536/71,303,168 B, linear free16,626,048/25,165,824 B,
VRAM free4,690,944 B; Old41,079,912/50,331,648 B, linear3,234,176/10,485,760 B,
VRAM4,723,712 B. Measured stacks73,044/72,984 of524,288 B. Process commitments
103,419,904/130,023,424 B and67,768,320/83,886,080 B. These are sparse samples.

Evidence: `ambient-handoff-test/result.json`, `ambient-v2-runtime/integration.json`,
and `ambient-v2-console-replay-builds/pair-verification.json`. Isolated builders
are `build_ambient_handoff.py`; replay runner accepts `--revision v2`.
The v2 candidate is local only. Code upload on switching remains a cost; the next
candidate tests shared ordinary/ambient code residency without changing equations.

## Shared code bank v3

The native general entry (192 instructions) and ambient entry (58) now share a
250-instruction DVLP. On a direct switch back from ambient, the general entry
stays in that bank. Full external resets still select the original general/HUD/
particle bank, preserving that existing sharing relationship. This follows the
pinned Melee shared-DVLP approach. It neither removes the original general
lighting work nor reconnects a retired renderer.

The shader linker verifies every relocated operation, branch and descriptor;
constant/output/uniform entry tables remain unchanged. Azahar's actual float24
interpreter compares65,536 general world/actor vertices and65,536 ambient vertices
against the previous linked program. All output lanes match exactly, including
general local/environment/specular lighting branches. Both paths have complete
output writes and bounded control flow. Actual extracted SDK/owner testing covers
131,072 switches with no VSH/GSH code-upload flags within the bank. The first
transition from the old general bank still requests the required upload.

The new blob is2,692 B versus1,108 B for the ambient-only blob, plus one SDK shader
instance. It uses no added linear/VRAM resource, worker or per-frame allocation.
Only the ambient blob changes; the other18 programs and game archive are retained.
This removes repeated code uploads within the admitted pair; it is not a measured
total-frame speedup or proof of physical performance.

Tools: `test_pica_ambient_bank.py`, `test_pica_ambient_bank_handoff.py`,
`pica_ambient_bank_patch.py`, `build_ambient_bank.py`. Exact shader hash begins
`f97558c9ea14`. Runtime acceptance is recorded separately below.

V3 New Thorntail and Old palace both pass state/save/log-drain/owned-exit checks.
New1500/1740 and Old1410 captures were inspected: world, characters, textures,
particles and HUD remain; New water rings remain. New7,365 retained switches /
12,650 full publications; Old2,903 /8,424. Retained-switch counts include entry
into the bank from the old bank, so they are NOT counts of avoided code uploads.
No new physical rendering acceptance is implied.

Sparse memory: New heap49,230,792/71,303,168 B, linear free16,632,192/25,165,824 B,
VRAM free4,690,944 B; Old heap41,080,168/50,331,648 B, linear3,234,176/10,485,760 B,
VRAM4,723,712 B. Measured stacks73,044/72,984 of524,288 B. Process commitments
103,419,904/130,023,424 B and67,768,320/83,886,080 B. Normal-memory samples grew
256 B from v2; process commitments are unchanged. Old Thorntail's previously
smaller linear headroom remains relevant; palace does not supersede that evidence.

Neither v3 boundary snapshot saw a compiler. One fresh v2 New control used the
same save/assets/script/options/clock, with compiler monitoring each second.
It DID overlap foreign compilation:92 observations, timing comparison invalid.
Raw v2/v3 emulator values were55.340/56.185 submitted FPS,18.070/17.798 ms/frame,
scene2.0800/2.0790 ms and submission3.6538/3.6124 ms. Draws/frame180.144/180.069,
indices/frame77,861/78,218; animated workloads differ. These numbers therefore
DO NOT establish a speedup, a clean cost gate, or physical-console FPS. Do not
repeat unchanged contended runs simply to obtain a favorable number.

V3 evidence: `ambient-bank-test`, `ambient-bank-handoff-test`,
`ambient-v3-runtime/{integration,cost-comparison}.json`; the two game directories
are `artifacts/native/ambient-v3-fast-on-new` and `ambient-v3-on-old`.
`run_ambient_cost_control.py` records the rejected timing control. This candidate
remains local and unpromoted pending a useful net-cost/hardware comparison.

Final shared normal source`bb94b5b34f6d`/3dsx`73b0cb1df5a0`/CIA`ed7996ca2c97`
and replay`a62474102735`/3dsx`6791473474eb`/CIA`1d55a81fc8a9` are built locally.
Pair verification confirms616 identical source entries,19 identical linked GPU
programs, unchanged game archive, Old80/New124 profiles, installation recipes and
complete worktree/archive restoration. No fresh normal startup or conversion was
rerun. No deployment or changes to private assets, saves or settings occurred.
