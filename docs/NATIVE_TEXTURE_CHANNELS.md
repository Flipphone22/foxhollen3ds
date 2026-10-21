# Native texture-channel selection

11 October: local implementation; not deployed or physically accepted. This
restores a previously ignored source material parameter, `swapSelector`.
It does not establish a fix for the reported white Arwing, disappearing trees,
day/night lighting, fur, shadows or SD pauses.

The original `addTexLayerStageSwizzled` selects from `gRcpTevSwapTable[24]`.
Tricky, scarabs, magic gems, the magic-cave effect and several other objects
change this parameter. Alpha is not part of the RGB permutation. The native
adapter now reads the actual rendered `ObjModel`'s texture-reference entry;
simulation, random selection and source object updates remain unchanged.

Ordinary independent single-texture materials reconstruct the selected RGB
channels using three disjoint constant masks, then apply their existing base
modulation in the fourth PICA combiner stage. All 24 selections share one
compiled program. Selection zero retains the original native program.

Environment-lit selections 6–23 use only two input channels. They retain the
existing clamped C1 lighting sum in stage0's RGB buffer, reconstruct texture
colour in stages1–2, and modulate by C1 in stage3. Three shared programs cover
the possible channel pairs. The actual SDK buffer-update mask is retained.
Five nonidentity three-channel permutations with environment lighting remain
unsupported. Separate opacity masks are now supported by the
[opacity follow-up](NATIVE_TEXTURE_OPACITY_CHANNELS.md), with explicit validation
limits. Multiple layers, auxiliary textures and active C2 compositions remain
outside this implementation. Their existing native
paths remain selected, and nonzero unsupported draws are counted.

The follow-up now composes these programs with optional LIGHT MAPS. At model
load, it prepares descriptors for the existing map=0/base=1 sampler layout.
Environment C1 reads the map; texture reconstruction reads the base image.
Alpha and the C1 buffer update remain unchanged. Ordinary and map-lit variants
share the existing program pool and retain the existing sampler/UV owners.
Active specular materials still take precedence and remain outside this repair.

Stages4–5 remain available for colour fade and distance fog. Height fog's current
world-only eligibility cannot overwrite these actor stages. Existing native
environment lighting accuracy is unchanged; its approximation is not redefined
as original-game parity by this channel repair.

There are no new images, geometry buffers, draw passes, shaders or workers.
Programs are compiled and reference-counted at model load. Each material adds
40 bytes of CPU metadata (24 initially, plus16 for map variants); the immutable
channel masks occupy288 bytes. Programs
use the existing bounded128-entry/11264-byte pool. Live CPU work reads the source
selector once per material, chooses a prepared program at bind, and updates
changed constant masks. This belongs on the CPU because it is original live
material state; vertices and texture pixels are not rebuilt.

Pinned Melee's write-site renderer-state invalidation and the pinned citro3d
texture-environment descriptors were consulted. The existing SDK submission,
owner retirement and shader/material handoffs are reused. No direct register
writes or new GPU resource lifetimes were introduced.

Validation now includes786432 ordinary RGBA cases against the original channel
table,589824 environment cases including C1 saturation and alpha independence,
and100000 actual owner/binder transitions across100 load/retire cycles. Tests
execute the pinned SDK descriptor setters and actual native owner functions.
Missing constant publication, incorrect channel selection and loss of the C1
buffer are rejected. These are ASan/UBSan host checks, not GPU timing proof.

The first three build iterations passed cave shutdown/state/save checks but
executed zero repaired draws. That is failed feature coverage, not acceptance.
A bounded read-only New emulator observation identified model1231 with selector22
and no prepared swizzle program. Its unlit-material compiler legitimately writes
unused input slots in REPLACE stages. The new compiler had incorrectly required
those unused fields to be zero. Version4 checks only consumed source/operand
fields, and the regression now executes the real unlit-material function.

The first Old diagnostic observer rejected its48MiB heap and exposed an unrelated
undefined-name error in the rejection message. Its supported heap profiles now
match `fh_heap_profile.h` and `os_memory.c`; original heap/header/canary/ownership
proofs remain required. A subsequent observation was rejected because it tried
to verify all unrelated live materials simultaneously. The diagnostic now
verifies one affected owner and its complete selected material/frame records.
Neither failed diagnostic nor the successful observed run is timing evidence.

Final runtime/build results are recorded in
`artifacts/research/pica-scene-rewrite/texture-swizzle-test/`, with separate
`texture-swizzle-owner-test` and `texture-swizzle-environment-test` reports.
Further visual, hardware, masked/layered and full lighting-family acceptance
remains required.

Final v4 serial cave runs pass on New and Old emulated profiles: state checks,
save preservation, shutdown-log drain and owned-process exit. New records2145
repaired draws and Old2058, with zero nonzero selections falling back in this scene. Load
admission is87 declarations, with10 partial declarations; this inventory includes
menu resources and does not mean every admitted family was drawn. Specifically,
the environment-composed programs have host coverage but no separately isolated
live GPU visual acceptance. Both1410 captures were inspected: world, Fox, HUD,
cave lighting and the Old software stick remain present. They are regression
images, not original-versus-port proof of the recoloured effect's appearance.

| Cave measurement | Prior New | v4 New | v4 Old |
| --- | ---: | ---: | ---: |
| Azahar FPS |59.8312|59.8312|57.5109|
| Scene CPU, ms |1.816845|1.819972|1.817238|
| Submission CPU, ms |5.340837|5.411768|5.433044|

New combined rendering CPU rises0.074058ms, below the0.20ms investigation budget.
Animation/workload phases differ, so this is a short regression comparison.
Old has no matched pre-feature OFF baseline; v3-to-v4 combined cost rises0.011712ms,
which only measures the admission correction. No physical FPS or GPU-cost claim.
Program peak rises34 to38 entries in the unchanged128-entry pool.

New heap48,389,256/71,303,168 bytes; Old40,852,616/50,331,648. Relative to the
pre-feature New run, heap peak increases6,056 bytes. Linear minimum free is
18,513,536/25,165,824 New and3,833,472/10,485,760 Old. Both retain4,822,016 free
VRAM and measured73,008/524,288 stack. Process peaks are103,227,392/130,023,424
New and67,575,808/83,886,080 Old. No new linear/VRAM allocation is introduced.

Shared build proof: replay `960273bc8d06` / CIA `bd4ad7229f9e`, normal
`b73739b78bc8` / CIA `73ad9ce91414`. All344native sources and the game archive
match. Main511-, fur214- and material304-instruction programs are byte-identical
to the previously checked shaders; shared CIA profiles remainOld80/New124.
Normal UI/runtime behavior is unchanged and its prior evidence is reused; no
new normal-app boot is claimed. Builds, saves and graphics preferences on both
consoles remain unchanged; the pending VRAM replay has not been replaced.

## LIGHT MAPS follow-up

`texture-channels-map-v1` fixes the previous bypass when optional LIGHT MAPS
selected its native material. The host tests above now cover both sampler
layouts, ordinary/environment lighting, and100000 actual binding transitions
including mapped/unmapped changes. Missing C1-buffer and constant-publication
mutants remain rejected. The CPU metadata increase is16bytes/material and two
telemetry counters; the existing program pool is unchanged.

Serial cave runs use the same assets, input recipe and LIGHT MAPS ON setting.
The New comparison uses the archived prior v4 executable. No build or emulator
ran concurrently with the measured windows. Saves, state assertions, log drain
and owned-process exit pass. New and Old1410 images were inspected: world,
Fox and HUD remain present, as does the Old touch stick. These images do not
isolate every recoloured effect or establish original-image parity.

| Measurement | Prior New ON | New candidate ON | Old candidate ON |
| --- | ---: | ---: | ---: |
| Azahar FPS |59.831227|59.831223|56.234903|
| Scene CPU ms |1.910654|1.913100|1.895908|
| Submission CPU ms |5.719184|5.727784|5.752939|

New combined rendering CPU increases0.011046ms, below the0.20ms investigation
gate. Animation/workload phases differ; this is a short regression comparison,
not a console-performance estimate. Old has no paired prior ON measurement.
The cost of LIGHT MAPS as a whole is separate and remains above its previously
recorded budget; its default remains OFF.

New heap48,393,272/71,303,168bytes; Old40,856,632/50,331,648. The New increase
is4,016bytes. Linear minimum free remains18,513,536/25,165,824 New and
3,833,472/10,485,760 Old; VRAM free4,822,016 and measured stack73,008/524,288
on both. Process peaks103,231,488/130,023,424 New and67,579,904/83,886,080 Old.
There is no extra linear/VRAM allocation. Evidence is in
`texture-channels-map-test/runtime-comparison.json`.

The initial runner storage preflight correctly rejected insufficient free disk
space before launching. Archived ELF and game-library files were losslessly
NTFS-compressed in place with unchanged SHA256 checks; no evidence, private
assets or saves were deleted. Compression records are in the same test folder.

Shared follow-up build proof: normal `b7edeed89c4c` / CIA
`4b3d3af928ca`, replay `8d9a6fb8396f` / CIA `44e86b268a93`.
All344 native sources and the game archive match. The511/214/304-instruction
programs are unchanged; CIA memory profiles remainOld80/New124. Normal UI
evidence is reused because it is unchanged. No new normal boot or physical
acceptance is claimed. Both installations, saves and settings remain unchanged.
Mapped draws: New2145, Old2013; no unsupported selections in these runs.
Environment-composed draws remain zero, so that family still has host-only
specific validation. The existing program pool peaks at43/128 entries.
