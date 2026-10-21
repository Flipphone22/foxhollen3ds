10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native specular restoration: local implementation work

10 October: native map generation and a first material family are connected to
gameplay in local replay `d8f70d951f45` (not deployed). The SPECULAR control is
on graphics page 9, default OFF on both models. This is partial restoration,
not original-game visual parity or physical-console validation.

Three persistent 32x32 RGBA8 VRAM attachments consume 12,288 bytes. One immutable
8,852-byte linear mesh is prepared once; the original 0x5dc base image uses the
existing bounded native image pool. Resident resources remain allocated with
the toggle OFF, so OFF removes rendering work rather than that storage cost.
The attachment descriptors have no depth allocation or separate target owner.
The verified main queue flushes writes before subsequent sampling; normal
shutdown retires GPU readers before textures, mesh and base-image reference
are released. No promotion/demotion, CPU readback or per-frame geometry copies.

Mode-1 RCP slots are sampled at the original group boundary, before the native
diffuse callback resets the group. Source sun/moon directions, lobe parameters,
material bytes and diffuse alpha are live uniforms. Equal snapshots reuse their
maps. Each changed map takes two GPU draws, retaining RGB and alpha separately
until fragment interpolation and addition. This adapts Melee's queue and fence
ownership, avoiding its documented dynamic VRAM-promotion failure path.

The first consumer compiles four native TEV stages at material load: retained
diffuse C1, selected red/blue secondary lobe, the original green/alpha addition,
then the base texture. Native view-normal UVs use the existing scene shader,
which stays at 511 instructions after folding two light multiply/add pairs.
All other sampled shader outputs remain bit-identical in the linked regression.
Masked, layered, bump/auxiliary and secondary-mask families are counted as
partial, retaining their existing base rendering. This palace load admitted
12 material declarations and counted 39 omissions; counts include loaded menu
resources, not just currently visible actors. No unavailable-map draws occurred.

The New profile ON/OFF palace checks both measured 59.83 Azahar FPS. Scene
preparation plus native submission was 1.853949+3.950051 ms ON versus
1.819457+3.930982 ms OFF: +0.053561 ms, within the 0.20 ms investigation gate.
The complete ON run used 782 map updates, 1,319 reuse hits, 1,564 map draws and
4,854 material draws. Map submission's own 40.10 ms total is nested evidence,
not an additional per-frame cost to add to the frame timings. Animation phases
can differ; these short runs are not deterministic full-game comparisons.
The Old profile ON/OFF completed at 57.389/58.598 Azahar FPS. Scene preparation
plus submission was 1.840293+3.961589 versus 1.827539+3.943194 ms: +0.031149 ms,
within the 0.50 ms CPU gate. Total cadence is lower ON; the largest timing delta
is +0.348674 ms in the frame's `other` bucket, so it is not dismissed as an
unchanged-performance result or attributed to GPU work without evidence.
These runs do not forecast Old console throughput. Captures retain world, actors, particles and HUD;
original-image highlight shape/brightness acceptance remains open.

New/Old peak normal allocations were 48,204,832/40,864,800 bytes against
71,303,168/50,331,648-byte heaps. Minimum free linear was 18,945,152/4,265,088
bytes against 25,165,824/10,485,760-byte grants; free VRAM 4,846,592 bytes.
Measured main stack was 72,844 bytes of 524,288 reserved. Default remains OFF
pending broader material/image acceptance and hardware costs.

New checks: 100 resident resource lifecycles and 100,000 unchanged updates under
ASan/UBSan, plus a missing-publication mutant; 196,608 material RGB comparisons
using actual Citro3D descriptors, with alpha and retained-buffer checks; 32,768
live-normal UV cases and ordinary/fur linked float24 regressions; graphics menu,
recording lock, persistence and idle no-flush tests. The first local test recipe
wrongly placed GRAPHICS after input steps; the parser rejected it. That failed
run is retained, its owned processes stopped, and the correctly ordered recipe
passed. No parser checks were relaxed. The first consumer build hit the existing
strict <512 link guard; the additional fused light operation restored 511 without
relaxing that guard. All runtime evidence uses the successful archived binary.

The original `rcp_dolphin.c` uses three 32x32 secondary-light maps, each with
two lobe widths. `Rcp_SetupDistortionLights` supplies separate sun and moon
channels: red/blue specular responses multiplied by their diffuse alpha, then
added to the original texture 0x5dc in `addSphereMapLitStages`. Material selection
chooses a red/blue lobe through `reg2TexSlot`; it is not generic white shininess.
Live material masks, `reg2Alpha`, pre/post-light layer order and the separate
diffuse sphere contribution must survive integration.

The original six radius/strength pairs produce attenuation scales about
12.31445,12.31445,21.61024,44.62169,123.98050,21.61024. The lobe is
`h*h / (scale/2 + (1-scale/2)*h*h)`, with the original facing, denominator and
channel clamps. A 256-entry PICA light LUT using the actual pinned Citro3D
encoder has maximum response errors of 0.000693,0.000693,0.001306,0.005436,
0.031622,0.001306 over 262,145 samples per curve. Four meet a one-byte
comparison gate, two do not. This does not make LUT rendering impossible or
prove visible error in a 32x32 map; it rules out calling the unmodified LUT
an exact replacement. The six LUTs alone would occupy 6,144 ordinary bytes.

The separate program `pica_specular_map.v.pica` evaluates a source light
channel directly on PICA. It has **30 instructions, 6 float registers including
constants, no boolean registers and three outputs**. It is a separate program;
it does not extend the existing 511-instruction actor/HUD program. Direction,
half-vector, both attenuation pairs, material colour and diffuse-alpha scale
are small uniform inputs. RGB and diffuse alpha remain separate interpolants
so the native combiner can multiply them after raster interpolation. Computing
their product per vertex would change the original operation order.

`pica_specular_map.c` prepares one immutable grid: 289 vertices, 1,536 indices,
8,852 bytes. It preserves all 512 original triangles, winding, border overdraw
and non-unit corner normals. Source orthographic X is flipped explicitly.
Preparation is intended once at resource initialization, not per frame.
PICA will perform light evaluation; this is not CPU per-vertex lighting.

Validation:

- Actual original mesh generation versus the new compiler: all 1,536 ordered
  corners have exact normal bytes and matching projected XY; ASan/UBSan pass.
- Assembled shader output/control-flow check passes.
- Azahar's float24 interpreter executes 221,952 vertices across all six curves,
  moving light directions and sun/moon alpha scales; max error 0.000003636.
  An incorrect-denominator shader mutant fails. This is shader execution only,
  not rendered image, interpolation, hardware precision or GPU timing evidence.
- LUT experiment executes original slot/attenuation initialization and actual
  Citro3D packing. The decompiler's float-to-signed-byte store is expressed as
  truncate-to-integer/unsigned-low-byte in the host harness to avoid C UB. The
  game and its parameters are unchanged. An initial harness run exposed that
  signed-byte UB and was not accepted as validation.

The read-only source inventory finds 382 used secondary-light materials across
203 models: 321 single-layer, 61 two-layer declarations. It covers 829 unique
source models, with eight alternate/unsupported instruction streams listed in
the report, so it is not a whole-game coverage proof. Opacity planes, callbacks,
indirect/auxiliary textures and live material masking still need admission checks.

Next work is the remaining masked/layered/material families, direct original
image comparisons, and hardware validation of target-to-sampler transitions and
cost. Do not restore a GX interpreter or CPU geometry path to cover those cases.

Proposed initial budget is <=0.20 ms added New Azahar CPU preparation/submission,
<=0.50 ms Old, with no per-frame allocations. These are investigation gates,
not physical guarantees. Account separately for target passes, resident target
storage, command uploads and GPU work before enabling the effect. No additional
graphics worker is assumed; the New audio worker already uses core 2.

Evidence under `artifacts/research/pica-scene-rewrite/`:
`specular-{lut,shader,mesh}-test/` and `specular-material-audit/report.json`.
Tests: `tools/test_pica_specular_{lut,shader,mesh}.py` and
`tools/audit_pica_specular_materials.py`. Original assets and saves are unchanged.

Integrated evidence: `specular-runtime-test/runtime-comparison.json`,
`specular-{owner,material}-test/`, and
`specular-material-v2-builds-consumer-shader-test/`. Reproduce with
`build-specular-material-v2.py`, `check-specular-material-v2.py new on|off`
or `old on|off` in the same research directory. Those builders explicitly
isolate the unrelated native-image VRAM WIP and restore it afterward.
