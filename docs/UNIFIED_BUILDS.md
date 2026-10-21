# Unified Old and New 3DS builds

Foxhollow must have one `.3dsx` and one `.cia`, both supporting Old and New
3DS. Play, benchmarks and playback belong in its existing startup menu and
must use the same compiled ELF and renderer. Do not deploy a second replay
launcher or another HOME icon. The 16 October separate replay upload was
rejected by the user; its installation instructions are superseded.

The current resident renderer integration is built in isolation with
`tools/build_unified_resident.py console-setup --revision <unique-revision>`.
`build_unified_startup.py` preserves the earlier menu-only integration.
It retains the deployed normal app's conversion recipe and CIA identity.
Do not blind-build the unrelated root work in progress. Menu integration
alone is not a rendering speedup; check the selected build's runtime acceptance before
deployment. Benchmarks use normal prepared assets and private per-run saves.

`tools/build_native.py` produces both formats by default. Use
`--on-device-setup` on the integrated snapshot for the shipping application.
The historical `--replay-console` variant is for local regression evidence only.
`tools/build_native_matrix.py` builds game, replay and emulator variants and
checks their hashes. Run these Linux tools through `tools/linux_exec.sh`.
The matrix and packaging steps reject incomplete format pairs. Legacy
`--old-3ds` build flags are rejected; select hardware in the test runner instead.

`tools/package_native_cia.py` bootstraps checksum-pinned makerom and bannertool,
uses the pinned Melee launch-logo verification, and verifies the container,
code, icon, banner and access profile. Code segments must match the ELF used
for the 3DSX. Generated artwork contains no game assets.

The CIA requests 80 MiB on Old 3DS and 124 MiB on New 3DS. These are requests,
not measurements. Startup queries the actual kernel memory entitlement before
allocating heaps. With sufficient headroom it chooses normal/linear heaps of
48/10 MiB for an Old 80 MiB grant or 68/24 MiB for New. A 52/16 MiB profile remains available if another launch path grants 96 MiB; the existing 46/6 MiB fallback supports the
observed 64 MiB Old Homebrew Launcher grant. Expanded profiles reserve at least
15% of process memory including existing code/commit. Pool reserves still apply.
The 64 MiB fallback is tight at process level and is not a 15% total-headroom claim.
Runtime hardware detection controls CPU features, worker cores and controls;
extra RAM never enables New-only cores on Old hardware.

For Homebrew Launcher, launch `foxhollow.3dsx`.
For HOME Menu, install the matching CIA with FBI, then launch its HOME Menu
icon. FTP only copies the CIA to SD; it cannot install the title. Foxhollow's
title ID remains `000400000F484C00`. The old separate replay title
`000400000F484D00` is retired, not the update target.
The CIA remains beside the 3DSX under `/3ds/foxhollow/`.
Both formats use the existing private SD assets and replay fixtures; normal
saves are preserved. Do not distribute packages containing fixtures/assets.

`tools/run_console_headless.py --hardware old|new --format cia` boots the CXI
content verified byte-for-byte against the CIA. This covers executable startup
and replay, not physical HOME Menu installation or extended-memory reboot.
`--format 3dsx` remains the default. Emulator grants/timings may differ from
physical hardware; record `memory.jsonl` and compare matching replay scenes.

An extended RAM CIA does not itself establish an FPS gain. The first physical
CIA run must verify the actual grant, successful exit and the same palace-facing
WORLD TEXTURES replay with GPU profiling OFF. Models/effects remain incomplete.

Repacking the same ELF produced identical CXI content. Makerom generates random
ticket/content IDs in the CIA envelope, so whole-CIA hashes can differ between
rebuilds; every shipped package records its exact hash. This is reproducibility
of executable content, not a claim of byte-identical CIA envelopes.

The first 96 MiB CIA failed before the replay menu on physical Old 3DS; its
cause remains unconfirmed. Mode 2 is a documented 96 MiB request, not an invalid
format. The validator accepts it and checks that each build encodes its requested
profile. The local 80 MiB candidate leaves more RAM for system software, but is
not a proven crash fix. After the separate 96 MiB probe also crashed as a CIA
while its 3DSX worked, the user selected the 80 MiB target. Validate and deploy
the shared Old80/New124 replay pair; no lower automatic CIA request is selected.
Headless CXI boot does not exercise
HOME Menu's memory-mode transition and did not catch the physical failure.
See [defined memory modes](https://www.3dbrew.org/wiki/NCCH/Extended_Header#Flag0).
References: [Xash3DS RSF](https://github.com/masterfeizz/Xash3DS/blob/master/cia.rsf),
[Forsaken's current RSF](https://github.com/colbyshores/forsaken-3ds/blob/master/assets/forsaken.rsf)
explicitly documents Old HOME Menu crashes with 96 MiB.
