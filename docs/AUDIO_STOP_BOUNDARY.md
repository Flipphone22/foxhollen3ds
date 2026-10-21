# Audio stop timestamps after a completed mix block

Old229 build `cfa8c24c05c3` aborted in an Arwing segment at frame 3996. The saved panic reports `caller=0014f6d0 core=1`; the exact deployed ELF resolves the preceding instruction to `salDeactivateVoice`, after `fh_audio_source_stop` returned false. This is distinct from the previous raw AUX callback fault. The log does not record which backend stop precondition failed.

The deployed mixer resets `renderOffset` to zero when a block finishes, but keeps `cycleTick` at that block's start. A stop between blocks therefore requests the old start time even after controls through sample 128 of the 160-sample block have been queued. The backend correctly rejects a stop earlier than its last submitted control.

The candidate records whether the source cycle has completed. Between blocks it uses the completed block boundary for native stop timestamps. `salPCBeginCycle` and initialization clear that flag. In-block stops retain their original offsets. The separate depop cursor/offset, voice ownership, queue ordering checks, callback behavior, simulation and renderer are unchanged. Additional static state is one unsigned word; there is no new allocation, worker, wait, copy or GPU work.

`test_audio_stop_boundary.py` verifies the generated mixer against its MD5 recorded in the deployed ELF's DWARF, then executes its actual begin/deactivate/end fragments against a mock sink enforcing the native DSP ordering rule. The baseline aborts all 1,000 between-block stops; the candidate accepts all 1,000 while retaining 6,000 tested in-block timestamps. This is a control-flow reproduction, not a DSP waveform test or proof of the unrecorded hardware failure condition.

The pinned Melee audio implementation was consulted for ownership/publication. It has no equivalent MusyX per-voice stop timeline; its NDSP buffer ownership and flush rules are not changed or replaced here.

Evidence: `artifacts/research/pica-scene-rewrite/audio-stop-boundary-test/result.json`, `artifacts/console-test/20261018-180711-old229-startup-direct/`, and `artifacts/console-crashes/20261018-180725/`. Do not attribute the collected older OS dumps to this run without additional identity evidence.

Build `audio-stop-boundary-v1-console-setup-builds` is isolated from the unvalidated CPU skinning candidate. Its generated mixer exactly matches the tested candidate. The game archive, shaders, install recipe and Old80/New124 profiles remain unchanged. Physical Arwing acceptance remains outstanding.

Old229 deployment completed and byte-verified: `artifacts/console-test/20261019-021715-old229-direct-install`. Source `4f58d475c8f7`. Install `SD:/3ds/foxhollow/foxhollow.cia` with FBI to update the HOME icon. Headless palace correctness passed; physical Arwing validation is still pending.
