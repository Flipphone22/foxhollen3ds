# Per-invocation actor row cache: not accepted

The 16 October experiment prepares bone uniform rows lazily during one immutable
actor-pose borrow. It does not cache across actors or frames. It retains the
existing native palette, command-buffer publication and SDK state ownership,
with no shader, geometry, texture, simulation or game archive changes. Pinned
Melee uniform-row reuse informed the lifetime design.

`tools/actor_pose_rows_patch.py` modifies three archived native sources. The
isolated builder is `tools/build_actor_pose_rows.py console-setup --revision v1`.
It starts from `unified-world-packet-v1-console-setup-builds`, preserves the
shared Old 80 MiB/New 124 MiB profiles, and restores root/archive state. The
candidate source is `4917134477bb58b93da9df2ee6da72c7441f24b4bfd2c3f3b7c72482a0bcf16f`.
It is **not deployed or accepted for release**.

The exact parent/candidate ARM publisher comparison passed 1,466 checks,
including byte-identical commands/state, fresh poses at reused addresses,
world-bone slots and capacity/context rejection before mutation. Synthetic
publisher encounters fell from 971,769 to 920,972 (5.23%). An earlier oversized
fixture timed out; its incomplete result is retained separately.

The actual Old palace run completed with state checks, isolated saves/settings,
drained logs and owned-process exit. The frame-1600 capture was inspected:
textured palace floor/walls, Krystal, purple creatures, particles and HUD remain.
This single view is not full visual parity or hardware acceptance.

The actual eight-frame profile rejected the expected saving:

| Owner | Parent ARM encounters | Candidate | Calls, both |
| --- | ---: | ---: | ---: |
| Actor command append | 624,312 | 632,584 | 280 |
| Actor scene dispatch | 238,872 | 251,072 | 160 |
| New cache begin | 0 | 5,184 | 0 / 144 |

The run recorded 81,450 hits and 73,277 misses (whole run, not just the profile).
Reuse was insufficient to offset cache bookkeeping/copies. Whole-window
encounters rose 23,408,397 to 23,877,314 (+2.00%), but differing particle/audio
work prevents attributing that whole difference to this change. These are ARM
instruction encounters, not cycles, wall time or physical FPS.

The cache uses 12,396 bytes of local stack, no new heap/linear/VRAM allocation.
Observed stack high-water remained 72,984/524,288 bytes; this does not mean the
new frame has no stack cost. Heap peak remained 41,734,480/50,331,648; minimum
linear free 1,673,216/10,485,760; VRAM free 4,723,712; process commitment
69,611,520/83,886,080. No endurance or physical-console validation was performed.

Evidence: `artifacts/research/pica-scene-rewrite/actor-pose-rows-test/` and
`artifacts/native/unified-actor-pose-v1-benchmark-old-dsp-profile-v2/`.
Reproduce analysis using `tools/analyze_arm_blocks.py RUN`, followed by
`tools/compare_actor_pose_rows.py`. Continue from the world-packet parent, not
this cache candidate. Broader persistent submission coverage remains necessary.
