# Per-object native shadow direction, 11 October 2026

The projected native shadow adapter used global `gShadowOffset*` and fixed
opacity 150. The original projected-caster path instead selects a light for
each object, smooths that direction, clamps shallow angles, and updates the
object's shadow fade/hold/tint state. This was a source mismatch affecting
lighting transitions. It is separate from the reported invisible trees,
white Arwing, surface lighting and fur defects.

The replacement calls the original `skyGetObjectLightDirection`, including
its sky slot, overrides and local-light selection. A small scalar helper
reproduces the original `newshadows.c` 90/10 filter and -0.707 Y clamp. It
updates the existing object's shadow offset and supplies that vector to the
existing GPU projection. Projected casters now use `objShadowUpdateAlpha`
with `framesThisStep`; static CRASH decals keep their original alpha policy.
The existing object queue selects one shadow route per object. The retired
`renderShadows` render-target path stays disconnected.

No additional resident allocation, shader instruction, texture, worker or draw
pass is introduced. CPU work remaining is source light selection and a few
scalar vector operations per admitted silhouette. Pinned Melee's explicit
state ownership/invalidation and resource lifetime rules were reviewed;
existing native GPU owners, cache flushes and retirement fences are unchanged.
The existing GROUND SHADOW and SHADOW SHAPE toggles apply, default off.

`tools/test_pica_shadow_direction.py` extracts the actual original direction
block and compares 131,072 sequential cases, including reversals, poles and
the clamp: zero error with ASan/UBSan. A wrong-smoothing mutant fails.
Nonfinite input is rejected without modifying the stored direction.

Serial unattended CIA-content runs use the same ThornTail idle fixture and
original geometry/textures, with particles, fur, specular and silhouettes on:

| Azahar profile/build | FPS | Scene CPU ms | Submission CPU ms |
|---|---:|---:|---:|
| New, deployed I/O baseline | 50.19753 | 2.47181 | 5.80106 |
| New, direction correction | 50.19896 | 2.48215 | 5.81155 |
| Old, direction correction | 40.88372 | 2.45146 | 5.85939 |

New's combined difference is +0.02083 ms. These are short relative checks,
not hardware FPS or a speedup. Animation/material phases differ slightly;
there is no Old before/after pair. Baseline build activity finished during
startup, before measured gameplay. The earlier read-only object-observer run
is diagnostic only and is excluded from this timing comparison.

All three runs pass fixture/state checks, save preservation, log drain and
owned-process exit. New before/after and Old retrace-1260 images were inspected:
world, Fox, HUD and shadows remain visible. This static daytime scene does
not accept the complete day/night transition or every local-light case.
The corrected New run submitted 3,288 projected silhouettes. Sparse/striped
foliage remains visible in the captures and is not claimed fixed.

After-change memory: New heap 48,819,712 / 71,303,168 B; minimum linear free
16,545,152 / 25,165,824 B. Old heap 41,496,448 / 50,331,648 B; linear free
1,873,280 / 10,485,760 B. Both VRAM free 4,846,592 B and measured main stack
72,860 / 524,288 B. Process commitments remain New 103,186,432 / 130,023,424 B,
Old 67,534,848 / 83,886,080 B. No new allocation is attributable to the change;
observed small heap/linear differences reflect different loaded/animated phases.
Old's headroom is much smaller in this original-asset/effects configuration.

Evidence: `artifacts/research/pica-scene-rewrite/shadow-direction-test/` and
`artifacts/native/shadow-direction-v1-{before-new,after-new,after-old}/`.
Local builds are under `shadow-direction-v1-builds` and
`shadow-direction-normal-v1-builds`; shared Old80/New124 profiles are retained.
No console deployment or physical validation is part of this correction yet.

Remaining shadows still use a hard silhouette and one receiver plane. Soft
filtering, finite/multiple/moving receivers and self-shadow fidelity remain
incomplete. This change does not establish full lighting parity or meet the
previous whole-shadow CPU budget by itself.

## Visibility investigation

Read-only live object capture identifies the affected landed ship as
`Landed_Arwi`, definition 1196, model 1152; the earlier WORLDarwing model 422
was the world-map asset. ThornTail fern trees are `SH_FernTree`, definition
663, model 238. Both use the `swaphol` source bank. Inventory and source texture
inspection are recorded in `thorntail-visibility-source.json`,
`thorntail-textures/`, and `artifacts/native/thorntail-visibility-v1-new/`.

The ship's first material has an auxiliary bump texture unsupported by the
native specular admission rule. This is a confirmed missing family, not yet
proof that it causes the reported all-white ship. The fern texture is a
196x512 CMPR atlas with transparent foliage and bark. Its source is intact;
live sampling/visibility still needs comparison. The normal actor path
rebuilds its attribute layout on the first draw, so the suspected compact
shadow fixed-attribute leak was ruled out. Neither issue has been repaired
by this shadow direction change.
