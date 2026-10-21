10 October: physical probe v3 completed, rejecting the padded morph loader in all12 comparisons; packed attributes matched all12. New223 normal913f01dfaa2a/replayd5f2e028e258 deployed/readback-verified with packed static lane; gameplay visual confirmation remains pending. See [MORPH_PACKED_FIX_20261010.md](MORPH_PACKED_FIX_20261010.md). Earlier padding acceptance statements below are superseded.

V3 deployment COMPLETE: New223, `8ab51c5ec8a2`, staged and final byte readback verified in `artifacts/console-morph-probe/20261010-144600`. Launch the same probe3dsx path; automatic, no A prompt. Game/saves/settings unchanged.

# Confirmed probe crash repair (v3)

New 3DS crash dump `crash_dump_00000192.dmp` identifies an ARM11 data abort
at PC `0x10418c`, fault address `0x0000000c`, unit=1 and texture=NULL. The exact
linked instruction dereferences `tex->param` inside `C3D_TexBind`. The probe log
stops after vertex-layout setup, before draw encoding. This was a probe CPU
fault, not evidence of a PICA shader failure. Collection and decoded finding:
`artifacts/console-morph-results/20261010-144158/{probe.log,analysis.json}`.

V3 (`8ab51c5ec8a2`) includes the same null-unit repair already used by the game.
The pinned Melee `native-3ds/port/3ds/citro3d_fix.c` explicitly documents this SDK
trap. The game and probe builders now share `tools/citro3d_null_texture.py`;
the game patch text remains identical. The isolated probe continues to use the
stock queue/uniform/draw machinery, plus only this texture-source repair.

`tools/test_arm_null_texture.py` executes linked ARM functions with page zero
unmapped. It reproduces the old probe's exact hardware PC/address for null units
1/2, and verifies 18 inactive/active null/2D/cube cases in both the corrected
probe and deployed game. The deployment gate requires this result for the exact
probe ELF. This catches the fault that Azahar's permissive null read missed.

Serial v3 New/Old headless runs pass all 36 GPU comparisons and exit. Their images
remain byte-identical to v2. This fixes the identified probe CPU bug locally;
physical probe completion and the game's morph/fur rendering remain unaccepted.

# Current status: hardware crash, not accepted

Probe v1 (`251564459bc4`) OS-crashed on New 3DS. Its collected log reaches
`BEGIN phase=0 fur=0 layout=0`: the ordinary interleaved reference. It never
records a GPU result, so this is not a failure attributed to split attributes.
Evidence: `artifacts/console-morph-results/20261010-142909/`. The latest numbered
ARM11 dump has PC `0x279804`, outside this small probe executable, and an older
file date. No matching exception dump was established. `/luma/errdisp.txt` is
absent. Do not ask the user to transcribe the screen in place of collecting
available system evidence.

V2 (`83adb34a03fb`) preserves the shader, layouts and comparisons. It separates
initial clear/presentation from drawing, matching the prior hardware-validated
particle probe. Durable checkpoints identify CPU encoding, GPU submission,
retirement, transfer and invalidation separately. Three initial command streams
are saved before submission; a timeout records busy/hotspot registers and retains
resources. This is improved fault isolation, **not an established crash fix**.
Serial New/Old headless runs each pass all 36 comparisons and exit; final images
are byte-identical to v1. Evidence: `artifacts/diagnostic/morph-input-v2-{new,old}`.
No console performance or visual repair claim follows from these checks.

`tools/collect_morph_gpu_probe.py` now collects bounded probe outputs, the latest
numbered ARM11 dump and an ErrDisp tail if present. It records missing system
logs, limits reads and never modifies game files, settings or saves.

The full linked ARM morph producer was also checked independently across 56
cases: 1/671/672/673/1345/4097/8191 vertices, four weights and separate/in-place
channel composition. 751,200 output bytes match the sparse fixed-point oracle.
This tests real cache chunk copies and producer code; it does not validate
physical cache coherence or GPU input fetching. Evidence:
`morph-full-producer-test/arm-stream.json` and `tools/test_arm_morph_stream.py`.

# Morph GPU input check

This diagnostic investigates the unresolved physical morph corruption reported
again after normal build `70b95ca077de`. Fur flicker is also still unresolved.
It is not a replacement game build or an FPS benchmark.

On New 3DS, launch `SD:/3ds/foxhollow-morph-probe/foxhollow-morph-probe.3dsx`
through Homebrew Launcher. It starts automatically, displays colored tiles,
compares GPU readbacks and exits. Reopen ftpd afterward. There is no A prompt.
If it explicitly reports a GPU timeout, it retains resources; recovery requires
a reboot. Normal completion should take seconds, not a gameplay session.

The same executable supports Old/New detection. No private game assets, saves,
settings or ordinary application files are read or written. Its results and
selected RGBA captures stay under its own SD directory.

The test uses the exact deployed 511-instruction actor shader, original compact
attribute formats, two-bone weights, 22 palette slots, ordinary/fur shader modes,
three rotating position slots and indexed triangles. Twelve updates each compare:

1. One interleaved reference stream containing the changed positions.
2. The actual production morph binder: 8-byte positions plus the resident
   40-byte stream with its leading padding component.
3. Separate 8-byte positions and a tightly packed 32-byte static attribute stream.

CPU interleaving is strictly the reference oracle in this diagnostic, never a
proposal to rebuild gameplay geometry each frame. GPU completion precedes writes,
readback, inspection and release. It adapts the pinned Melee split-stream binding
and resource retirement rules, and uses SDK display transfer for color readback.
There is no raw depth readback or new graphics worker.

Both serial headless New/Old checks passed 36 comparisons, with byte-identical
color results across layouts. Each ran in about 4.4 host seconds; these are not
console performance estimates. Captures show the expected colored tile grid.
Sources: `tools/{build,run}_morph_gpu_probe.py`, `tools/morph_gpu_probe/main.c`;
evidence: `artifacts/diagnostic/morph-input-v1-{new,old}/`.

This isolates vertex input fetching and shader use; it does not establish correct
game morph production, command handoffs from particles/HUD, the fur material,
fur surface depth, or cross-frame game ownership. Passing on hardware narrows
the investigation; it does not accept the reported visual bugs as fixed.

The latest gameplay collection is `console-visual/20261010-141722-223` (new log
SHA256 `08779b97e4008d1f3535b04f43de2766fddbe1a3edee57f17598aca75205bc1a`).
Its shutdown at frame 5196 encounters a pending memory-fill operation. Additional
bounded evidence is `20261010-141808-223`: the complete 24 MiB CPU view is checksum
valid, but the pending queue contains zero draw command lists. Thus the analyzer's
zero errors over zero draws is not a successful model validation. The cause of
this shutdown wait remains open.

Fur coverage-mask WIP was shelved under `fur-mask-wip` when the user confirmed
the regressions persist. It was never built or deployed. Main sources were
restored while preserving the unrelated six-file VRAM WIP.
