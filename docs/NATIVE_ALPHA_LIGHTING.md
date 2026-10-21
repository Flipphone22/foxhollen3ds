10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native actor alpha lighting

10 October local candidate, not deployed. Replay f8a1635f6bd1 (actor-alpha-v2-builds)
adds the alpha channel selected by the source model's flags24 bit 0x10. This is
a correction to the existing LIGHTING option, not a new draw pass. Fox, Krystal
and Hagabon models do not select this mode; this change is not a demonstrated
fix for their reported eye, RGB tint or particle issues.

The source objSetupLightChannels selects GX_COLOR0A0 for these models. The native
adapter previously retained only the RGB light colours. It now retains all four
components, ambient alpha and the source vertex/register material selection.
The PICA vertex shader accumulates and clamps RGBA, applying light alpha only
when the original selected it. The object-colour override retains unlit alpha.
Existing LIGHTING OFF and shadow passes disable the new branch.

The original object light selection stays on the CPU. This change adds one
boolean and fills the previously unused fourth components of existing light
uniforms. It does not introduce vertex rebuilding, GX interpretation, texture
copies, render targets, resource pools, workers or extra draws. It follows the
pinned Melee vertex-light/command ownership arrangement; the separate alpha
semantics are a Foxhollow-specific adaptation.

tools/test_pica_alpha_lighting.py extracts the original object-channel and
diffuse-light functions, observes their GX channel controls, and compares the
native parameters/equation over 9,792 alpha samples. All match, and a mutant
that discards the light alpha fails. This is a CPU source-equation comparison,
not physical PICA float24 or screenshot acceptance. The private asset audit
finds this model flag on 23 of 843 unique source models across 53 banks.

The build now validates and reports the actual pica_scene_actor_hud_shader
embedded in the ELF through tools/native_shader_report.py. The seven-entry
native program grows from 470 to 476 of 512 shared instructions, leaving 36.
Its actor entry uses 96 float vectors and 14 of 16 booleans. Older minimal-path
shader reports are not measurements of this active program. The gate also
requires that linked bytes match the freshly assembled program.

Short New cave and outdoor Krystal checks, and an Old cave check, pass replay
state, shutdown, save preservation and owned-process checks. Captures were
inspected. These scenes report zero alpha-lit objects: they establish unaffected
rendering only. A live affected scene and physical validation remain open.

New cave shadows-off comparison: 59.831 FPS in Azahar before/after; native
submission 5.073268 -> 5.074215 ms, scene preparation 1.812184 -> 1.828756 ms.
The outdoor candidate reports 59.831747 FPS, scene 0.628617 / submit 1.150576 ms.
Neither result establishes physical performance or a speedup.

Matched Old cave baseline/candidate checks report 57.920754 -> 58.124128 Azahar
FPS, scene 1.815048 -> 1.787165 ms and native submission 5.096551 -> 5.085514 ms.
The approximately 0.2 FPS difference is within the variation of these short
runs, not a demonstrated speedup. Neither has observations exceeding 250 ms.

Cave memory is unchanged: New heap peak 48,109,904 / 71,303,168 bytes, Old
40,769,872 / 50,331,648; linear free New 19,474,048 / 25,165,824, Old
4,793,984 / 10,485,760; VRAM free 4,858,880. Measured main stack is 72,836 of
524,288 reserved bytes. Process commitment New 103,141,376 / 130,023,424,
Old 67,489,792 / 83,886,080. The outdoor scene's larger heap peak is 48,184,464.

Evidence: artifacts/research/pica-scene-rewrite/lighting-channel-audit,
actor-alpha-v1-builds, actor-alpha-v2-builds, and planar-shadow-test summaries
for actor-alpha-v1-cave-disabled-new, actor-alpha-v2-cave-disabled-old and
actor-alpha-eyes-candidate-new. Console installations and saves are unchanged.
