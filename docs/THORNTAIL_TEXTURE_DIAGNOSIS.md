# ThornTail resident texture diagnosis, 11 October 2026

Follow-up sampler audit: all2327 unique stored textures use linear within-level
filtering (889 min1/mag1,1438 min5/mag1); no wrapped NPOT axes were found.
The local native sampler helper fixes reversed citro3d min/mag arguments and
incorrect nearest-mipmap base-level filtering, but changes **none** of those
stored images. It is not a repair for the reported white Arwing or disappearing
trees. Dynamic/generated images are outside this inventory. Actual SDK setter
checks cover147456 dirty-state/filter/call-site cases, with reversed-order and
all-mip-linear mutants rejected under sanitizers. See
`sampler-filter-test/{result,source-inventory}.json` in the research directory.
The correction is local/unbuilt; reduced-resolution and mip-level selection
remain separate issues. `tools/pica_sampler_filter_patch.py` is already applied
to the worktree but must be applied once to the two old stable snapshot files
inside the next isolated builder.

Follow-up integration: the correction is now included in local diffuse-map-v3
normal/replay builds. It is already present in their stable snapshots, so do
not apply that patch twice. A source review also ruled out the proposed SDK
pointer-equality shortcut: pinned `C3D_TexBind` marks the unit dirty on every
call, and the retained-raster adaptation deliberately leaves texture binding
and texture-cache clear commands unchanged. This is source evidence against
that particular hypothesis, not physical proof that all coherency is correct.
No speculative extra per-draw cache flush was added.

This investigation does **not** fix the reported visibility or white-Arwing
defects. It narrows the cause without changing the game, assets or console.

The exact local replay binary `974089bcdd56` was run silently in isolated
headless Azahar, using original geometry and full-resolution textures.
`tools/capture_native_images.py` reads the native image pool, model materials,
sampler descriptors and GPU-visible linear storage through the owned process.
Layouts come from that binary's DWARF. Existing executable anchors, process
start time, live retrace and normal-heap checks remain mandatory. The optional
linear reader independently identifies the immutable specular mesh using all
1,536 indices and its exact first vertex, checks the actual linear allocation
bounds and requires a unique mapping. It never writes or pauses the guest.
Subsequent samples revalidate the anchors instead of repeating the memory scan.

The completed late sample is at retrace 1968 in
`artifacts/native/thorntail-images-v4-new/native-image-capture/report.json`.
Every captured image and the corresponding ownership/material descriptors were
read twice and required to remain stable. This is evidence of live resident
state, **not** a capture of every PICA register at a particular draw.

* Fern model238 material0 binds image269, a 256x512 RGB5551 allocation with
  original source dimensions196x512, UV scale(0.765625,1), sampler flags518,
  constant alpha255, no environment/specular program, and alpha-test threshold64.
  Its 262,144 GPU-visible bytes match an independent conversion of original
  texture1965 exactly. Alpha errors: zero; maximum RGB quantization error5/255.
  The decoded image was inspected: foliage, transparent areas and bark intact.
* Landed Arwing model1152 material0 binds image321, full512x512 ETC1,
  UV scale(1,1), sampler flags8742, constant alpha255, diffuse environment on
  and no admitted specular program for its auxiliary-bump family.
  Its 131,072 GPU-visible bytes exactly match the prepared pack at offset1655648
  (SHA256 `12c0c8adc6069fa181994a02808ff84ca3207e713e6edbd5c06550f524a3d5d1`).
  Decoding and comparing original texture5 gives RGB RMSE4.70744/255 and zero
  alpha errors. Compression is lossy; this is not original-byte texture parity.
  The image was inspected: blue paint, cockpit, markings and panel details
  are present. It is not a blank white resident image.
* Arwing material1 binds image322,16x16RGBA8, with its separate admitted specular
  program. This capture does not prove that its final glass/lighting equation
  matches the original, nor that every draw binds the intended sampler.

The source/native comparisons are reproducible with
`tools/compare_thorntail_images.py <run-directory>`; private decoded PNGs and
`comparison.json` remain alongside the captures. No assets are published.

The initial attempt failed on the host DWARF parser's bit-field representation;
it did not crash the game. The parser now handles anonymous SDK unions and
`DW_AT_data_bit_offset`, scopes duplicate symbol names to their source owner,
and supports an offline layout check. A subsequent early sample at retrace964
caught tree load state before material publication and before the Arwing loaded;
its zero constant must **not** be interpreted as an active-draw bug. The final
sample waits beyond retrace1250 and captures both models after publication.

All four diagnostic runs completed guest log drain, save preservation and owned
exit. The first two use idle input; the last two use ordinary scripted movement
through the arch. The movement capture at retrace1500 shows plants and Fox,
but does not provide a close view of the Arwing. Every run has a
`diagnostic-scope.json` excluding it from timing comparison. There is no FPS or
console correctness claim here, and no new app build or deployment this turn.

Next work should compare the final native draw state and projected geometry
against the original rendering, including source material families. Replacing
these two resident images or repeating geometry restoration is not supported
by this evidence. Physical-only cache/binding failures are not ruled out by
correct Azahar memory. The broader flickering, lighting, fur and SD-stall reports
remain open.


## 11 October final native draw observation

Added opt-in `--native-draw-capture` to the isolated headless runner. Its pinned
custom Azahar observes commands before accelerated draw submission; it leaves
hardware shaders and game state enabled/unchanged. It records registers, exact
resident vertex program/swizzles, live uniforms and decoded indexed inputs. The
capture is bounded to64 image/buffer/entry identities and512 vertices per draw,
starting at retrace1250. It is diagnostic evidence, never an FPS comparison.
The separate `inspect_native_draw_capture.py` executes those actual captured
inputs in the pinned float24 interpreter without another game boot.

`thorntail-native-draw-v2-new` uses the unchanged974089bcdd56 replay, original
assets and the ordinary movement route. Three captures at retrace1250 contain
512/648 fern vertices and512/10782 plus512/576 Arwing vertices. The physical
texture addresses match the prior linear images after virtual-to-physical
translation. All1536 sampled vertices produce finite position/colour/UV outputs;
base UVs match the captured input and scale/offset exactly (zero sampled error).
Fern vertex alpha is1, RGB approximately(.910,.929,.996), with texture*primary
combining. Arwing base UVs span the expected image; its actual combiner still
multiplies texture0 by the fragment-light contribution. Its primary vertex RGB
is0 and environment lighting is enabled, so primary black alone is not evidence
of a black/white output. Fragment lighting/rasterization is not evaluated by
this offline vertex check.

The replay completes with state checks, preserved saves, drained logs and owned
exit. Images1200/1500 inspected show Fox, foliage, terrain and HUD; neither is
a close Arwing view. This samples one state per persistent draw identity, not
every instance/pose, the complete mesh or a physical GPU cache. No claim that
reported flicker, white Arwing or invisible trees is fixed. No app code/build,
asset or remote installation changed in this step. Current local shadow repair
and installed SD-reader versions remain as recorded above.

Failure records retained: the initial observer compile used private shader
fields and an invalid bit-field conversion; corrected to public getters and
typed enum comparison. `failed-v1/` retains the build failure, and all source/
binary restoration flags pass. The first recipe accidentally omitted original
assets due to nested source replacement and failed before gameplay; retained
as`thorntail-native-draw-v1-new`. The corrected direct recipe explicitly supplies
original assets and the diagnostic flag. Initial offline inspector header/type
compile errors were host-only; corrected to the pinned PicaCore::Regs definition.

Fur investigation reconfirms that two-shell reduction, missing original indirect
wind coordinates and unsupported projected/layered families remain substantial
fidelity gaps. Sampled texture/UV success must not be reported as fur acceptance.


## Ordinary-input close approach and actual console options

`thorntail-arwing-close-v3-new` centers the camera using ordinary L input, then
moves toward the ship with stick(26,90). No position/camera writes or altered
saves. Capture1650 shows close grey panels and blue Arwing sections, tree, Fox,
world and HUD. Earlier v1 hit a wall and v2 remained distant; retained as route
failures, not missing-texture reproductions. All completed shutdown and saves.
The cached candidate `original-cache-v1-arwing-new` retains that visible output.
Neither reproduces the physical white/missing surface report.

After the completed New deployment, bounded read-only FTP collection of both
32-byte graphics slots found valid FHGRAPH4 checksums. Newest slot0 serial4
uses mask3866625, display7, original geometry1/textures1; slot1 is serial3.
Compared with the prior route mask3211265, splashes and water droplets are also
enabled. Textures, geometry, lighting and morphs are enabled in the saved state.
Evidence: console-settings-update/20261010-165737-223/graphics-settings.json.
A matching-mask check is warranted; settings were not changed remotely.

`thorntail-console-options-v2-new` completed in66.80hostseconds with actual
graphics mask3866625, original assets, state/save/drain/owned-exit checks passing.
Inspected1650: tree and blue/panelled Arwing remain visible, Fox/HUD intact.
49 splash objects/98 draws exercised; scene SD reads remain0. This still does
not reproduce the physical defect or rule out other camera/time/material states.
The first v1 recipe accidentally retained mask3211265 because its .fhr suffix
was excluded by the text update. Cancelled and excluded; Windows wrapper
interruption left emulator/Xvfb children, subsequently terminated only after
checking their exact owned command lines. V2 recipe mask was checked before
launch. No additional console changes from this diagnostic.
