Old 3DS save loading, 16 October

The physical run of `a6184b997ac1` gets past the previous startup hang, but
loading the user's ThornTail Hollow save reaches `PICA actor base image` panic.
The fresh startup receipt identifies that build. At retrace 1147, a prepared
4,096-byte texture (source 64x512 CMPR, native 64x128 ETC1) is rejected with
1,563,904 bytes free out of 10 MiB linear RAM. This is the 15% admission guard,
not a missing asset, broken conversion, or failed 4 MiB texture-pool limit.

Read-only physical evidence:
`artifacts/console-test/20261016-122719-old229-startup-direct/`.
The active GCI and both graphics settings were double-read and checked;
the backup GCI read failed. No remote save or setting was modified. The active
fixture is sufficient to reproduce; do not claim collection of the backup.

The same installed binary, exact save and settings reproduce the identical
texture failure in isolated Old80 Azahar, including free bytes, slot 324, and
1,678,368 resident texture bytes. The earlier passing test loaded a cave save,
not this save. Contrary to an intermediate hypothesis, both the earlier
emulator run and physical run used the same 48+10 MiB heap selection.

The selected allocation depends on the measured process grant and current
commit. The shipping executable has 8,777,728 bytes committed before heaps;
48+12 MiB exceeds the 85% process ceiling, causing the previous 48+10 fallback.
The repair replaces the 48+12 tier with 47+12 and removes its obsolete 512 KiB
normal/linear repartition. Total reservation is 59 MiB. New124 selection,
Old80 entitlement, SDK mapping ownership, allocator synchronization and all
15% admission reserves remain intact. No textures, effects or simulation are
removed. Pinned Melee linear allocator locking, audio shutdown ownership and
early-queue resource lifetime rules were consulted; none are replaced.

Candidate `7d87ef925cd7` changes only `fh_heap_profile.h` and
`platform_hardware.c`. The isolated builder restores root WIP and game archive.
All 18 GPU programs, game archive, prepared-asset recipe and shared Old80/New124
CIA remain unchanged. No conversion rerun is needed.

Actual archived C selector: 1,673,793 boundary cases under ASan/UBSan, invalid
inputs, physical commit/texture-admission regression, unchanged New tier;
reverting the selector fails the physical regression.

Candidate normal app loads that save into ThornTail Hollow, renders Fox,
Arwing, trees, the EarthWalker and HUD, and exits via Select. Loaded captures
1601/1750 inspected; later bottom screen complete, earlier asynchronous capture
intersects its update. 168,754 audited draws, zero layout faults; native DSP
LLE active; log drain and save preservation pass. Host elapsed 77.20 s is
diagnostic throughput, not hardware FPS. No FPS gain claim.

Sampled memory: normal peak 40,588,152 / 49,283,072 bytes; linear free minimum
2,717,824 / 12,582,912; initialized VRAM free 4,723,712; main stack upper
bound 73,044 / 524,288; process commit 70,643,712 / 83,886,080. All measured
reserves pass, including conservative full main-stack allowance. This short
load regression is not an extended memory/endurance proof.

Retained OS dump 00000005 matches the installed ELF fault window inside
`ndspiReadChnState`, called by the audio update thread. Its stack page is
unmapped. Abrupt `exit(1)` after the logged panic is consistent with teardown
while audio is alive, but the dump is not dated and this secondary crash has
not been independently reproduced. Do not call this an audio engine repair.
The verified change removes the reproducible resource-admission failure.

Reproduce: `test_save_load_heap.py`, `build_save_load_heap.py console-setup
--revision v1`, `run_save_load_regression.py` (old binary, expected failure),
`run_save_load_regression.py --candidate` (pass), `verify_save_load_heap.py`.
Recorded evidence lives in `save-load-heap-test/` and
`artifacts/native/unified-{packet-inline,save-heap}-v1-play-old-dsp-actual-save/`.
The normal installation package is `foxhollow-iso-setup-2b2c1fa7161b82e5.zip`;
consult the transfer record for actual deployment status. HOME icon updates
require installing `SD:/3ds/foxhollow/foxhollow.cia` using FBI.

Deployment COMPLETE: `20261016-203756-old229-direct-install`; all six staged and final files
byte-verified. Old229 only; source `7d87ef925cd7`. Hardware confirmation
is pending. No assets, saves or settings replaced.
