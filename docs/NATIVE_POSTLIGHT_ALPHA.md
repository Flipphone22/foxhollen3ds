# Secondary-alpha specular with a post-light overlay

Local candidate only; not deployed or accepted as a complete visual repair.
The native material compiler now has a four-stage route for six inventoried
two-texture materials with secondary alpha. Authored object definitions identify
Andross model1223 materials6/8, DIM_Boss84 material3, BossGeneral79 material1,
WM_spiritpl/WM_Animspir606 material2 and WM_krazoast573 material1.
The earlier description of model1223 as an Arwing was incorrect: `arwing` is
its asset-bank name. This work must not be presented as a parked-Arwing repair.
`audit_postlight_alpha_identity.py` records the actual object/model mapping.
The sixth material uses KAlpha with raster lighting rather than the other five
materials' Swizzled constructor with a diffuse sphere map. Both source routes
are now checked; actual gameplay admission and appearance still need coverage.

The original `addLightTexReg2Stage`, `addAlphaLitColorReg2Stage` and
`addTexLayerStageSwizzled` give:

```
L = clamp(raster + diffuse * strength)
C2 = clamp(S * L + G * secondaryAlpha * L)
litBase = clamp(base * L + C2)
a = baseAlpha * objectMaterialOpacity
RGB = litBase * a + overlay * (1-a)
alpha = a * overlayAlpha
```

The existing half-map supplies S/2 and G/2. Stage0 computes L and half the
green-alpha product, retaining RGB in the delayed TEV buffer. Stage1 uses
ADD_MULTIPLY and scale2 to compute C2; its independent alpha prepares a.
Stage2 computes litBase from retained L. Stage3 performs the overlay.
This preserves both saturation boundaries and stages4/5 for fade/fog.
No new shader instructions, geometry passes, maps, resident allocations,
workers or CPU vertices are required. Material compilation happens on load;
live work adds constant selection and a counter, with no established cost yet.

The pinned Melee program binding/cache invalidation and the pinned Citro3D
TEV descriptors were consulted. Existing native program publication restores
the RGB buffer mask1; alpha buffering remains disabled. Existing map ownership,
ordered publication and retirement remain unchanged. The compiler admits only
the verified post-light, constant-opacity, unmasked non-intensity family.

Validation completed:

- 131,072 actual original source stage sequences, 524,288 RGBA comparisons;
  maximum arithmetic error1.79e-7. Covers diffuse-map and KAlpha/raster modes.
- Actual SDK descriptors:262,144 RGBA samples, ideal error1.79e-7;
  conservative8-bit model maximum6.106RGB/1.973alpha byte levels.
  Removing retained L is rejected by474 witnesses.
- Actual compiler/constant branches:4,080 descriptors,7 admission rejects,
  522,240 constant publications; prior1,671,168 RGB checks and old layered
  checks remain passing. Host checks use ASan/UBSan.
- Local-only Azahar GPU check uses the real half-map owner and descriptors:
  64 samples maximum RGBA errors1,1,2,1 bytes. Fragment fog and primary-light
  controls pass. Bounded2.93-second run exits with resources retired; diagnostic
  constant-colour capture inspected. This is not game-image acceptance.
- Initial isolated ARM replay build605297662db2; v2bd6b038102fc includes the
  verified non-diffuse family. Same game archive and native shader
  programs as the accepted postlight base. Shared Old80/New124 CIA retained.

Reproduce with `test_pica_layered_source.py --postlight-alpha`,
`test_pica_postlight_alpha_plan.py`, `test_pica_postlight_alpha_owner.py`,
`build_postlight_alpha_v2.py`, `verify_postlight_alpha_v1.py --v2`,
`build_postlight_alpha_gpu_check.py`, and `run_postlight_alpha_gpu_check.py
--label <unique-label>`. The GPU check is local-only; no new console probe was
installed or requested. Initial plan-test compilation missed two test-stub
enums; fixing those did not change the native descriptor.
The inherited owner-test wrapper also had an ineffective output-directory
replacement and initially wrote to `specular-material-test`. The wrapper now
replaces the full directory substring; rerunning produced the dedicated
`postlight-alpha-owner-test` evidence. The earlier output was not treated as
unchanged baseline evidence. No emulator rerun was needed for that correction.

The builder snapshots only two changed owner/compiler files and the new plan
header, restores unrelated work in `finally`, and leaves the archive unchanged.
Pure patch `pica_postlight_alpha_patch.py` is NOT applied to global owner files.
This candidate does not include the pending cloud shader. Evidence is under
`postlight-alpha-{source,plan,owner}-test`, `postlight-alpha-v{1,2}-builds`, and
`artifacts/diagnostic/postlight-alpha-gpu-v1`. Full gameplay image comparison,
runtime cost/memory and physical validation remain open; no FPS gain is claimed.

The private upper-palace warp65 fixture completed normally in42.23 host seconds
on the New profile, with unchanged normal saves and owned exit. Capture1080
was inspected: Krystal, palace surfaces and HUD remain. It produced **zero**
secondary-alpha material draws, so it is not affected-path acceptance; no
baseline or Old replay was repeated for this ineffective coverage fixture.
Its unrelated239-frame window reports59.831 Azahar FPS,0.818ms scene and1.660ms
submission. Heap48,695,488/71,303,168B, linear free17,999,872/25,165,824B,
VRAM free4,723,712B, measured stack73,008B, process103,301,120/130,023,424B.
These do not establish incremental cost or physical FPS. See
`postlight-alpha-owner-test/upper-fixture-result.json`. Position-only fixture
metadata identifies the authored warp and preserves the original save; it is
never installed or counted toward normal progression.
