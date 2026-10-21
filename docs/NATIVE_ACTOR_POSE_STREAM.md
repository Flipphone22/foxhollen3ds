# Contiguous live bone command publication

This candidate removes temporary row arrays and per-word command-header-gap
selection from the native actor palette writer. It does not restore the rejected
per-invocation pose cache. There is no new pose table, heap/linear/VRAM allocation,
worker, shader, geometry conversion or GPU command.

The existing publisher validates the complete destination capacity first. It
then writes the ordered matrix values contiguously into its unpublished CPU
command range. Finally it places data0 before the saved SDK header. No callback
or GPU publication occurs during this temporary header overwrite. Float32
arithmetic order, bone/world selection, dirty rows, chunk boundaries, padding
and final command bytes are unchanged.

Pinned Melee uniform_upload.h was consulted for float-word/cache semantics;
existing Foxhollow command ownership and cache/fence handoffs remain intact.
CPU tail-offset math remains necessary for the current compact skinning layout;
this change only removes redundant packing around it.

`tools/actor_pose_stream_patch.py` applies to archived shared source610d4b418e6d,
changing only pica_actor_commands.c. Root WIP is not an input. The new whole-app
builder is `build_actor_pose_stream.py console-setup --revision v1`; the bounded
runtime wrapper is run_actor_pose_stream.py (family unified-actor-stream).
Shared application source **e25b518c7339** is now built and packaged as
`foxhollow-iso-setup-70d2ece34b572017.zip`. Its 3dsx is63ccc72ca652 and CIA04021278dcac.
The exact Old80 binary completed the palace replay with native DSP audio, state,
save/settings preservation, drain and owned-exit checks. Capture1601 was inspected:
world textures, Krystal, purple creatures, particles and HUD remain present.
This is limited scene acceptance, not full-game fidelity or physical validation.

The paired profile contains280 actor publication calls in both runs. Their ARM
instruction encounters fall624312->477944 (23.44%). Raw whole-window work falls
23224461->21929498, but the parent had120 lightning calls while the candidate had
none; the whole difference cannot be attributed to this writer. Timing is
excluded because of instruction profiling, DSP LLE and capture overhead.

All18 GPU programs, the game archive, Old80/New124 CIA profiles and setup recipe
are unchanged. Root/archive restoration and package checks pass. The prior New
610d4b runtime supplies explicitly reused GPU evidence; this is not a fresh New
candidate test. Old observed normal heap41859104/50331648, linear free1622784/
10485760, VRAM free4723712, stack72984/524288, process69595136/83886080 are unchanged
from the parent. No new allocation is introduced.

The package is **not deployed**. The bounded Old229 FTP check on16 October at
08:15:11UTC timed out; no remote files changed. Evidence:
`actor-pose-stream-test/{integration,ftp-availability}.json` and
`artifacts/native/unified-actor-stream-v1-benchmark-old-dsp-profile-v2/`.

The actual ARM parent/candidate test passes 2,304 checks over 576 publication
cases plus changed-value repeats. It compares complete command buffers and state,
including 1..22 bones, sparse/zero masks, 21-bone upload splits, world bone256,
different starting offsets, reused pose addresses and short-capacity rejection.
The first 96 cases are instruction-counted: parent318138, candidate254261
(-20.08% for this synthetic publisher sample, not whole-frame work or FPS).
Corrupted header, lost data0 and wrong tail-coordinate mutants fail the byte
comparison. Source SHA8ea4b18fe3f730448b5c08c8c07d963e93512027d4960807fd44b7a2231a4079.

The initial harness incorrectly expected unused cache counters that the linker
removed; corrected. A subsequent fully instrumented run hit its explicit
60-second emulation bound, with its partial result preserved. Counting only the
first96 cases permits the full unchanged correctness suite to finish. The first
header mutant failed compilation for an unused variable; the corrected mutant
uses the header and fails at runtime as required. None was an application/GPU
failure. Evidence: actor-pose-stream-test and its three named mutant directories.

## Broader status comparison

`tools/compare_last_hardware_work.py` verifies source/ELF, save, input, recipe,
settings, emulator and the eight-frame window against physically tested
Old45631515f007. Validatede25b518c7339 has21,929,498 versus27,651,093 ARM instruction
encounters (-20.69% total CPU work observed). This is not an attributable
full-frame timing or physical FPS result: lightning and particle/audio phases
vary, and the changed audio backend moves work onto the DSP. No instruction-to-
hardware-FPS multiplier is used. Last physical evidence remains73.988ms/13.5157FPS;
61.19ms remains an unverified hypothesis. The comparison JSON is stored beside
the candidate runtime as last-hardware-work-comparison.json.
