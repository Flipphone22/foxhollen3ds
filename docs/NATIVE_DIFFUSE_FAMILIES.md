# Native light-map masks and layers

11 October, local implementation; not deployed or physically validated.

The source mode-0 light-map consumer now accepts co-registered opacity masks
and constant-opacity two-layer materials. It preserves the existing source
C1 clamp, intensity-channel selections, layer operation and lighting order.
Masks replace base alpha in the original constant-opacity family; other
families retain their source multiplication. Every animated mask frame must
have the same UV scale as its corresponding base frame. The game adapter
already publishes the same frame and scroll for this mask family. Independent
mask coordinates remain unsupported rather than being silently treated as equal.

The four TEV stages are compiled at load time. Texture0 holds the diffuse map,
texture1 the base, texture2 the mask or second layer. Co-registered masks use
the SDK-owned texture2 coordinate selector; layered materials use independent
UV2 and the existing fragment-light fog path. No extra geometry pass, CPU
vertices, per-draw allocation, texture copy or shader instructions were added.
Fade and fog retain their existing stage owners. Simultaneous C2/specular still
uses analytic C1 lighting and is not full source-map parity. Fur is unchanged.

The first candidate exceeded its provisional New CPU gate: paired palace
scene+submission added .233227ms against a .20ms budget. Its replay was
`3df8f8562c12`, with 7,603 layered map draws and zero unready maps. It was not
deployed or accepted as within budget. The original report remains in
`diffuse-families-runtime-test` alongside the corrected candidate's evidence.

The second candidate selects the same three-coordinate shader across compatible
single-layer, masked and layered draws. Selection requires constant opacity and
unit RGB tint, so ignoring the shader's c7 tint multiplication is valid; c7 is
reserved for the second layer's coordinates. Materials outside that contract
use their existing native path. Full-map single-layer C2 retains its own shader
selection. This avoids repeatedly republishing the pipeline and bone palette
between compatible draws. Its first check instead increased pipeline uploads
from7,877 to10,826: an unconditional actor-entry reset discarded the selection
even between compatible actors. The third candidate removes that redundant
ordinary-actor reset. Shadow's compact early draw and fur still explicitly
reset; every ordinary material still selects before its draw, and external
owner invalidation still forces publication. The actual entry/selector test
passes32 mixed owner sequences and3,200 retained entries, rejecting both the
old unconditional-reset and missing-special-reset mutations. No shader or
geometry change was introduced. V2 was not accepted as a performance repair.

Pinned Melee `renderer.c` and `citro3d_fix.c` were consulted for persistent
bindings and cache ownership. This implementation retains our checked SDK
texture binding/disable path and explicitly resets texture2 routing at material
handoffs. It does not copy Melee's descriptor cache shortcut into this change,
add raw register writes or alter the frame owner's resource retirement.

V3 reduced uploads to9,559 but did not recover the CPU gate. V4 also prepares
the same sampler layout for constant-opacity unlit materials, preserving their
original TEV operations with no C1 buffer. It binds the existing neutral image
in the unused map slot. Lit/unlit parts can therefore share the pipeline;
no additional image allocation is involved. Materials requiring vertex alpha
retain their ordinary native path. Map-lit and plain routed draw counts are
reported separately. No simulation or animation work is omitted.

V4 reduced pipeline uploads to4,968 but its exact-binary New ON/OFF pair still
added .297975ms scene+submission, above .20ms. Both ran about59.8248 AzaharFPS;
unchanged pacing is not evidence that the cost vanished. It was not accepted
as a speedup. V5 adds conservatively bounded reuse of diffuse-map contents:
ambient/light colours and light count must match, while light-position changes
must bound every RGB channel's continuous output change by1/512, under half an
8-bit level. The comparison always uses the last state actually drawn, so the
tolerance does not accumulate over a long movement. Light-colour and ambient
changes force redraws. This is an explicit, small rendering approximation,
not exact source-image identity; source game/light state is unchanged.

The bound uses authored normal length below2, map-position components within2,
and `|normalize(a)-normalize(b)| <= 2|a-b|/|a|`. L1 displacement and a maximum-
component lower distance bound avoid square roots or normalization on the CPU.
Each of at most8 lights receives an equal share of the error budget. No extra
resident image or worker is needed. The original map shader is unchanged.
The actual mesh oracle checks7,392,909 RGB values across8,527 accepted and7,473
rejected states, maximum sampled difference.05854 of an8-bit level. The exact
assembled Azahar float24 shader checks119,646 accepted RGB pairs, maximum
.02828 levels. The cumulative-motion test forces20 redraws with980 reuses;
ambient/colour/count changes and NaNs reject reuse. This does not establish
physical GPU precision or timing for every game state.

Checks execute the actual load compiler and pinned SDK descriptors: 3,788,800
RGBA comparisons, masked/constant/intensity cases, four layer operations and
both lighting orders. Wrong base/mask unit mutations fail. Actual live binding
checks exercise 64 animated masked/base cases and32 independent layer frame,
scale and scroll handoffs for diffuse and specular. Actual selection/constant
checks cover83,968 cases and reject the half-opacity mutation. The existing
environment owner passes ordinary/fog-only/layered transitions, including
eight-light admission and cached LUT publication. All run with ASan/UBSan.

V5's New check completed with958 map updates,1,338 reuses (839 bounded),
5,140 pipeline publications and scene1.904964/submission4.316948ms. That did
not establish a useful submission gain over exact V4; animation phases and
workloads differ. V5 is retained as experimental evidence, and its reuse helper
is **not called by the current renderer**. The map owner was restored byte for
byte to V4 (SHA256`dee2e83327bc23498e7a25221655db6fbc7fd8b32035228400358d88df66da0b`).
Do not select V5 just because its version is higher. New's CPU budget remains
open. V4 normal`077590850267`/CIA`1c1d48fc2d4b` and replay`28948d103c2a`/
CIA`d29fd8bf192e` share the game archive and329 native source files. Exact
511/214/197/73-instruction shaders match the previously checked binaries.
Old80/New124 CIA profiles are verified. Both are built, not deployed; LIGHT
MAPS remains OFF by default. The unused reuse helper is retained for host
experiments only and has no current renderer consumer.

V4 New ON/OFF scene1.934108/1.851332ms and submit4.315964/4.100765ms give
the .297975ms increase above. Old ON completes at57.5343 AzaharFPS, with
scene1.927442/submit4.332433ms; no current Old OFF pair or physical timing
claim. All three complete state assertions, save preservation, log draining
and owned-process exit. New/Old1410 images were inspected: palace geometry,
Krystal, creatures/glows and HUD remain present. No full original-image
fidelity acceptance follows from these captures.

Measured New/Old normal heap peaks are48,472,720/71,303,168B and
40,936,056/50,331,648B; minimum free linear18,396,416/25,165,824B and
3,716,352/10,485,760B; minimum free VRAM4,822,016B; stack72,796/524,288B.
Process commitments103,206,912/130,023,424B and67,555,328/83,886,080B stay
inside the declared grants. These are scene-specific measurements, not global
guarantees. V4 New records34,887 map-lit draws,12,662 plain routed draws and
7,601 layered draws; no unready maps. Masked runtime draws are zero: that
family has source/compiler/binding coverage but no live palace acceptance.
117 admitted/zero partial declarations refer only to already translated load
families, including menu assets, not complete original material coverage.

None of these candidates establishes a fix for intermittent disappearing
trees, the white Arwing, physical filesystem stalls, full day/night fidelity,
shadows or the remaining source fur displacement.

The matching normal app completes its25.02-second/721-retrace isolated check:
startup, LIGHT MAPS ON/OFF/ON, settings persistence and Select shutdown all
pass. The540 capture was inspected and FHGRAPH4 mask8404993/checksum verified.
Only this isolated test profile was changed. No FTP, deployed files, private
assets or console saves/preferences changed during this work. All owned test,
build and emulator processes are terminal. The next acceptance step is reducing
the added cost with comparable-workload evidence, followed by original-image
and hardware validation; the performance gate remains explicitly failed.
