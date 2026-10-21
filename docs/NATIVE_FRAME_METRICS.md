# Less render-log traffic, with complete frame accounting

The physical trace collected at `console-visual/20261010-222419-223` is
byte-identical to the prior211130 collection. It is not a new run. In that
retained trace, the main thread waited180.797ms for the diagnostic queue.
Later1024-byte native-scene reads took68.885/216.206ms while diagnostic writes
overlapped. Those overlaps support reducing logging traffic; they do not prove
that writes explain every stall. The already deployed memory-log reduction
has not received fresh physical evidence.

The normal application now accounts for every submitted frame in a small
fixed state, and serializes about one interval per wall-clock second. Each
completed interval over100ms additionally produces a timestamped row. Shutdown
publishes the remaining short interval once. There are no new allocations,
threads, forced flushes or sleeps. Existing buffered stdio and the joined
logger remain responsible for SD writes, following the pinned Melee ownership
discipline. No game assets, saves or simulation work are skipped.

The first16 CSV columns describe the last frame in each recorded interval.
New `window_*` columns hold complete counts, sums, longest interval and its
frame, plus counts above33.5/100ms. Consumers must use these totals, not average
the sampled last-frame columns. `frame_metrics_analysis.py` and both native
analysis tools now do so. They admit only complete windows at a requested frame
boundary and report the actual covered range. Percentiles are explicitly
unavailable for aggregated rows; replay/per-frame data retains them.

Replay builds retain per-frame serialization and their separate replay phase
accounting. The normal application's FPS uses all intervals, not the logging
cadence. GPU values remain the SDK's completed-segment measurement, not a claim
of full physical GPU frame time.
The inherited `submit_ms` counter covers only its existing native owners; it
is not the replay's complete `native_submit` phase. Its summed value is retained
exactly, without upgrading that partial counter into a full CPU breakdown.

The actual C serializer and Python analysis pass ASan/UBSan checks over36,017
frames with60/20/15FPS sections and a nine-second gap. Normal output1235 rows/
154,701B versus detailed36,017 rows/4,178,065B:27.0x less render-log traffic.
Every draw, index, submission duration, GPU sample, frame interval, threshold
count and final short window is checked against the independent sequence.
The nine-second gap remains timestamped at frame15000. A removed final row is
rejected by the coverage check. This is a serializer test, not console timing.

Shared normal9eec94e236e3/replay97a579346965 builds match334 native sources,
the unchanged game archive,511-instruction main program and270-instruction
material bank. Their CIA profiles remain Old80/New124MiB. Six unrelated VRAM
work-in-progress files are isolated and restored by the build helpers.

The exact normal check completes25.11host seconds/721 retraces, settings
toggles and Select exit. It records713 rendered frames in14 rows/1951B,
retaining a1.107-second startup/loading gap at render frame303. The540 capture
was inspected. The exact replay completes44.77host seconds, state assertions,
save preservation, log drain and owned exit; all1526 frames have individual
rows. Its1410 capture was inspected. Measured palace cadence59.830786AzaharFPS,
scene1.869282ms/submission4.318846ms is comparable to the preceding bank
candidate's1.898985/4.325605ms; no material timing regression is established.
This is not a physical FPS estimate or proof that logging changes sped up the
game. The normal run includes startup and is not a gameplay FPS benchmark.

New replay memory: heap48,472,648/71,303,168B, minimum free linear18,396,416/
25,165,824B, free VRAM4,822,016B, measured stack72,796/524,288B; process
103,206,912/130,023,424B. No additional linear RAM/VRAM or worker is used.
The preceding same-renderer Old bank run supplies unchanged renderer coverage;
it was not repeated for this serializer-only change.

Evidence: `frame-metrics-test/result.json`,
`frame-metrics-runtime-test/{integration,build-verification,runtime-comparison}.json`.
Physical stutter repair, all remaining rendering discrepancies and performance
targets remain unvalidated. LIGHT MAPS remains opt-in OFF and its additional
CPU budget still fails the earlier0.20ms gate; this logger change does not
reclassify that feature as accepted.
