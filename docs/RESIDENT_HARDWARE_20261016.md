16 October resident command replay validation

Old229 transfer artifacts/console-test-apps/20261015-233614 COMPLETE. Eight
payloads fully compared while staged, renamed and final sizes checked. Normal
saves match before/after. CIA awaits FBI installation. New223 unchanged.

The shared replay candidate is source `88d497806a81`, 3dsx `e5263d06d4a3`,
CIA `0546e94ed973`. It uses the established software audio backend, not the
experimental DSP voice/effect integration. Old80/New124 CIA profiles remain.
No physical performance improvement has been established.

This candidate combines the accepted actor-stream-v6 resident actor/world
command publishers with sparse fetch-register metadata compiled at asset load.
Per draw, the publisher checks the union of old/new potentially nonzero fetch
registers and retains only changed addresses. Live override arrays still check
all twelve registers. There are no new GPU commands, resource copies, fences,
cache flushes, shaders, threads, texture buffers or passes from this change.
Pinned Melee command_cache.c publication/lifetime rules were consulted; SDK
ownership and explicit resource publication are retained. Static descriptor
growth is four bytes per descriptor/state. This is not a claimed large FPS win.

The candidate also carries both already-deployed crash repairs: final sample
release retires its actual software readers; post-capture shaders initialize
all UV output components. The exact repaired post program matches the deployed
normal build. The other 17 GPU programs match actor-stream-v6. The original
game archive is unchanged. The isolated builder restores root WIP and archive.

Reproduce with tools/build_resident_hardware.py console-replay --revision v1
(preserve the existing v1 archive; choose another revision for a fresh build).
tools/verify_resident_hardware.py verifies source reconstruction, command-test
hashes, generated sample registry, linked shader bytes and memory profiles.
tools/resident_hardware_bench.py records the two fixtures and validates the
actual CIA content under isolated, silent, headless Old Azahar.

The existing Old229 replay manifest and local asset hashes match: model9a795b4d,
scene f9483195, texture760f3b09. No conversion or new asset payload is needed.
Existing asset references are checked against the installed manifest and sizes;
that is not a fresh byte hash of all assets. The new app/cases receive staged
byte verification before activation. Normal installation and saves stay intact.

Checks completed:

- Actual SDK owner fixtures: world92683 and actor274821 checks; missing-old-
  register clearing mutant rejected. Capacity rejection, overrides, inactive
  high registers and state handoffs covered. Not physical GPU execution.
- Palace: 58.56 host seconds, replay status1, saves unchanged, logs drained,
  owned process exited. Capture1410 inspected: Krystal, central room, purple
  creatures, textures and HUD present. Resident actor37888/world54725 draws.
- Cave: 82.55 host seconds, same lifecycle checks. Capture1410 inspected: Fox,
  ladder, mushrooms, tunnel, textures, mist, HUD and bottom controls present.
  Resident actor98601/world86215 draws. Material errors0 in both runs.
- Sampled normal RAM reserve after main stack: palace16.91%, cave17.07%; linear
 42.41%/47.14%; process16.63% both. This is route coverage, not endurance proof.

Hardware procedure once the transfer is complete: install
`SD:/3ds/foxhollow-replay/foxhollow-replay.cia` using FBI. Launch Foxhollow Replay
and confirm source88d497806a81. Run `00-resident-palace`, then relaunch and run
`01-resident-cave`. Use PICA ACTORS+HUD, MAIN, texture VRAM OFF, GPU profiling
OFF. Each case automatically sets Old Fast graphics (lighting/environment/
specular/particles on, ground shadows and optional effects off) and exits.
Reopen ftpd after both. Do not compare these stationary benchmark results
directly to the unrelated 11.22 FPS roaming gameplay run.

Separate, NOT DEPLOYABLE audio branch

Combined sourcefd24eb359d23 passed a short ThornTail route and a recorded
3000-tick opening prefix (185.87 host seconds, status1, saves/drain/owned exit).
Opening captures1200/2500 inspected: name-entry cockpit and story text/clouds.
This does not cover the whole opening fight/progression. Its latest raw guest
fullframe18.6615->18.2059ms (+2.503% FPS) is a diagnostic comparison with timing
exclusions, not a causal or console speedup, nor a comparison to installed2fba.

New root-only raw AUX transport supports monotonically increasing source
generations while preserving older queued samples. Complete returned blocks
preflight by queue position, not ambiguous source clocks that can restart.
Storage unchanged:81920Bstereo/1064Bqueue metadata.3.84M threaded source frames,
serial wrap, overlapping clocks, old-sample retention and rollback rejection
pass ASan/UBSan. Shared original-reverb comparisons pass1536returned bus blocks
with resets and1536without; four fault mutants reject in each mode. Strict
ARM11 compile passes. These are component proofs, not game integration.
audio_generation_patch.py prepares the producer handoff; its builder still
requires a missing integration test result and has NOT been invoked. Do not
deploy this branch or remove pending/reset diagnostics speculatively. Wet/dry
delay, backpressure, native depop and auditory parity remain unresolved.

Full visual parity, playable sustained performance and final progression/save/
endurance requirements remain incomplete. User-accepted shadow flicker stays
accepted; no new hardware probe was requested for that symptom.

The timing analyzer now recognizes the exact archived extended schema2 fields
(present_prep/copy/publish/retrace_callbacks). Every interval still must sum
exactly. Both routes pass; renamed/missing/double-counted field mutations reject.
Evidence: resident-hardware-v1-console-replay-builds/timing-layout-test.json.
Guest frame means palace16.7274ms/cave16.7286ms are paced emulator observations,
not physical predictions or a demonstrated gain over the installed normal app.
# Superseded deployment instructions

The user rejected the separate replay application. Do **not** install its CIA
or use the separate-launcher instructions below. The upload and test evidence
remain historical records. Benchmarks are to run from the existing Foxhollow
startup menu, sharing the normal application's renderer and executable.
