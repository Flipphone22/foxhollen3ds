# Actual cave shadow workload

12 October candidate `339af249fd11`, source `f9c14ed9c2450b5b`, replaces the
combined shadow shader with three independent entries in one shared code pool.
The caster is the unchanged 43-instruction UI skinning program. No CPU skinning,
extra geometry or GPU allocation is introduced. It passes all ten cases in
Azahar. Physical collection `20261011-173947-results` also repairs the missing
projection and matches the independent UI silhouette exactly. The strict offline
pixel gate remains failed (231 edge pixels); see its `acceptance-review.json`. The same path and automatic procedure
below apply. This is an updated diagnostic, not an updated normal game build.

The collected previous cave run (`20261011-172850-results`) completed normally
but reproduced bad live skinning and six completely blank projection images.
The separate UI shader produced the expected silhouette on that same hardware.
Depth/fog controls exclude either as the sole cause. The exact hardware
instruction/state failure is not yet established; nested mode selection is
removed rather than treating emulator success as hardware proof.

The earlier V3 hardware probe passes all fifteen simple mask/depth/fog controls.
That does **not** establish a fix for the missing game shadows. Its silhouette
was a square; it did not execute the game's skinned caster or large cave transforms.

This separate private diagnostic replays three actual caster draws (10,278
indices) and ten complete receiver draws captured from installed normal
`4ff085bfe304` in the cave. The previous 65-instruction shadow program, 511-instruction
world program, 43-instruction UI program, material and image owner come from that
installed build. The new candidate replaces only the shadow program/entry owner.
Geometry, palette, camera, projector and fog tables are retained.
Offline float24 interpreter positions provide an independent rasterization
control; they are **not** a proposed CPU skinning path for the game.

Run through Homebrew Launcher:

`SD:/3ds/foxhollow-shadow-cave-probe/foxhollow-shadow-cave-probe.3dsx`

It runs ten cases automatically, records color images, and exits. No button,
save loading, FUR setting or reboot is needed. Reopen ftpd afterward. This is a
different entry from the earlier `foxhollow-shadow-probe`. It does not modify
game files, saves or graphics settings. No CIA installation is required.

| Case | Test |
|---|---|
| 0 | Actual PICA-skinned silhouette |
| 1 | Reference silhouette from offline shader outputs |
| 2 | Actual silhouette projected onto cave geometry, depth disabled |
| 3 | Reference silhouette projected onto the same geometry |
| 4 | Actual projection against depth written by the receiver program |
| 5 | Actual projection against depth written by the installed world program |
| 6 | Reference silhouette against the same world depth |
| 7 | Case 5 with distance fog disabled |
| 8 | Actual silhouette immediately after the full world program |
| 9 | Same handoff using the physically accepted UI skinning program |

Local validation `shadow-cave-v3-new` completed in 5.71 host seconds. Every paired
image is byte-identical; depth-tested projections cover 702 pixels. Inspected
images contain the full silhouette and a projected cave shadow. This is **not**
hardware acceptance or a performance benchmark.

The main target is 256x512 RGBA8/D24S8 with the game's 240x400 viewport. The
128x128 RGB565 shadow image owner is unchanged. Only retired color buffers are
read back; no raw depth reads or raw register writes. Immutable geometry is
published once and retained through completion. A four-second GPU timeout keeps
in-flight resources alive and reports failure. Reboot only if that timeout occurs.
The diagnostic reserves 8 MiB normal heap, 4 MiB linear heap and 512 KiB stack;
observed free linear/VRAM are 1,179,520/5,144,576 bytes. The patched SDK binder
passes 18 actual linked ARM null-sampler checks before installation.

Pinned Melee `renderer.c` queue/explicit cache ownership and `citro3d_fix.c`
null-sampler behavior were consulted. The existing native image owner is reused;
no new gameplay worker, copy, allocation or shader is introduced.

Limits: this isolates one caster and ten receiving draws, not all gameplay
resource traffic. Captured inputs came from Azahar, not console GPU memory. The
opaque depth prepass reproduces positions/transforms, but not original cutout
material coverage; culling is disabled consistently for these controls. If all
cases pass on hardware, investigate live game ownership/state without claiming
this proves the game fixed.

Failure records are retained. Local v1 used reversed target dimensions and its
padding guard rejected the output. V2 corrected dimensions; its first analysis
incorrectly expected transfer padding below rather than above the reversed-Y
image. Corrected analysis of the same saved bytes passes without another boot.
V3 adds the two actual shader-handoff cases and passes all checks. These were
local failures, never deployed.

12 October collection: the latest console run executed the earlier fifteen-case
V3 entry again (source `8afe9275f8ee2ff1`); all its controls passed. The cave
folder contained its verified executable and metadata but no log or images.
To remove this ambiguity, installation now retires the old selectable launcher
with verified local/remote backups. The cave binary is unchanged and fully
readback-verified (`console-shadow-cave-probe/20261011-161749/transfer.json`).
Game files, saves and the old diagnostic outputs are preserved. Hardware cave
acceptance remains pending; this is still the fourth diagnostic.
