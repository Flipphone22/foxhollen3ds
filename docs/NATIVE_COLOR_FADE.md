10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native object colour fade

Local replay `40d4e3e663e5` and matching normal `1a8df06e30b2`; not deployed. The original gameplay colour-fade
animation now feeds native actor draws. This restores an omitted material
operation used for damage flashes and other object tints, without changing
gameplay or reconnecting the GX renderer.

The generated object adapter snapshots the original ACTIVE/OVERRIDE flags and
four unsigned colour bytes after the live light snapshot. Native TEV stage 4
interpolates previous RGB toward that colour using its alpha as strength. It
preserves previous surface alpha, including texture cutouts. Existing distance
fog follows in stage 5. World geometry and planar shadows explicitly disable
the object tint; UI/custom light snapshots clear inherited object colour.
Shader/material ownership changes invalidate the stage's resident state.

The distinction between fog families is now protected explicitly by
`fh_pica_post_stages.h` and `tools/test_pica_post_order.py`. The original
height-dependent `renderHeavyFog` is a material operation before colour fade;
it remains missing. The existing native **distance fog** models GX fixed fog
after the completed TEV output, so its stage5 must remain after fade stage4.
A proposed swap was rejected before building; no swapped version was deployed.
The combined actual-owner/SDK check covers101,376 RGB samples, alpha retention,
independent toggles, cached binds and handoffs; a swapped-stage mutant fails.
Existing196,608 fade and200,020 distance-fog checks also pass under ASan/UBSan.
Local replayced92144ca2d has byte-identical fog/fade function bodies, the same
game archive and the same511-instruction scene shader as compact-shadow-v2.
Those unchanged runtime results are reused; no extra emulator boots or FPS
claim are justified by this enum/ownership refactor. No deployment.

The design follows the pinned Melee renderer's explicit native combiner-state
ownership and citro3d interpolation helpers. It does not interpret GX commands
per draw. There are six unsigned state/counter words (24 bytes), no allocation,
vertex loop, worker, texture upload, extra geometry pass or shader instructions.
Repeated matching stage state is not published again. This essential feedback
is enabled on both models; it does not need an expensive-effects toggle.

`tools/test_pica_color_fade.py` executes the extracted original
`addColorFadeStage` and actual native/citro3d stage code. All 196,608 scalar RGB
comparisons pass under ASan/UBSan, with alpha preservation, unaffected adjacent
stages, redundant-publication avoidance and owner handoff checks. Reversed
interpolation and missing invalidation mutants fail. These scalar comparisons
do not establish physical fixed-point rounding. The existing planar handoff
test also passes 96 entry/publication/exit cases with fog and HUD preservation.

Serial exact-console-binary headless Azahar checks pass state assertions,
save preservation, log drain and owned-process exit on both emulated models:

| Cave | Before FPS | Candidate FPS | Before submission ms | Candidate submission ms |
|---|---:|---:|---:|---:|
| New | 59.831223 | 59.831222 | 5.208310 | 5.236425 |
| Old | 57.171782 | 57.163946 | 5.232470 | 5.251664 |

The comparison uses the preceding morph candidate and the same recipe. Some
animation/particle phases differ; this is a short regression check, not exact
fixed-state performance proof or a console FPS estimate. The cave itself only
exercises one tinted draw during startup, so it is not visual acceptance of
damage feedback. Both cave capture 1410 images were inspected.

A separate palace movement recipe exercises 2,350 active object publications
and 5,694 tinted draws. Captures 1500 and 1800 show Krystal changing from normal
blue to a red/purple tint, while adjacent world geometry, the purple creature,
particles and HUD retain their colours. Its measured window runs at 59.831221
Azahar FPS, submission 1.753276 ms and scene preparation 0.964635 ms. This is a
different camera/workload from the ordinary palace replay, not a speedup claim.
The recipe changes ordinary scripted movement only; no recorded input tape or
gameplay state was edited. Complete damage sequences remain to be compared with
the desktop reference and physical console.

Cave peak heaps are unchanged: New 48,114,488/71,303,168 bytes and Old
40,774,456/50,331,648. Minimum free linear is 19,358,464/25,165,824 and
4,678,400/10,485,760. Initialized VRAM free is 4,858,880; measured main stack
72,836 of 524,288 reserved. The palace peaks at 48,199,104 normal bytes,
19,325,056 free linear. Both profiles retain their 124/80 MiB process grants.

This does not fix every reported red/green lighting issue. Heavy/height fog
itself is still missing, including its original ordering relative to colour
fade. Source material exclusions and every special callback need broader
visual coverage. Evidence is in `color-fade-test/{result,integration}.json`
under `artifacts/research/pica-scene-rewrite`, with native captures in
`artifacts/native/color-fade-v1-*`.

The matching normal app was built from the same hash-verified game archive and
renderer sources; its build and source manifest are archived under
`color-fade-normal-v1-builds/console`. Normal-only UI behavior was unchanged,
so the preceding ordinary-app menu/persistence evidence is reused rather than
reported as a new normal runtime test. Both builders isolated and restored the
unrelated native-image VRAM work. No remote assets, applications or saves changed.
