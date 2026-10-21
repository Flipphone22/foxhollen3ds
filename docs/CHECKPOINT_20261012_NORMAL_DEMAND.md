# Normal-demand experiment checkpoint

Goal ACTIVE/INCOMPLETE. Previous turn PROGRESS: directional shader experiment
implemented, checked, then rejected. This turn PROGRESS: native normal-demand
shader/state publication implemented, shared builds, exact linked float24 and
SDK/sanitizer checks, three serial scene runs, V2 refinement and recorded limits.
No blocker. Hardware unavailable: no FTP, question, probe or deployment.

Continue accepted work from **actor-live-v2-builds** and matching normal
**actor-live-normal-v1-builds**. `normal-demand-v2-builds` is an unpromoted
experiment; V1 is superseded. No physical FPS gain or overall shader-work
reduction is established. Do not blindly build the global WIP tree.

V2 replaydbcc25d52a1e/CIA5cee88bdc599, unchanged game archive,458tracked sources,
sharedOld80/New124profiles. V1 replay425951271596/CIA741dda77cf4e.
Only ordinary/layered/fur programs change; ten others remain exact. Slot3 now
means normal demand in those programs, but still vertex material in the cloud
program. Cloud explicitly restores that state. UV-selection flag is preserved;
do not follow the older checkpoint's tentative plan to reclaim it. Colour-scale
sign carries vertex/constant material. The linked actor bank is510/512,96floats,
16bools. No extra GPU memory, shader bank, draw, worker or game-code change.

196,608vertices/V1 and again/V2 pass exact28-lane shader comparisons; six
wrong-sign/map-demand mutations per version fail. Actual SDK publication test
passes786,432handoffs plus missing-publication mutant. V2's publication code is
identical; evidence reused. V1Newmoving andOldcave plusV2Oldcave pass all state,
save, drain, exit checks; captures inspected. Render CPU changes +.048221,
+.019510,+.035361ms vsactor-live. V1New53.40962FPS, V1Old57.38154,
V2Old57.34439AzaharFPS; not speedup/physical evidence. V2New not rerun.

All owned handles terminal:12326(first test completion recovered from files),
87635failedVMcompile,2840V1build,27232compression,83197V1New,78175V1Old,
93411V2build,77418V2VM,88661V2Old. Initial V1New storage-guard rejection occurred
before emulator launch. Completed ELF compression13541816files358705108bytes,
13561132files716323956bytes; hashes preserved, no overlap with timing.
Both installed normal apps remain eb521d9e988c/CIA b9b63688dbd6.

Details and exact evidence are in [NATIVE_NORMAL_DEMAND.md](NATIVE_NORMAL_DEMAND.md).
Skip counters include already-unlit draws, so they cannot prove newly avoided
shader work. Existing native capture deduplicates/caps by program/material and
is not a complete-frame histogram. Any offline weighting extension must keep
that distinction and actual GPU cache/instruction-timing uncertainty explicit.
No further console probe is needed for this candidate. Full parity, shadow
budget, stalls, physical performance and completion criteria remain unmet.
