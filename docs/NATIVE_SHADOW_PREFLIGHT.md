# Receiver selection before shadow capture

Local candidate based on `collision-current-v2-builds`, not deployed. Hardware
is unavailable; this change does not require another console probe.

The previous native path rendered a geometric caster into its shadow image
before finding out whether any registered collision surface would receive that
image. The candidate prepares the receiver commands first. An empty result
avoids the capture, clear, target transitions and receiver submission. Authored
texture masks similarly avoid an empty auxiliary submission.

The selection equations, source surface routes, finite queries, texture bounds,
point ranges and draw order are unchanged. The bounded command list contains
only receiver references, matrices, query bounds and point ranges. It neither
copies geometry nor changes simulation, animation or the source shadow policy.
No GPU program or image resource is added. The existing pinned Melee/SDK
submission and retirement rules remain in place for every nonempty capture.

There are at most 128 prepared receiver commands. They are CPU-owned scratch
for synchronous submission, never borrowed GPU buffers. This adds normal-memory
storage and copying, so an empty-capture counter alone cannot establish a win.
The actual ARM storage size, total CPU cost and captured workload must be
recorded from the build and replay before considering promotion.

`tools/test_shadow_preflight.py` extracts the actual previous selection loop
and compares the new prepared commands against it under ASan/UBSan. The 12,000
cases include both surface routes, authored masks, varying block origins,
finite bounds and all capacities from zero through 128. The missing-matrix
mutation fails. These cases prove the CPU refactor, not GPU performance or
physical-console visual acceptance.

The existing shadow parity gaps and failed full-effect CPU budget still apply.
This optimization must not be described as repairing softness, dynamic object
receivers, the source receiver-count limit, or all visual discrepancies.

The shared candidate is `0a3e70fd1617` (CIA `2b8fcf1a3341`). The build
verifies identical game archive, all thirteen GPU programs and Old80/New124
profiles. One source file differs from the collision baseline.

The moving New-profile replay completed in73.057hostseconds with saves/state/
log-drain/owned-exit checks passing. Both1500/1650 images were inspected: world,
Fox, Arwing, HUD and grounded shadow remain. It skips378empty authored-mask
submissions, but **zero geometric captures**. Combined scene/submission CPU
changes by-0.004807ms, effectively unchanged. Animation and workload counts
vary; Azahar52.413versus53.332FPS is not a physical estimate or a speedup.
This candidate is **not promoted** and does not justify another hardware run.

ARM prepared storage is10752normalbytes; process commitment grows12288bytes
including code/alignment. New process103337984/130023424, heap56566256/71303168,
linearfree16041472/25165824, VRAMfree4723712, measuredstack73068/524288. There is
no new Oldruntime evidence. Existing collision-current-v2 remains the baseline.
