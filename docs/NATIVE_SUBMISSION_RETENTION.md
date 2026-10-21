# Submission retention candidate

Local build 3ae4c8caf502 combines retained actor cache words and a sixteen-row
clean scan of the public SDK uniform dirty flags. No setter-side summary is
used: direct SDK/native byte writes remain authoritative. Four-row and single
row handling remains for other patterns. No GPU work or simulation is omitted.

Actual ARM comparisons passed 1,164 cases including commands, cache validity,
ever-dirty state, direct writes at every position in both stages, random sparse
masks, context/float24 transitions and capacity rejection. Synthetic instruction
counts improve 1.891% overall but random sparse masks regress. This is not FPS.

The isolated shared build preserves the game archive, all GPU programs, setup
recipe and Old80/New124 profiles. Working source and archive restoration passed.
The compiled SDK uploader begins with the exact tested source, followed by the
existing actor handoff wrappers. Both tested code paths have no added allocation.

The Old palace run passed save/settings preservation, state, log drain and owned
exit. Its image was inspected: Krystal, world textures, purple creatures,
particles and HUD remain. This is limited correctness coverage, not full parity.

Eight-frame main encounters rose 15,243,960 to 15,510,429 (1.748%). The candidate
window includes lightning absent from the baseline and differing particle work.
Therefore neither a causal frame regression nor a gain is established. Uniform
work rose 586,017 to 594,946; actor publication fell 375,816 to 372,520. The build
is NOT promoted or deployed. Do not translate these numbers into console FPS.

Reproduce using build_submission_retention.py, run_submission_retention.py,
analyze_arm_blocks.py and compare_submission_retention.py. Keep the uploaded
4f58d475c8f7 audio-stop build as production parent. See the archived
PERFORMANCE_UNPROVEN.json and submission-retention-test/result.json.

## Short-circuit scanner revision

`uniform_sparse_scan_v2.py` first checks the initial dirty word and reads the
remaining three only when that first word is clean. The original four-row skip
is retained when the complete sixteen-row range is not clean. This remains a
scan of authoritative public SDK bytes, with no stale side index.

The exact same 1,164 ARM cases produce the same command hash and state. Counts
are shipping 1,544,939, v1 1,515,723, v2 1,494,730 (3.250% below shipping).
There are still 252 individual regressions against shipping, at most 84
instructions. These are synthetic distributions, not captured gameplay inputs,
and cannot establish frame time. No new build or deployment was made for v2.
Use `test_uniform_sparse_scan_v2_arm.py`; earlier v1 evidence stays intact.
