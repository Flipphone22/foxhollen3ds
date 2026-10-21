# Shadow GPU isolation probe

Launch `SD:/3ds/foxhollow-shadow-probe/foxhollow-shadow-probe.3dsx` through the
Homebrew Launcher. V3 runs fifteen cases automatically, saves results and exits.
No buttons or game save are needed. Reopen ftpd afterward for collection.
If it explicitly reports a GPU timeout, reboot: it retains GPU-owned resources
instead of freeing memory while commands could still be using it.

This is a diagnostic, not a repaired game build or a performance benchmark.
The top screen is not a gameplay preview; the bottom screen reports progress.
The original game, assets, saves and graphics preferences are untouched.

## Cases and scope

0. Solid receiver geometry without depth rejection.
1. Known CPU-prepared shadow texture, no depth rejection.
2. Same texture with the current GEQUAL depth state and small bias.
3. Same texture with the former exact EQUAL depth state.
4. GPU-produced shadow image, no depth rejection.
5. GPU-produced image with the current depth state.
6. Known texture with texture-coordinate Q explicitly one.
7. GPU-produced image with Q explicitly one.
8. Original authored-mask intensity mode.
9–11. CPU texture with the game's actual fog owner: near, half transmission,
and far.
12–14. GPU-produced image with the same three fog states.

The probe extracts the deployed 65-instruction shader and material binding and
uses the deployed image owner and receiver code. The Q-one diagnostic changes
only its output-coordinate assignment. Geometry and matrices are simple fixed
fixtures. The first nine cases disable fog; V3 additionally links the deployed
fog owner, LUTs and procedural sampler for six controlled fog cases. It does not
test skinned caster capture, actual cave transforms/receiver policies, or prove
complete cave rendering correctness.

Images are 64x64 colour readbacks, never raw depth readbacks. GPU completion is
bounded and texture-cache ownership follows the existing SDK/image owner.
The linked null-sampler regression checks 18 cases with page zero unmapped.

## Local evidence

V2 binary SHA256 `183528216b185e71314f299f98c904f2121f4ffa160e3504ab7ba23071fc8b58`;
source ID `200e5ae6527a552a`. Exact-binary headless validation completed in
5.024 host seconds and exited normally. This is not a console runtime estimate.
Cases1..7 produce centered masks; the32px CPU fixture has wider bilinear edges
than the128px generated image at64px output. The analytic pixel oracle accounts
for this filtering and tests every pixel, including clear borders. The first
hard-edged oracle failed; its report is retained and the same completed run was
reanalysed with the corrected bilinear oracle, without rerunning the emulator.
All nine cases match within3 byte levels per channel (mean error at most0.059).
Captures0/1/4/8 were inspected.

## Hardware V2 result and V3 fog extension

`console-shadow-probe/20261011-150409-results` matches V2 and completed all nine
cases normally. Every capture meets the pixel oracle (no channel error over3;
maximum mean error0.032), including the solid control and GPU image/depth cases.
The newest crash dump is byte-identical to the previous collection. These simple
controls pass on hardware; the missing cave shadows remain unfixed.

V3 SHA256 `3d55adefeeb542b6139efb765ce5675be9970b1e7bc6e96a6a6bfc02355f2aa1`,
source `8afe9275f8ee2ff1`, uses the unchanged deployed65-instruction shader and
adds the actual deployed fog implementation. Six extra cases check original
exponential transmission1,1/2,1/256 using both image sources. Exact-binary
headless validation passed all15 cases in4.514host seconds, owned process exited,
and partial/far images were inspected. Hardware V3 is pending. This is a targeted
test of whether the procedural fog stage erases shadows, not a gameplay fix.

Hardware V3 subsequently **passed all15 pixel controls** in
`console-shadow-probe/20261011-152157-results`, matching source8afe9275f8ee2ff1.
Near/half mask centres are65/160; far is255 versus254 in Azahar, within the
three-byte tolerance. The half-fog image was inspected. The missing cave shadow
is therefore not explained by these simple mask/depth/fog controls. No new probe
or repaired gameplay build was deployed after this result.

The next investigation captures actual cave commands instead of adding another
simple fixture: `native/shadow-live-draws-v1-new` contains16 receiver draws,
6820 sampled vertices, live matrices,65-instruction shader identity and fog LUTs.
This uses the installed normal binary4ff085bfe304 in a silent isolated emulator.
Save hashes/drain/owned exit pass; image1360 was inspected. Captures are bounded
to512 vertices/draw and currently exclude caster draws, authored masks and GPU
texture/depth pixels. These are diagnostic data, not performance evidence or a
physical correctness claim.

## V1 invalid capture, retained as regression evidence

Physical run `console-shadow-probe/20261011-145135-results` completed normally,
but its32px-wide DisplayTransfer violates the documented64px minimum. Even the
solid control contains a black patch; shifted masks cannot establish a gameplay
shader or depth fault. Azahar accepted this unsupported transfer. V2 uses64x64
and a compile-time minimum/alignment guard. The collector now distinguishes
successful collection from valid capture dimensions and pixel-oracle success.
See [transfer constraints](https://www.3dbrew.org/wiki/GPU/External_Registers#Tiled_to_linear).
The actual gameplay shadow shader/image owner is unchanged by this probe repair.

Build: `tools/build_shadow_gpu_probe.py --fog` in the Linux SDK environment.
Check: `tools/run_shadow_gpu_probe.py --fog --label shadow-fog-v3-new`
(isolated, silent, bounded Azahar).
Deploy: `tools/install_shadow_gpu_probe.py --fog --host new223`.
Collect: `tools/collect_shadow_gpu_probe.py --host new223`.
Evidence: `artifacts/diagnostic/shadow-isolation-v2-new/validation.json`.

11 October follow-up: the separate [actual cave workload](SHADOW_CAVE_PROBE.md)
is now installed at `SD:/3ds/foxhollow-shadow-cave-probe/foxhollow-shadow-cave-probe.3dsx`.
It uses captured skinned meshes, receiver geometry, live matrices and world-shader
handoff rather than square controls. Ten cases pass locally; hardware acceptance
is pending. This V3 probe and the normal game installation remain unchanged.
