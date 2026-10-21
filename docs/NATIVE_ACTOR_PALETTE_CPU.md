# Native actor palette CPU publication

17 October local candidate, not deployed or accepted for release. Parent is
the exact archived 9c33896214fc audio-service console build. The rejected
per-pose cache described in NATIVE_ACTOR_POSE_ROWS.md is not reintroduced.

Two changes in pica_actor_commands.c preserve the published command stream:

- Unroll the fixed three bone rows, retaining float32 arithmetic order.
- Set contiguous uniform dirty bits by word ranges rather than one bit per row.

No allocation, worker, shader, GPU command, geometry, simulation or resource
lifetime changes. Existing prevalidation and unpublished command ownership
remain. Pinned Melee uniform_upload.h was reviewed for cache/wire ownership;
the range marking is a local adaptation, not code taken from that renderer.

Actual ARM11 parent/candidate comparison passes 2,304 complete command/state
checks including sparse masks, world bones, chunk boundaries, changed poses
at reused addresses and short-capacity rejection. First 96 publication cases
are counted. Synthetic encounters: parent254453, unroll247462 (-2.75%),
dirty ranges240290 (-5.57%), combined231925 (-8.85%). These numbers are not
captured-workload timing or FPS predictions. No additional hardware test is
justified by this isolated result.

Actual extracted dirty helper passes 152,096 ASan/UBSan comparisons spanning
all legal ranges in 96 rows and 32 initial bit patterns, with adjacent canaries.
Source hashes and results are under actor-{pose-unroll,dirty-range,palette-cpu}-test
in artifacts/research/pica-scene-rewrite. Reproduction tools are
test_actor_pose_unroll.py, test_actor_dirty_range.py,
test_actor_dirty_ranges_host.py and test_actor_palette_cpu.py.

The existing exact-binary eight-frame profile counts 477944 encounters in
fh_pica_actor_command_append. DWARF line attribution is recorded beside that
run but has no source MD5, so the analyzer deliberately does not certify its
source text. No new emulator run was used for that analysis.

build_actor_palette_cpu.py isolates the combined patch against the deployed
archive, verifies shared Old80/New124 profiles and identical shaders/game
archive, and restores root WIP. Whole-application runtime cost, image and
physical acceptance remain pending; neither synthetic result establishes
the required 30 FPS Old or 60 FPS New targets.

Shared build 88915fc75528 completed; source/archive restoration passed. Only
pica_actor_commands.c changed; shaders/game archive and Old80/New124 remain
identical. Exact palace run completed in128.74 host seconds (timing excluded),
benchmark status1, saves/settings preserved, logs drained, owned exit, zero
missing actor attempts. Capture1601 inspected: Krystal, textured floor, sky,
HUD; no claim of full image parity. Known unsupported core-ID SVC caused raw
telemetry failure; independent exact-ELF execution audit proves core1 audio.
Actor append encounters477944->416800; main-core total16224306->16017300,
core16193750->6099321. Particle/audio phase differs, so whole-frame speedup
is not established. Observed memory unchanged: heap41364432/49283072,
linear free3591424/12582912, VRAM4723712, stack72984/524288,
process70647808/83886080. User requested hardware CIA test; package
319f9db06c3abab6 retains explicit limited-test review and general-release
rejection. Transfer evidence20261017-172336-old229-direct-install
is authoritative for upload completion. No assets/saves/settings in package.
