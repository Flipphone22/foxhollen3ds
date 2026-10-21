# Movie validation and cockpit CPU work

**Production compiler qualification:** the later actual-ELF profile found that
the initial `movie-crc-v1` implementation does **not** retain the isolated GCC
instruction improvement. The production THP unit uses Clang. Its linked helper
takes 1,253,394 instructions for the same aligned frame versus SDK zlib's
1,110,894, confirmed by `movie-crc-test/wait-v1-linked.json`. The live eight-frame
profile agrees at 10,027,152 helper encounters for eight frames. Earlier raw
full-frame comparisons therefore must not be attributed to faster CRC code.
The old evidence is retained. A local alignment annotation is being evaluated
against the actual linked function before accepting a replacement.

The aligned successor now passes actual-ELF execution against the same linked
SDK zlib. Clang emits aligned word loads after the byte-prefix loop rather than
reassembling every word with byte loads. It uses 958,482 instructions versus
1,110,894 for the aligned 196608-byte test (13.72% fewer); unaligned full frames
also pass and improve. The helper has a 36-byte register-save stack frame in
production, distinct from the earlier GCC wrapper's 32-byte result. No aligned
load occurs until the prefix has proven four-byte alignment and eight bytes
remain. This preserves strict-aliasing safety through `memcpy`.

Evidence: `movie-crc-test/aligned-v1-linked.json`, testing the actual archived
ELF; `movie-crc-aligned-test/result.json`, 65,600 sanitizer comparisons. The
internal Clang helper specializes away the constant zero CRC parameter, so the
actual-ELF harness explicitly uses its inspected two-argument ABI. Its first
three-argument attempt failed before acceptance. Both successful and rejected
candidate evidence remain available. This is still instruction evidence, not
a physical cache/bandwidth or FPS measurement.

The aligned normal build `7da61f9737b8` /3dsx `d48e49ce1606` completes the
ordinary cockpit route in 75.30 host seconds with intact inspected output,
clean shutdown and unchanged saves. Its sampled full-frame average is 19.5199 ms
versus the event-wait parent's 19.6477 ms, with a 36.234 ms worst interval.
That small, phase-varying difference is not a causal performance claim. Memory
budgets match the event-wait parent. It is not deployed; the replay application
is not rebuilt. The 16.67 ms target remains unmet, as does the deferred-surface
0.5 ms maximum submission gate (observed .52695 ms). No new visual family should
be stacked on that unresolved gate. Next investigate read-ahead overlap and
remaining CPU/copy costs rather than repeating surrogate CRC benchmarks.

The eight-frame profile of normal build `3b777c0adf19` is in
`artifacts/native/world-deferred-cockpit-profile-adaptive-new/`. It uses the
same ordinary cockpit HID and private save fixture as the deferred-surface
comparison. It reaches Game Select, drains logs and exits normally. The
profile covers render-counter values 900 through 907, with screenshots outside
that window. Its elapsed time is excluded from FPS comparisons.

Core 0 records 66,047,006 ARM instruction encounters. `padUpdate` accounts for
14.83%, `PADClamp` 7.12%, `fh_dvd_pump` 4.87%, `fh_movie_prefetch_poll` 1.75%,
and `crc32_z` 13.46%. There are 36,149 input polls and eight CRC calls. These
are scheduler-window instruction encounters, including failed predicates;
they are neither hardware cycles nor a measured wall-time decomposition.
The source synchronous `DVDRead` loop in `fileio.c` polls input/reset/DVD
status without a normal-path sleep. Call-site attribution of the observed
polls still needs proof before changing that scheduling contract.

The movie decoder checks each decompressed frame's CRC. That validation stays
in place. `fh_movie_crc.h` replaces only the CRC calculation with eight-byte
slicing using the same CRC-32/ISO-HDLC polynomial, initial/final complements
and incremental convention. Its generated tables contain polynomial constants,
not game data. No prepared assets or conversion recipes change.

The candidate adds 8192 read-only table bytes, uses 32 bytes of ARM stack,
and adds no heap, linear RAM, VRAM, worker, GPU command or allocation.
Unaligned prefixes and tails are checked without out-of-bounds loads;
`memcpy` avoids aliasing violations. Pinned Melee's explicit resource/cache
ownership was reviewed; this CPU-only immutable calculation requires no GPU
publication or cache-maintenance change.

`test_movie_crc.py` passes 65,600 ASan/UBSan comparisons against zlib, including
all offsets 0–15, all lengths 0–4096, full RGB8/RGBA8 frame sizes and incremental
updates. `profile_movie_crc_arm.py` executes the compiled candidate and linked
SDK zlib on ARM11 Unicorn. For an aligned 196608-byte frame it records
1,110,895 instructions for zlib and 860,182 for the candidate (22.57% fewer).
An unaligned full frame also improves; a seven-byte input regresses by two
instructions. This is not a frame-time prediction: memory/cache costs and
asynchronous waits are not modeled by instruction counts.

The isolated builder changes the movie adapter call and adds the header to the
accepted `world-deferred-timing-v2` snapshot. Root mixed work is never built
directly. Whole-frame runtime comparison, physical acceptance and the dinosaur
rescue crash remain separate requirements. Nothing here establishes that crash's
cause or repairs it.

## Local integration result

Normal source `3a39b443c810`, 3dsx `18cd3a3453ad`, is built and not deployed.
Its isolated archive is `movie-crc-v1-console-setup-builds`. Only the header
and movie adapter change; the game archive and all linked GPU programs match
the parent. Old80/New124 packaging and the setup recipe remain unchanged.
Root and archive restoration passed. The replay application is not updated.

The ordinary HLE cockpit run passes in 76.81 host seconds, with clean exit,
drained logs, no movie decode errors and unchanged private saves/assets.
The final capture was inspected: cockpit geometry, textures, Fox/fur, the
transparent Game Select panel and HUD remain visible. This is not source
pixel parity. Sampled frame windows 1000–1600 average 20.3712 ms versus
22.4924 ms in the earlier same-route parent; the raw FPS change is +10.413%.
These are separate phase-varying samples, not a causal 10.4% improvement claim
or hardware prediction. The worst sampled interval is still 37.36 ms and
the 16.67 ms target remains unmet. GPU-reported averages are .7374 versus
.7231 ms; those are emulator values.

Observed heap, linear and VRAM figures are unchanged: 54,180,888/71,303,168
heap bytes, 17,125,120/25,165,824 linear bytes free, and 4,690,944 VRAM bytes
free. Process commitment rises exactly 8192 bytes to 104,800,256/130,023,424.
Measured stack upper bound remains 72,936/524,288. Old-console cache behavior
and physical gains remain untested. Evidence is `movie-crc-test/runtime.json`.
Next investigate the synchronous movie-read polling path and its actual wait
cost, without changing simulation or dropping validation.
