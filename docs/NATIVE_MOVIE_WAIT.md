# Bounded synchronous movie-read wait

The exact linked call-site audit in
`world-deferred-cockpit-profile-adaptive-new/poll-calls.json` attributes 36,141
of 36,149 `padUpdate` calls to `DVDRead`. The remaining eight calls come from
`gameMain`. Every observed input-function entry is accounted for by those
direct ARM call instructions. This identifies the busy loop; it does not
measure physical wait duration or prove a hardware FPS gain.

The candidate adds an event wait after the original synchronous `DVDRead`
loop's input, reset and DVD-completion servicing. It only admits the current
queue-head movie request while BUSY. It excludes cancellations, completed
requests awaiting replay release, generic disc reads, reentrant pumping and
unrelated file handles. Asynchronous pumping stays nonblocking.

The existing movie worker publishes READY with release semantics, then signals
its existing one-shot event. The synchronous caller waits on that event for at
most one millisecond before returning to the original service loop. Completion
between the state check and wait cannot be lost. No fixed one-millisecond sleep
is inserted after a completed read. A stale event can cause one immediate
return; the next service pass still checks the authoritative state. Only main
publishes buffers, cache notifications and completion callbacks. A timeout
neither cancels a read nor causes a panic.

Pinned Melee `file_io.c` uses an owned sleeping worker; its async-file test
fixture sleeps one millisecond while polling (line 334). That fixture is not
Melee's production synchronous-read implementation. This adaptation uses the existing
completion event to wake earlier, and limits admission to the movie path found
in the captured workload. There are no new buffers, threads, events, heap or
GPU resources. Existing join-before-close ownership remains intact. The existing
wait-tick accumulator now includes these synchronous waits.

`test_movie_wait.py` executes the actual patched async owner with pthread-backed
SDK events and 16 deliberately delayed reads under ASan/UBSan. It verifies
returned bytes, untouched buffer tails, completion-before-wait, idle/no-worker
behavior, and joining a pending read on close. The run required 298 polls for
16 twenty-millisecond reads. This is host scheduling evidence, not a portable
poll-count guarantee. A separate test executes the actual admission function
against canceled, unrelated, empty, reentrant, replay-gated and generic-read
cases. The first test compilation omitted `<string.h>`; that harness-only
error was corrected before the passing run.

The isolated builder uses `movie-crc-v1`, verifies tested source hashes and
compiles only `main/fileio.c` in the game archive. The archive audit checks
member order and every payload: only `fileio.o` and the symbol index change.
Full game timing, physical acceptance, long playback, and the dinosaur-rescue
crash are separate, still-open requirements.

## Runtime and CPU evidence

Normal source `9a6d173675b9`, 3dsx `f73150706c28`, passes the ordinary cockpit
route in 75.00 host seconds with clean exit, drained logs, unchanged private
saves/assets and an inspected final image. Sampled presentation windows average
19.6477 ms versus the CRC-only parent's 20.3712 ms. The raw +3.68% FPS comparison
is phase-dependent emulator evidence, not causal attribution or physical FPS.
The 16.67 ms target is still unmet; the worst sampled interval is 37.305 ms.
Observed memory budgets are identical to the parent.

The subsequent actual-ELF profile completes the same eight-frame window, normal
shutdown and save checks. The direct-call audit counts only 141 `DVDRead` input
polls, down from 36,141, while the eight `gameMain` input polls remain. Core-0
instruction encounters fall from 66,047,006 to 40,600,051, but that comparison
also includes the earlier CRC implementation and asynchronous scheduling
variation; it is not a frame-time percentage. Profiler time is excluded.

This run exposed the CRC implementation's production-Clang regression, which
the earlier standalone-GCC test missed. See `NATIVE_MOVIE_CRC.md`: accepting
the wait repair does not accept that CRC code-generation regression. A successor
build preserves the wait and corrects proven alignment in the CRC helper.
