# Native texture colour selection with opacity masks

11 October: local implementation. Hardware and original-image acceptance remain
unproven. This extends native channel selection to an existing separate-opacity
material family; it is not a demonstrated fix for the white Arwing, disappearing
trees, day/night/shadows, fur or SD stalls.

The source `shader_dolphin.c::addTexLayerStageSwizzled` selects RGB from its
24-entry channel table but keeps alpha independent. When `imageOffset` supplies
a separate opacity image, its second stage replaces base-image alpha with
mask alpha times KAlpha. The existing native actor-alpha compiler already
prepares that rule. The channel compiler previously rejected these materials.

`fh_pica_texture_swizzle_opacity_compile` now admits exactly the existing
single-base/opacity descriptor: RGB remains a pass-through in the mask stage,
while alpha multiplies PREVIOUS by the mask's alpha or intensity channel.
Unrelated layer operations are rejected. RGB reconstruction occupies stages0–2
and lighting stage3, while alpha follows its original two-stage sequence.
Environment variants retain the C1 RGB buffer in stage0 and shift the two alpha
stages to1–2. Optional LIGHT MAPS remaps map/base/mask to samplers0/1/2.

Sampler coordinates, texture ownership, source updates and visibility rules are
unchanged. Programs are prepared and reference-counted at model load. The bind
path selects existing plans and masks; there is no new per-frame interpretation,
image, vertex work, draw pass or worker. No new per-material fields are needed;
one four-byte counter distinguishes actual masked draws from other channel draws.
The unchanged128-entry program pool may hold additional shared descriptors.

Pinned Melee `renderer.c` persistent raster-state invalidation and the pinned
Citro3D `texenv.h` setters were consulted. Existing program invalidation and
SDK publication remain in charge; no raw register ownership is introduced.

`tools/test_pica_texture_swizzle_opacity.py` executes the actual actor and alpha
compiler functions plus pinned SDK descriptors. It checks1,376,256 RGBA cases
against source channel selection and the source's independent mask-alpha rule,
including base/mask intensity, mapped/unmapped and ordinary/environment routes.
Wrong mask sampler and omitted environment-mask mutants fail. The initial host
compile exposed missing SDK alpha-enum names in the test stub; the stub was
corrected without changing the renderer to satisfy it.

Existing ordinary/environment tests pass786,432 and589,824 cases. The actual
owner/binder check now includes masked/unmasked lifecycles and passes200,000
bindings over200 lifecycles, reference release, changed constants, external
invalidation and enabled-specular precedence. Missing constant publication is
still rejected. These are ASan/UBSan checks, not hardware or live visual proof.

Pure patch `tools/pica_texture_swizzle_opacity_patch.py` stages the material owner
on top of `texture-channels-map-v1-builds`; it is already applied to current WIP.
Do not apply twice. The snapshot builders isolate and restore the six unrelated
VRAM WIP files and game archive. Private assets and saves are not modified.

An earlier cache investigation reused30 saved light-state snapshots. It found
no nonconsecutive recurrence of logical slot0, so a larger diffuse map cache
was not justified by those sparse observations and was not implemented. This
does not prove a larger cache can never help other scenes or unseen updates.

## GPU check and gameplay regression scope

`tools/texture_opacity_gpu_probe/` renders96 independent tiles through the actual
native descriptor compiler: all24 RGB selections, two mask channel types, and
ordinary versus LIGHT MAPS sampler layouts. Its256x384 target runs on an isolated
Old-profile Azahar with standard clock settings and no audio. After explicit
completion, diagnostic-only readback checks all interior pixels against source
equations, including alpha. Maximum error is0.53 of an8-bit channel level; RGB
and alpha images were inspected. The probe completes in2.27host seconds and
retires its resources; this is test duration, not a gameplay performance number.
Exact source/binary evidence is archived in
`artifacts/diagnostic/texture-opacity-gpu-v2/`. Environment-lit masked variants
still have host-only specific coverage; no hardware test is claimed.

The first probe used a768x128 target and completed but produced a striped
capture, failing at198.47 levels of error. Repacking the same96 draws into the
256x384 target passes. The larger target/readback failure's precise cause is
unresolved; it is not evidence of a gameplay material bug or a hardware limit.
The failing capture/report remains in `texture-opacity-gpu-v1`.

The actual New cave replay passes state/save/drain/owned-exit checks with2145
ordinary mapped channel draws, but **zero masked draws**. It is an unaffected
gameplay regression check, not masked-material visual acceptance. No duplicate
Old cave run was made for this uninformative coverage. Compared with the prior
mapped build: scene1.913100->1.927414ms, submit5.727784->5.727441ms; combined
CPU+0.013970ms, both59.83122AzaharFPS. Phases differ; no physical prediction.
Heap48,393,272/71,303,168B; minimum linear free18,513,536/25,165,824B; VRAM
free4,822,016B; stack73,008/524,288B; process103,231,488/130,023,424B. Measured
memory and program-pool peak43/128 are unchanged in this scene. Coverage across
additional masked assets may consume more shared programs and remains unmeasured.

The1410 gameplay image retains geometry, Fox and HUD, but **shows less cave haze
than the prior mapped run**. HEIGHT FOG is OFF in both; distance/fragment-fog
counters are similar, and the new masked family has zero draws. Causation is
not established. This repeats an earlier unresolved haze variation and prevents
claiming a clean full visual comparison. Keep this exact pair for investigation:
`texture-channels-map-v1-on-new/hid-1410.png` and
`texture-channels-opacity-v1-on-new/hid-1410.png` under `artifacts/native/`.

Shared builds: replay`45d6a97b1d65`/CIA`babceefa15d9`, normal`5af250cf5d6d`/
CIA`02d7224e3855`, built locally and not deployed. All344native sources and
the game archive match;511/214/304-instruction shaders are unchanged, and
CIAOld80/New124 profiles are verified. Unchanged normal UI evidence is reused;
no separate normal boot is claimed. Only the native channel header and material
owner differ from the preceding mapped build. The unrelated six VRAM WIP files
and archive were restored. Console installations/settings/saves are untouched.

Build/runtime evidence: `texture-channels-opacity-test/` under the rewrite
research folder, with host equations in `texture-swizzle-opacity-test/`.
Archived diffuse build files were losslessly NTFS-compressed with matching
SHA256 values to preserve disk headroom; no evidence/assets/saves were deleted.
