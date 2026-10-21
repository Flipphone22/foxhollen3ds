# Defined vertex input when distance fog is disabled

16 October 2026. Candidate normal build `852f9538c147` repairs a confirmed
source defect. A subsequent physical run reproduced the Start game hang on
this exact build: the fog repair is **not sufficient to fix the hang**. The
fresh scene commands publish c94 correctly. See
[packet submission investigation](NATIVE_PACKET_INLINE_SUBMISSION.md).

The vertex programs consume c94 (`fog_distance_scale`) even when fragment fog
is disabled. The old fog owner only published this uniform inside its enabled
branch. Cold OFF or invalid-range draws could therefore consume inherited,
undefined state. The replacement publishes zero while disabled and the existing
scale while enabled. Dirty-state caching remains intact; repeated OFF draws do
not upload again, and OFF does not rebuild or upload the fog LUT.

The captured failing queue's first unfinished command is queue 2, containing 32
scene draws. `curEntry` is the next command to submit; queue 5's specular copy
had not been submitted. Earlier attribution to that copy was incorrect.

The captured command history does not establish c94 for the first six draws.
Executing captured vertices through the pinned Azahar float24 interpreter finds
18,472 vertices with this unknown input. Synthetic zero injection gives finite
outputs; synthetic NaN injection produces nonfinite outputs for those vertices.
This does **not** show that hardware contained NaN or prove the hang's cause.

Validation:

- Actual archived fog owner under ASan/UBSan: cold OFF, 10,000 retained OFF
  draws, and 1,000 invalidation/option/range transitions pass; parent rejected.
- Existing fog-owner/SDK LUT regression: 200,020 source comparisons and retained
  draw/handoff checks pass; missing-invalidation mutant rejected.
- Normal Old-mode Start game run reaches the menu and saved cave, then exits:
  80.256 host seconds, 167,936 audited draws, no layout fault. Captures inspected.
  Parent also passed this emulator route; this is not hardware-hang reproduction.
- Memory matches parent: heap 39,769,800/50,331,648 B; linear free
  2,214,016/10,485,760 B; VRAM free 4,723,712 B; stack 72,984/524,288 B;
  process 69,599,232/83,886,080 B. No physical FPS improvement established.
- One native source changed; all 18 GPU programs and game archive unchanged.
  Shared Old80/New124 CIA profiles retained; isolated build restoration passes.

Evidence is under `artifacts/research/pica-scene-rewrite/` in
`fog-disabled-uniform-test/`, `fog-defined-owner-regression/`, and
`unified-fog-defined-v1-console-setup-builds/`; runtime evidence is
`artifacts/native/unified-fog-defined-v1-play-old-dsp/`.

Deployment completed to Old229, with all six application files staged and fully
byte-verified before publication, then fully read back at their final paths:
`artifacts/console-test/20261016-194237-old229-direct-install/transfer.json`.
Source `852f9538c147`, 3dsx `26276f0c5b5e`, CIA `159bab400939`, package
`6e88e4bf9552f8f2`. No assets, saves, settings, or logs were replaced.
New223 unchanged. Install `SD:/3ds/foxhollow/foxhollow.cia` through FBI to update
the HOME icon, then test normal **Start game**, not a separate probe.

The first transfer failed before mutation because this ftpd rejected SIZE and
reset LIST data connections. The direct deployment helper uses bounded direct
RETR, validates the package, stages and verifies all files, retains backups,
and records every publication. It does not infer missing files from ambiguous
FTP errors or silently clear diagnostic logs.
