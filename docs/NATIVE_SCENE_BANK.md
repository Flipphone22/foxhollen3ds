# Native shared scene shader bank

Local work, not deployed. Full visual parity, physical shadow stability and
sustained console performance remain open. This replaces program-storage and
submission machinery; game state, assets and shader equations are preserved.

The preserved baseline is `ambient-v3-*` (250-instruction general/ambient bank,
separate UI/cloud programs). Pinned Melee shares DVLP storage across entries;
the same SDK ownership mechanism is used here. UI/cloud still reset their
fragment/raster state. Their sharing is code residency, not an assertion that
their material state is interchangeable. Existing GS-to-VSH boundaries and
uniform restoration are retained.

## Measure actual publications

`pica_program_observer_patch.py` counts program configuration at SDK
`C3Di_UpdateContext`, immediately around the actual `shaderProgramConfigure`.
It records calls, emitted command words, VSH/GSH upload-request flags and invalid
buffer spans. It never parses or filters GPU commands, changes dirty flags, or
writes per-frame files. Storage is40 bytes of normal-memory counters plus two
temporary values; the existing shutdown log publishes one summary. CPU overhead
is not claimed to be zero. GSH/VSH flags are requests, not GPU cycles or utilization.
Word counts include code and program configuration; they exclude later uniform,
texture, geometry and draw commands.

The actual observer passes2,044 synthetic SDK emission/flag cases under
ASan/UBSan; buffer replacement is marked invalid and two bad-accounting mutants
are rejected. Actual game runs below exercise real SDK emissions. Control and
candidate both contain the same telemetry.

## First four-entry candidate: v1

General192 + ambient58 + UI43 + cloud183 =476 instructions. Native equations
and separate entries remain intact; two separate binary owners are removed.
Every relocated instruction/descriptor/entry table is checked, and65,536 vertices
per entry match all28 output lanes in the actual Azahar float24 interpreter.
SDK handoff tests preserve full UI/cloud state resets while avoiding redundant
code-upload flags within the bank. All borrowed shader instances are released
before their shared DVLB; parent queue retirement and unbinding remain unchanged.

The New Thorntail route passes state/save/log-drain/owned-exit checks. Capture1740
was inspected: Fox, terrain, trees, textures, HUD and water remain. V1 is not
accepted as a speedup: fewer upload requests came with a larger reload payload.

| Whole-run counter | Preserved control | Four-entry v1 |
|---|---:|---:|
| Program configurations |29,971|28,894|
| Emitted program words |10,483,078|10,822,198|
| VSH upload requests |20,830|18,105|
| Invalid spans |0|0|
| Program words / scene draw |53.9803|55.9619|

Source animation/workload differs:102,019/101,362 actor draws,92,183/92,023 world
draws and9,172/9,480 ripple draws. The control timing is excluded due compilation.
No net FPS gain is established. This increased traffic motivated a smaller bank;
no normal v1 package or extra hardware test was requested.

## Common-helper candidate: v2

The same four paths now share two counted helpers: actor skinning/lighting and
UI/ambient skinning. Calls retain all original shader work. An extra NOP separates
IF and CALL retirement, following the existing conservative native flow contract.
Factoring gives484 instructions before duplicate-helper removal and305 afterward.
There are207 instruction slots free, maximum call depth1 and IF depth4. Each
executed helper adds one CALL plus one NOP; this is not a free vertex-cycle win.

`pica_scene_bank_shared.py` requires identical source bodies, normalizes relocated
branch targets and redirects only complete duplicate CALLs. For MOV fingerprints,
only destination/source1 descriptor bits are relevant; Picasso produced different
unused source2 selector bits for one otherwise identical MOV. The first strict
fingerprint attempt rejected that difference. Narrowly ignoring unused MOV source2
bits permits sharing; other instructions retain exact descriptor identity.

The linked shader hash starts`0dbfd86ec8a8`. All262,144 tested vertices and7,340,032
output lanes match the four original programs exactly. The vertex contract checks
complete outputs, bounded CALL/IF stacks, one flow retirement per instruction,
MOVA separation and no loops or geometry emissions. Empty/recursive calls and an
omitted skinning call are rejected. Tests are not physical GPU acceptance.

Tools: `test_pica_scene_bank.py`, `test_pica_scene_bank_shared.py`,
`test_pica_scene_bank_mutants.py`, `test_pica_scene_bank_handoff.py`,
`test_pica_program_observer.py`, `build_scene_bank.py`,
`build_scene_bank_shared.py`, `run_scene_bank.py`, `analyze_scene_bank.py`.
Evidence lives in`artifacts/research/pica-scene-rewrite/scene-bank-*` and
`program-observer-test`. V2 game, UI, memory and command-traffic results follow
separately; compilation or shader equivalence alone does not pass those gates.

## V2 game checks and limits

The same candidate3dsx `08a34c64bda1` (source `a25121517e0e`) completed
New Thorntail, Old palace and New item-menu routes in73.55/48.03/49.55 host
seconds. All state assertions, save preservation, log drainage and owned exits
passed. Inspected New1740 and Old1410 show world textures, actors and HUD;
New UI1410 shows the item strip/background with3,156 actual UI draws. These
checks do not establish complete game progression or original-image parity.

V2 New emitted9,885,542 program words over191,628 scene draws, or51.5871 words
per draw versus53.9803 in the control (4.43% lower). Invalid spans and unready
specular samplers were zero. Workloads differ; this normalization is not an
instruction-identical comparison. Native submission averages3.6687ms versus
3.6961ms control, but the control timing is excluded for host compilation.
**No net FPS improvement is established.** The added counted CALLs also require
physical GPU validation; they may offset some command-traffic savings.

| Sampled resource, bytes | New Thorntail | Old palace |
|---|---:|---:|
| Normal heap used / reserved |49,230,472 /71,303,168|41,079,824 /50,331,648|
| Linear free / reserved |16,631,168 /25,165,824|3,234,176 /10,485,760|
| VRAM free |4,690,944|4,723,712|
| Measured main stack / reserved |73,044 /524,288|72,984 /524,288|
| Process committed / limit |103,419,904 /130,023,424|67,768,320 /83,886,080|

These are sparse measurements; prior Old Thorntail's lower linear headroom
still applies. Native log `instructions` labels retain the pre-sharing source
program lengths, not executed instruction counts. The linked bank report is
authoritative for305 stored instructions. Reports: `scene-bank-runtime/routes-v2.json`
and `comparison-v2.json`. All paths retain the same game archive. No console
deployment, shadow-flicker acceptance, or performance promotion follows.

The next performance work should address the larger remaining CPU costs:
New route scene preparation2.096ms, object rendering2.888ms and native
submission3.669ms. These are emulator phase measurements, not hardware forecasts.
Further program packing alone cannot establish the requested frame targets.

Matching normal setup `96435b5e1914` (3dsx`c2f1634d3e9e`, CIA`ea4889d2d2be`)
and replay `a25121517e0e` (3dsx`08a34c64bda1`, CIA`babf008704e1`) are archived,
not deployed or promoted. All618 renderer/build sources,17 GPU programs and
game archive match. Old80/New124 profiles, original setup recipe and worktree
restoration pass. No fresh normal boot or conversion is claimed; existing
unchanged setup evidence is reused. All owned build/emulator/FTP processes ended.
