# Actor cache retention candidate

The shipping publisher compares live colours, raster state and sampler identities
before emitting native commands, but then rewrites those cache entries even when
unchanged. The isolated candidate uses the existing change masks to retain them.
It does not remove validation or alter command publication or GPU ownership.
The pinned Melee uniform uploader likewise retains unchanged cached rows; this
adaptation concerns native actor state rather than copying its shader interface.

The actual ARM parent and candidate passed 2,880 full command/state comparisons,
including enabled/disabled texture units, sampler replacement, invalidation and
capacity rejection. The first narrow run timed out and is not passing evidence.
The expanded run completed under a 240-second bound. Synthetic publisher work
fell from 411,356 to 409,380 instructions (0.48036%). This is not a whole-frame
or console FPS result. No app was built or deployed from this candidate.

Reproduce with `tools/test_actor_state_retention.py` under the SDK WSL wrapper,
then `tools/record_actor_state_retention.py`. The pure patch is
`tools/actor_state_retention_patch.py`. Keep it isolated until composed changes
have inclusive workload evidence. The production parent remains audio-stop-boundary-v1.
