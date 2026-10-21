# Specular with a second texture after lighting

This restores a previously omitted native material family under the existing
SPECULAR option. It is locally validated, not physically validated or deployed.
The pending skybox repair is independent.

The original `objSetupRenderOpGxState` and `addShaderLayerStages` place layers
with type bits `[129, 0]` on opposite sides of lighting. Nineteen inventoried
materials also use diffuse lighting, base-modulated secondary light and zero
secondary-light alpha. They include Hagabon model46 and model1257; the palace
replay exercises both. Six other post-light declarations use different
secondary-alpha rules and remain unsupported by this specialization.

For base A, overlay B, opacity `a=A.alpha*materialOpacity`, lighting L, and
secondary-map channel S, the source equation is:

```
litBase = clamp(A*L + clamp(S*L)*A)
RGB = litBase*a + B*(1-a)
alpha = a*B.alpha
```

The new four-stage PICA program computes L, then `(S/2 + 1/2)*L`, multiplies
by A with a scale of two and saturation, then interpolates B. Saturation stays
before the overlay. It uses the existing half-intensity GPU map and the existing
three-coordinate/fragment-fog shader. It needs no delayed TEV buffer, extra
geometry pass, shader instructions, CPU vertices, texture allocation or worker.
Colour fade and distance fog retain stages4 and5. Existing SPECULAR/lighting/
texture gates remain authoritative; option defaults are unchanged.

The pinned Melee `bind_program`, retained raster-state cache and invalidation
around target changes were consulted. This adaptation uses the existing native
SDK binding and resource owners; it does not introduce a GX interpreter or
bypass cache coherence and retirement. Live CPU work adds constant selection
for the new plan and a diagnostic counter. A different material shader is now
selected for these draws, so increased command traffic is a real cost.

Source validation executes 65,536 original GX stage-constructor sequences with
262,144 RGBA comparisons, maximum arithmetic error1.79e-7. The initial fixture
incorrectly used the Swizzled mode8 constructor without a diffuse/C1 producer;
its failure is retained. All19 actual candidate declarations have that producer,
and the native compiler rejects the unsupported no-diffuse variant. This is an
explicit admission rule, not an assumption that arbitrary C1 state is valid.

The actual PICA SDK descriptors pass262,144 RGBA stress cases, ideal maximum
error2.39e-7. An8-bit intermediate model with the existing128/255 GPU half-map
has maximum RGB error4.860 and alpha error1.969 byte levels. Moving saturation
past the overlay fails751 witnesses. The existing1,671,168 source RGB checks,
2,185 admission rejects, old layered descriptors, eight new compiler descriptors
and3,584 live constant publications pass with ASan/UBSan. Removing the existing
C1 buffer publication still fails the negative control.

Serial matching palace recipes passed in New and Old Azahar profiles, with
normal saves unchanged, state assertions, complete log drain and owned exit.
Candidate/baseline1410 captures were inspected: world, Krystal, creatures,
EarthWalker, particles and HUD remain present. Pose/particle phases differ, so
these are not exact image diffs or proof of full original-image fidelity.

| Emulator profile | Before scene + submission | Candidate | Difference | Local incremental budget |
| --- | ---: | ---: | ---: | ---: |
| New | 5.708 ms | 5.865 ms | +0.157 ms | 0.20 ms |
| Old | 5.737 ms | 5.853 ms | +0.116 ms | 0.50 ms |

The candidate produced6,060/5,940 new-family draws in the full New/Old runs,
with zero unready specular samplers. Azahar cadence was59.821/58.679 FPS;
these are not console forecasts. Whole-run command bytes rose73,225,072 to
81,969,456 on New; animated workload counts differ, so this is not a fixed-state
per-frame comparison. Physical command/CPU/GPU cost remains unvalidated.

New heap peak48,480,968/71,303,168 bytes; linear free18,188,288/25,165,824.
Old heap peak40,944,304/50,331,648; linear free3,508,224/10,485,760.
Both retain4,723,712 bytes free VRAM and72,996 bytes measured main stack against
524,288 reserved. Process commitment103,301,120/130,023,424 New and
67,649,536/83,886,080 Old. New measured memory is unchanged versus its baseline;
Old allocation phase differences are recorded in the JSON comparison.

Reproduction and evidence:

- `tools/test_pica_layered_source.py --postlight`
- `tools/test_pica_postlight_specular_plan.py`
- `tools/test_pica_postlight_owner.py`
- `tools/build_postlight_specular_v1.py`
- `tools/check_postlight_specular_v1.py {new,old} on [--baseline]`
- `tools/summarize_postlight_specular_v1.py`
- `tools/verify_postlight_specular_v1.py`
- `tools/build_postlight_specular_normal_v1.py`
- `tools/verify_postlight_specular_builds.py`

Builders isolate the complete accepted snapshot, shelf unrelated native WIP
outside build globs, and restore sources/archive in `finally`. Only model.o
changes in the game archive, from its material-load adapter. Main shader stays
511 instructions; the existing layered program stays304. Original assets,
simulation, saves, shader instruction limits and Old80/New124 CIA grants remain
intact. Raw logs and measurements are under `postlight-specular-owner-test`,
`postlight-source-test`, and `postlight-specular-plan-test` in
`artifacts/research/pica-scene-rewrite`.
