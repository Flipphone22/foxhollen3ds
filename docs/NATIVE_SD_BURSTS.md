# File and thread attribution for SD bursts

The physical pauses included thousands of reads, while the detailed trace only
retained individual calls lasting at least 50 ms. A succession of short calls
could therefore disappear from that trace. The cumulative `sd-io.csv` still
counted them, but could not identify their file and calling thread.

`fh_sd_bursts.h` adds 4,520 bytes of fixed RAM. Eight pending entries group
completed calls by file identity, operation and launch-local TLS token. Each
window spans at most one second with gaps at most 100 ms. Windows with at least
two calls and 100 ms of summed IPC spans enter a 32-entry ring. Handle reuse
flushes the old association. Pressure, overwrites and invalid timestamps are
reported explicitly. It is a bounded recent history, not a complete trace.

Accounting performs no allocation, file write or additional time query. It uses
the existing counter lock, held only after the real IPC returns. Startup writes
an incomplete marker; successful shutdown joins the diagnostic worker and writes
`sd-bursts.csv`. A crash cannot present a previous completed trace as the new run.
The existing individual slow-call trace remains available. Replay archival and
the bounded FTP collector retain both files. No new worker or GPU work is added.

The pinned Melee file reader, async ownership and cache publication were checked.
Foxhollow already has ordinary disc, stream and movie workers. Native geometry
and material loading still has synchronous work; this recorder does not move
that work or change game loading semantics.

Use `py -3.13 tools/analyze_sd_bursts.py <path>/sd-bursts.csv`. The analyzer rejects
incomplete, truncated and malformed histories. **Call spans include service waits
and scheduling. They are not CPU time or an exact main-thread stall partition.**
Different threads and `sd-slow.csv` overlap; their totals must not be added as
elapsed frame time. Events are ordered by retirement, not necessarily start time.

Validation:

- Actual FS read wrapper: 1,000 two-millisecond calls produce two exact one-second
  bursts with the correct basename, full-path hash, TLS, bytes and maximum. The
  old individual slow trace records none, as expected.
- ASan/UBSan checks cover distinct overlapping callers, errors, close/reuse,
  gaps, reverse timestamps, pressure and ring overwrite. The CSV analyzer rejects
  incomplete/truncated/invalid variants of actual C output.
- Existing publication test still produces 601 rows for 36,000 frames, preserving
  all counters, a nine-second maximum and the final short interval.
- The first ARM build rejected a printf type mismatch that the host compiler
  accepted. Explicit casts corrected it before runtime. Final shared builds
  retain the same game archive, all graphics shaders and Old80/New124 profiles.
- A single New headless Arwing route completed in 66.12 host seconds, preserving
  saves, passing state checks, draining logs and exiting its owned process. Its
  25 burst records identify disc, movie, cache and texture-pack callers; the
  archived replay trace is byte-identical. The inspected frame1650 retains Fox,
  textured Arwing, tree, world and HUD. This does not reproduce intermittent
  physical disappearance or prove visual parity.

Azahar route FPS was 54.402 versus 54.536 for the prior environment correction;
scene/submission time was 2.0569/3.9506 ms versus 2.0587/3.9584. Workload and
animation phase differ; this is a short regression check, not a speedup or
physical storage-latency prediction. New heap peak56,421,728/71,303,168 bytes;
linear minimum free16,495,744/25,165,824; VRAM free4,822,016; measured stack
73,068/524,288; process103,215,104/130,023,424. No new Old runtime measurement.

Builds: normal `fc3841012fb3` / CIA `a5507299e029`, replay `d9614c762951` /
CIA `89141e2a6e58`. Evidence and isolated build helpers reside in
`artifacts/research/pica-scene-rewrite/sd-bursts-*`. Physical stalls, missing
surfaces, complete fur and lighting/shadow parity remain unresolved.
