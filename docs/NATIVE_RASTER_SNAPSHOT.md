# Fused native raster cache

16 October 2026. Local work; no deployment or hardware performance claim.

The existing DSP-default palace capture attributes all 9,480 `memcmp` entries
and 8,437 `memcpy` entries to direct ARM branch sites in the exact archived ELF.
The retained SDK raster helper accounts for 1,311 compares and 1,368 copies.
Audio curve controls account for 4,403 compare calls in their producer and 300
in the callback. These are call counts, not a division of memory-function time.

`analyze_arm_call_sites.py` reuses the capture; it does not launch an emulator.
It decodes direct ARM B/BL sites and records instruction encounters. Failed
predicates can make these upper bounds on calls; indirect branches are excluded.
All listed observed branches in this capture were unconditional, and their
totals agree with the callee entry counts. Tail calls retain the enclosing
return path ambiguity. The ELF hash and capture guards are checked.

The raster candidate replaces the general byte comparison followed by a copy
with a single pass over aligned words. It loads each source word once, compares
it with the cached word and stores it in the private CPU shadow. Equal-value
stores do not touch GPU command or resource storage. The compiler can inline
and specialize fixed-size callers. SDK structure size/alignment assertions
guard the word access; GNU `may_alias` permits object-representation access.

Valid bits, enabled/disabled behavior, reuse counters and the boolean decision
to publish are unchanged. Context restoration, framebuffer changes, shader
configuration, geometry-stage boundaries, texture cache commands and resource
retirement retain their existing owners. Pinned Melee's command-cache and
retained-state ownership were consulted. No new worker, allocation, GPU command,
shader, pass or visual omission is introduced.

Validation so far:

- 33,536 actual old/new helper comparisons cover all byte mutations in 4..256
  byte blocks, valid/invalid state, reuse enabled/disabled, exact counters,
  source preservation and destination canaries. ASan/UBSan pass.
- The generated SDK source compiles against actual pinned SDK structures.
- The existing ARM SDK publisher test passes 152,263 assertions and preserves
  all 1,918 side effects. Missing context invalidation, program invalidation
  and geometry boundary mutants all fail. Raw native streams are not entered
  by that fixture; stubs fail immediately if an unmodelled owner is reached.
- Synthetic ARM11 helper cases use 18,060 instructions versus 20,000 (-9.7%).
  This includes wrapper work, uses GCC O2, and is not a captured distribution,
  CPU-cycle measurement or whole-frame prediction. The integrated app uses its
  normal toolchain and still requires separate evaluation.

The first standalone ARM attempt lacked VFP enablement and stopped on an invalid
instruction; enabling the same ARM11/VFP state used by existing tests resolved
that harness error. The older publication fixture initially lacked native-stream
symbols; fail-on-entry stubs supply its deliberately unexercised scope.

Reproduce with `test_raster_snapshot.py`, `test_raster_snapshot_publication.py`,
`build_raster_snapshot.py console-setup --revision v1`, and
`verify_raster_snapshot.py`. Use the isolated archived builder, never the root
work in progress. Parent is DSP-default/collision source `5f50dd9e7bc7`;
the unpromoted particle experiment is not silently folded into this comparison.

The candidate must retain one shared Old 80/New 124 MiB CIA and 3dsx, unchanged
private assets, save files and conversion recipe. Full rendering parity,
progression, endurance and target FPS remain incomplete.

The shared candidate is source `9dc28d861d24`, 3dsx `4181d780d24a`,
CIA `df2bbb77c37f` and tested CXI `598a6e43dd42`; it passed
the Old DSP palace run with 307 rows, unchanged saves/settings and drained owned
exit. The palace capture at 1600 was inspected: Krystal, textured world,
floating enemies and HUD remain visible. Audio source/callback faults and curve
shortages were zero. No new PCM comparison or complete visual-parity claim.

Whole sampled ARM work was 24,676,208 versus 23,937,467 (+3.09%), with 28 versus
25 audio producer calls and 324 versus 286 particle pool draws. No causal total
speedup or regression is established. The combined publication/libc bucket fell
0.96%, but includes unrelated audio comparisons and is not isolated cache cost.
Actual raster-helper calls to `memcmp` disappear; copied snapshots are handled
in the fused pass. Do not repeat boots to seek a favourable total.

Observed Old heap 41,717,672/50,331,648 bytes, linear minimum free
1,755,776/10,485,760, VRAM free 4,723,712, stack upper bound 72,984/524,288 and
process 69,582,848/83,886,080 match the parent. The 153.31-second host runtime
includes DSP LLE/profiling and is not performance evidence. New hardware,
moving routes and endurance remain unvalidated.

`prove_raster_snapshot_source.py` matches the complete generated SDK unit to
archived compilation SHA256 records, and reconstructs the parent with only the
helper replacement. All other source bytes match. An initial DWARF-MD5 approach
was rejected because this GCC SDK unit uses DWARF4 without source checksums;
the proof explicitly uses the separate compilation record instead.

Both FTP addresses timed out before login during one bounded availability
check. No console files, settings or saves were changed. The new candidate is
ready for packaging with DSP-default audio and retained collision queries;
it does not include the separate unpromoted particle-batching experiment.
