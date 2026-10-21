# Native-rate AUX first-window admission

The HLE run `artifacts/native/glow-shared-cost-v1-parent-save-new` stopped at
retrace 2 with native callback fault 13. Its first returned window began at
source frame 1095, phase 1174855, queue epoch 1, before the stream was initialized.
The native-rate owner required phase zero even though its source-to-device
mapping accepts every valid stamped phase. Unlike the older resampling owner,
this owner has no source-rate filter history to prime.

The repair initializes the expected phase from the first validated window.
Later windows still require exact source position, phase, epoch and mode.
Range/span/lookahead checks and effect-route admission remain intact. There are
no added buffers, per-sample operations, or changes to DSP publication.

`tools/test_audio_aux_first_phase.py` v2 passes 5,615,345 checks under ASan/UBSan,
including 4096 initial phases and the captured queue/window metadata with
explicitly synthetic PCM. Mutants restoring the zero-only guard or removing
phase/epoch continuity are rejected. Strict ARM compilation passes, with the
process function using 200 bytes of stack. V1 failed because its fixture omitted
required effect parameters; that failed artifact is retained.

`tools/build_audio_aux_first_phase.py` isolates the shared-v1 normal snapshot.
Its only source delta is `audio_aux_native.c`; all shaders, the game archive,
installation recipe and Old80/New124 profiles are unchanged. It restores root
files after building. `tools/run_audio_first_phase_save.py` repeats the HLE route.

This is separate from physical crash198 during the dinosaur rescue. That dump
faulted in C3D_FrameBegin and does not establish an audio cause. A successful
save-loading route is not coverage of the rescue dialogue/progression.

The exact normal build 6f55f2381d25 (3dsx 219d532c1848) completed the formerly
failing HLE route in 75.36 host seconds, reaching the cave and exiting cleanly
at retrace 1680. Saves/assets were unchanged; logs drained without errors.
The final capture shows Fox, Tricky, the cave gate, torch, world textures and HUD.
This establishes the startup fix on that emulator route only. It does not
establish physical crash repair, audio fidelity, or a hardware FPS gain.
Neither build is deployed. The matching replay now includes the fix:
source `11d7cebdbd47`, 3dsx `5837ff65be1a`, CIA `9f535d2179bf`.
`verify_audio_first_phase_pair.py` verifies 795 identical source records,
21 matching GPU programs, identical game archives and Old80/New124 profiles.
This source check is not a new replay-runtime claim.

The private save audit `rescue-save-context-current.json` retains valid checksums
and reports overlapping SnowHorn Wastes / LinkC map bounds. Loaded object
observations contain LinkC portcullises, pressure switches and a staff lever;
they do not contain a SnowHorn rescue actor. The key is owned but unused and
the rescue flag is unset. Repeating the previous key-menu test at this location
does not exercise the reported rescue. Progression must first reach the relevant
actor through normal navigation; do not patch those flags or teleport there.
