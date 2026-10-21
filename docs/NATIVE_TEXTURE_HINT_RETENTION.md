# Texture hint coalescing candidate

After a broad texture hint, each entry is either invalid or remains protected
by its current file-loader token. Another hint cannot change those states until
publication, ownership or mode changes occur. The candidate tracks that settled
state, flushes pending UI first, and skips redundant cache-wide queries.

Both production valid=1 sites rearm the pass. Registration, token exhaustion,
forget and mode configuration rearm too. Explicit CPU source writes still use
the existing write index and overlap invalidation, including unknown ranges.
They can only invalidate entries; they cannot restore an entry or grant a token.
No GPU storage reuse, fence, queue, shader, allocation or worker changes.
Added static state is eight bytes. The existing retained counter now counts
visited preserved entries; a separate coalesced counter records skipped passes.

The test extracts the actual archived visibility function, compares it with the
patched function under ASan/UBSan for 200,000 operations in each of six modes,
and checks validity and UI flushes after every step. Removing publication or
ownership rearming fails. The conservative index stub returns all entries;
publication/token changes are modeled explicitly, not full loader execution.
Thus the 124,933 to 67,889 query reduction per fixture is not a game measurement.

The isolated shared normal build a0723e0c7e8a is built, but not deployed or
promoted. Its parent is uploaded 4f58; only the two texture ownership files
change. Game archive, GPU programs, setup recipe and Old80/New124 profiles are
unchanged; selected workspace files and archive were restored after building.

The exact binary completed the Old palace benchmark with status 1, preserved
saves/settings, drained logs and exited normally. The inspected palace capture
shows Krystal, textured walls/floor, purple creatures, particles and HUD. This
does not prove full visual parity. The run logged 1,455 coalesced hints and zero
visited file-token retains; it exercises cache invalidation coalescing, not the
file-token preservation branch covered by the host fixture.

Across eight updates, total main-core encounters changed from 15,243,960 to
15,062,443 (-1.19075%). Texture visibility fell 189,862 to 109,179 (-80,683),
equivalent to 0.52928% of baseline main encounters. That is a local contribution,
not a causal whole-frame gain: particle update/draw encounters also fell 72,343
despite unchanged particle code. Captured encounters include failed predicates;
they are not CPU cycles, elapsed milliseconds or an FPS prediction. The profiled
run's 129.387 host seconds must not be compared as an uninstrumented frame time.

Sampled heap peak was 41,366,136/49,283,072 bytes; linear minimum free
3,642,112/12,582,912; initialized VRAM free 4,723,712; main stack peak
73,008/524,288; process 70,713,344/83,886,080. The implementation adds eight
static CPU bytes and no GPU allocations. No new physical measurement exists.

Evidence: texture-hint-retention-test/result.json (host oracle),
texture-hint-retention-test/runtime-comparison.json (inclusive comparison), and
artifacts/native/texture-hint-retention-v1-palace-old-profile/ (runtime/capture).
Reproduce with build_texture_hint_retention.py, run_texture_hint_retention.py,
analyze_arm_blocks.py and compare_texture_hint_retention.py. Keep this small
candidate separate from a claim to close the remaining frame-time gap. Existing
3ae4 and uniform scan v2 also remain unproven for whole-frame performance.
