# ISO conversion performance

13 October 2026: local setup optimization; no console time claim or deployment.

The user revised the acceptance target to **under70 minutes** on13 October,
without compromising quality. The earlier complete68m49 New-mode Azahar run
meets that revised emulator threshold. Physical duration remains unmeasured;
Old 3DS times must be reported separately. References below to the original
one-hour goal describe the historical target, not the current acceptance gate.
Finish validation of the readback candidate already running, then return to
remaining port work. Further geometry-hint integration is optional and is not
required solely to close the former one-hour gap.

Current candidate fully verified (13 October): `d23068f52b22`, shared3DSX
`f95810959928` / CIA`772720740117`. Fresh ISO-only run
`setup-readback-fresh-complete-new-v1` committed at3778.369890hostseconds
(**62m58.370s**), exited normally at3780.457488, and meets the revised70-minute
emulator threshold. Application work3770594ms: source322696, original202703,
performance559708, textures1835313, movie716920, publication133254. Prior movie
phase1057053ms; whole-run observed improvement350.451270seconds. No physical
conversion time is inferred.

The exact archived transaction dependencies were rebuilt as
`install-performance-readback-full-reference` (40.11hostsec). Its spool,
topology and geometry hashes match the earlier reference; the strict source
comparison was retained. Full ARM conversion verification then passed all
geometry/cache/topology/3754 texture variants and all1797 movie pixels/audio,
source identity, packed header, links and padding. Allocator arena peak10813440B;
normal/linear reservations are not live usage. Private ISO/save unchanged.

Exact-candidate New full movie/cockpit handoff144.16hostsec and Old CIA-content
short handoff39.60s pass; captures reviewed, queues drained, no reported errors,
source/save sentinels intact and owned exits. New heap53928104/71303168,
linearfree18633216/25165824; Old heap39050784/50331648, linearfree3955200/10485760.
Both VRAMfree4723712, mainstackupper72936/524288. Process New103985152/130023424,
Old68841472/83886080. This is startup/playback acceptance, not complete gameplay,
AV sync, renderer parity or physical FPS.

Asset-free package `foxhollow-iso-setup-191072361db3f64b.zip` contains the shared
apps, instructions, provenance, licences and conversion evidence. Full SHA256
191072361db3f64b2eb2f86569ad4b7e5d42c488bcf79fe7bd52883a738d87f2;5830644bytes.
Deployment selector/reproducible rebuild checks pass;9 incompatible/private-data
mutants rejected (`iso-setup-readback-package-v1-test`). Its selector-only result
has `full_conversion_validated=false`; complete asset acceptance is separately
in the run's full verifier and packaged manifest. New223 FTP refused before
login at074803UTC; no files changed, not deployed. All owned processes exited.

Geometry-hint integration prototype (not executed or enabled yet):
`install_geometry_search_patch.py` and the isolated builder's opt-in
`--geometry-hint-validation` require a passing full-output comparison, source
and header dependency hashes, the exact metadata hash and the reducer contract.
The generated read-only table contains hashes/IDs/sizes only. A small table
descriptor belongs to the startup spool and is cleared by its existing close;
there is no dynamic metadata allocation. These files are syntax-checked only;
the full prototype test and ARM integration remain deferred until the ongoing
conversion timing run exits. No new recipe compatibility exception is added.

The directory-only `install-exact-texture-search-audit-v1` reconfirms the earlier
encoding-work audit: only30 CMPR images/3,716 blocks reject ETC and retain exact
RGBA. This is a small opportunity compared with2,918,508 accepted ETC blocks;
no new direct-exact shortcut was added. The0.197hostsec directory audit and a
0.147sec read-only screenshot overlapped the live conversion, with no concurrent
compiler, benchmark or second emulator. `progress-textures-late.png` was inspected:
42m04elapsed, estimated66% overall, rough23m remaining, separate current-image
progress5%. This is a live estimate, not a completed measurement.

Follow-up profiling and implementation (13 October):

- `install-movie-progress-arm-v2-new` measures the archived actual ARM codec,
  task callbacks, ETA and menu formatting with counted display hooks. Three
  source frames in ABBA order save about 24–26ms per frame from the progress
  changes: 173.2–180.8ms becomes 149.3–154.4ms. Encoded bytes remain exact.
  LCD publication, SD output and assembly are excluded; this cannot explain
  the entire remaining nine-minute gap. V1's compile failure is retained.
- `install-movie-io-arm-v1-new` measures actual transaction writes/checkpoints
  using 16 copies of each matching prepared frame payload. For the 2.19MB case,
  sixteen checkpoints cost 1810.3ms; two checkpoints cost 238.3ms. Write time
  stays about 62ms and final readback/validation about 1370.5ms. Files are exact.
  These are isolated Azahar service timings, not physical SD or whole-movie
  predictions. Both experiments exited and cleaned up their owned processes.
- The movie producer now checkpoints every eight completed frames or at the
  1MiB threshold, whichever comes first, and always at the last frame. No
  additional buffer/worker is allocated. A normal cancellation commits all
  completed records, including cancellation during the next frame's decode,
  resize or encoding. Power loss may replay the current uncommitted batch:
  at most eight frames, with the byte threshold exceeded by at most one frame.
  Failure never advances a successful cursor; the existing recovery truncates
  the uncommitted suffix. Assembly checkpoint policy is unchanged.
- `install-movie-checkpoint-batch-v1-test` exercises the production producer
  under ASan/UBSan: 10 cancellation/resume/failure cases, an abrupt process
  termination, three in-frame cancellation phases and dense later frames.
  Every committed prefix matches the independent complete reference.
  `install-movie-checkpoint-full-v1-build` freshly converts all 1797 frames;
  movie SHA256 `1ea62624e7c76d4e72d5628f953bd10f5484e82d15549c3c75633d48655a0415`
  and spool SHA256 `2bb64ae1c328dc95e2bc2b7d9eec10ad655fbf74af0366afe20a43cecc617c98`
  match the prior complete host outputs exactly. Its 25.07 host seconds is not
  ARM timing. No prepared prefix was injected into this full conversion.
- `install-movie-checkpoint-recipe-v1-test` binds compatibility to the exact
  reviewed source hashes and that full-output/recovery evidence. All 25 source
  mutations and policy/library/arithmetic changes invalidate compatibility.
  Actual compiled hashes remain recorded; no broad dependency exclusions.

The completed 68m49 fresh ARM run below predates this checkpoint change.
No sub-hour result or physical conversion improvement has yet been measured.
The subsequent local readback change below combines the movie's separate
whole-file checksum and semantic passes without dropping either check.

Combined readback follow-up:

- New `fh_install_readback.h` exposes an opaque sequential reader. Every read
  updates the transaction's whole-file CRC. Completion requires successful
  semantic validation, all expected bytes consumed, matching CRC, true EOF,
  successful close, and the existing rename/completed-journal protocol. A
  callback cannot borrow the file handle, seek, or accidentally bless an
  unconsumed tail. An attempted out-of-bounds read poisons the reader even if
  the callback ignores its error. Existing transaction entry points are
  unchanged apart from line endings; only the movie selects the new API.
- Movie spool and final-pack validators retain header/source identity,
  dimensions, frame links, payload CRC, decompression, decoded CRC and padding
  checks. Fresh conversion reuses its already-built frame index for assembly;
  resuming a completed spool still reconstructs the index from disk. No new
  worker, bulk buffer, game/GPU operation or converted-asset seed is added.
- `install-movie-readback-v2-test`: 35 actual production checks pass ASan/UBSan
  over both complete 1797-frame files, corruption with recomputed whole CRC,
  truncation, tails, under/over-consumption, CRC failures and refused publication
  over an existing destination. V1's initial source-prefix assertion failed
  on CRLF/LF conversion before compiling; V2 explicitly checks normalization.
- `install-movie-readback-resume-v1-test`: actual SDK link and ASan/UBSan writer
  pass two assembly resumes and two uncommitted-tail recoveries. All1797 frames
  match the reference. This test seeds1795 private fixture frames and reuses
  the last two source-identical frames; it is not fresh encoding evidence.
  Separately, `install-movie-readback-full-v1-build` freshly converts every
  frame from the ISO with no prepared prefix. Whole movie and spool SHA256
  remain exactly equal to the reference; 25.87hostsec is not ARM throughput.
- `install-movie-readback-arm-v1-new`: ABBA actual ARM transaction benchmark
  with identical batch8 writes and readback comparisons. For2.19MB, final
  validation1370.520ms becomes802.255ms (about41% reduction); total2022.2ms
  becomes1453.9ms. The239840-byte case467.982ms becomes367.700ms. All bytes
  exact, 21.40hostsec, owned emulator exited. These timings include emulated
  filesystem services and are not physical SD or whole-movie measurements.
- `install-readback-recipe-v1-test` preserves all four asset keys only for
  the exact reviewed append-only owner/header and movie hashes.167 source
  dependency mutations plus per-stage policy/library/arithmetic changes
  invalidate the relevant key; actual source hashes remain in build records.

Pinned Melee `port/engine/dvd.c` and `port/3ds/card_storage.c` were consulted:
retain owned operations, recognize short reads, and publish replacement files
after complete writes. Foxhollow keeps its existing stricter fsync, CRC and
journal protocol; this startup-only change adds no asynchronous OS mechanism
and never touches gameplay saves.

Shared readback candidate `d23068f52b22` is built locally: 3DSX `f95810959928`,
CIA `772720740117`, `normal-setup-readback-v1-builds`. The isolated builder
restored all shelved sources/archive. Verification covers all575 archived
sources, the linked seed, exactly seven changed source files versus the
measured seeded baseline, unchanged game/renderer, shared Old80/New124 CIA,
all four unchanged asset keys, and five forged build-compatibility cases.
Not deployed; gameplay/runtime reuse of this exact candidate is unmeasured.

`setup-readback-fresh-complete-new-v1` is now running a **fresh ISO-only** New
Azahar conversion with this candidate, no prepared assets or copied journals.
It is bounded at7200hostsec and stops after the complete bundle commits.
Do not infer completion or a sub-hour result from this running record. Preserve
the existing process rather than restarting it when a tool poll is quiet.

During that run, a read-only header audit of the previous checked performance
spool (`install-reduction-hint-audit-v1`, 0.02369hostsec) found 1643 unchanged
models out of2110: 867/1267 world models and776/843 actor models. Their source
triangles total783374 of1383658. These counts do not measure saved time; payloads
were not reread in that audit. Its small overlap with the active timing run is
recorded explicitly.

Unintegrated geometry-hint prototype: `install_geometry_hints.c` and its header
describe `FHKEEP01`, storing only source SHA256/ID/byte length plus disc/reducer
hashes and exact ratio/error bits. The optional path recomputes descriptor,
triangle and alias metadata from the ISO and retains the model unchanged,
avoiding an unsuccessful simplifier search. Unknown sources or policies use
the existing reducer. It borrows the digest the caller just computed; the
performance caller already computes that SHA before reduction. This binding
must be proved in integration. There are no geometry bytes in the proposed
metadata, and no changes to the original reducer, active build, game or GPU.

`prepare_install_geometry_hints.py` and `test_install_geometry_hints.py` are
written but **not yet run**. The latter will validate the production reader,
negative cases, cancellation, actual caller SHA binding, and full generated
spool/topology/geometry against the independent reference under sanitizers.
No metadata has been exported, no app calls this path, and no quality or
performance acceptance is claimed. Compilation and heavy tests are deferred
until the current conversion timing run finishes. Reuse of already-converted
original geometry for unchanged models remains only a possible subsequent
optimization, with no implementation yet.

Live UI acceptance during the readback run: inspected `stage-textures.png`
shows build d23068f52b22, overall approximately26%, elapsed18m05s, rough ETA51m,
and a separately labelled current task. A later read-only capture,
`progress-textures-mid.png`, shows overall approximately39%, elapsed28m04s,
rough ETA45m and current reduced-image task40% (103/256). This confirms visible
whole-job progress distinct from per-image progress; it does not validate the
ETA's accuracy or completion time. `capture_install_progress.py` checked the
live PID's exact binary and working directory, then captured only its existing
Xvfb display without input, pause or another emulator. Capture cost0.144hostsec
(~0.66s including script startup) overlapped the timed run and is recorded.

Shared follow-up build `14c6adf97112` (3DSX `f63ed1a8aebf`, CIA `01bc0c4fef2d`)
in `normal-setup-checkpoint-batch-v1-builds` is built, not deployed. All 574
archived source hashes, linked seed, unchanged game/renderer, Old80/New124 CIA
envelope and the five exact changed files pass verification. All four asset
recipe keys remain unchanged; five forged compatibility cases are rejected.
`setup-checkpoint-batch-reuse-new-v1` passes in 59.01 host seconds using the
genuine converted installation, with no reconversion. Cockpit image inspected,
31,098 actor draws, zero material errors, log drain and owned exit pass;
source/save sentinel/fixture unchanged. Heap 53,928,008/71,303,168; minimum
linear free 18,635,264/25,165,824; VRAM free 4,723,712; stack upper bound
72,936/524,288; process 103,985,152/130,023,424. This is a compatibility check,
not another conversion measurement. Prior Old-mode evidence is for the
preceding binary, not a run of this new binary.

Fresh hinted run COMPLETE: `setup-search-hints-fresh-complete-new-v1` reached
its committed bundle after **4,128.821 host seconds (68m48.821s)** and exited
normally at 4,131.006s. Fresh ISO only, no copied checkpoints or prepared assets.
`conversion-complete-verification.json` passes every geometry/texture byte and
all decoded movie pixels, audio, links and padding. Source/save unchanged.
The one-hour target is missed by 8m48.821s in this emulator; physical time is
unmeasured. Application phases total 4,110,818ms (68m30.818s):

| Phase | Milliseconds |
|---|---:|
| Source verification | 322,712 |
| Original geometry | 202,641 |
| Performance geometry | 560,210 |
| Textures | 1,835,028 |
| Movie | 1,057,053 |
| Publication/verification | 133,174 |

All 3,754 texture variants and the complete
84,776,872-byte texture pack match the independent reference exactly
(SHA256 `51bd95568d44d6a4775073a80ef2465059b0edc0ac3d3d7e4d91f58286c1e97a`).
Movie stage began at 2,936.998490 host seconds; texture-stage directory
observations span 1,836.627112 seconds (about 30m37s). These observations include
the two-second monitor interval; use the final application phase table above.
The earlier baseline's texture spans total 93m13s across two resumed attempts,
including recovery; do not treat their ratio as a tightly paired speedup.
New-mode genuine converted-asset handoff `setup-search-hints-handoff-movie-new-v1`
passes 145.36hostsec, full movie window and cockpit, inspected late/end captures,
179,778 actor draws, zero material errors, clean drain/owned exit and untouched
source/save fixture. Periodic telemetry reports at least3,200 prepared movie
decodes; this is not AV-sync or physical SD acceptance. Heap53,928,104/71,303,168,
linear minimum free18,633,216/25,165,824; VRAM free4,723,712; measured stack upper
72,936/524,288; process103,985,152/130,023,424. Old-mode short handoff also passes
44.07hostsec (`setup-search-hints-handoff-old-v1`): cockpit/touch-stick capture
inspected,31,182 actor draws,zero material errors,drained owned exit; heap
39,050,784/50,331,648,linear free3,955,200/10,485,760,VRAM free4,723,712,
stack72,936/524,288,process68,841,472/83,886,080. Neither is a console FPS claim.

Local follow-up built (not in the measured fresh conversion): source3f1537c3b2e8,
3DSX1d4a36f3bff5 / CIA975b7bfcb7c4, `normal-setup-progress-batch-v1-builds`.
All574 archived sources/linked metadata verified; exactly four changes versus
the measured binary (task progress, RGB8 progress, ETA header, recipe metadata).
Game archive/renderer/shaders and all four asset recipe keys unchanged; shared
Old80/New124 envelope verified. New-mode existing-install reuse
`setup-progress-batch-reuse-new-v1` passes52.10hostsec, inspected cockpit capture,
31,098actor draws/zero material errors, unchanged fixture/source/save, clean
drain/owned exit. No reconversion. This does not measure fresh conversion speed.
Old-mode loading evidence above is for the preceding binary. No deployment.

RGB8 movie progress
is now reported every 16 tiles instead of every tile. Pixel conversion and
compression code are byte-identical after removing this exact callback edit.
`install-movie-progress-batch-v1-test` checks eight complete frames against
the archived producer, 78 cancellation cases and unchanged inputs under
ASan/UBSan, plus ARM compilation. Encode callbacks fall from 1,025 to 65 per
frame; cancellation can copy at most 15 extra tiles (960 pixels), with no I/O
or worker wait inside that interval. No runtime speedup is established yet.
`install-movie-progress-recipe-test` preserves the movie recipe only for this
exact reviewed source hash, retaining actual source hashes in build records;
all 25 dependency mutations and policy/library/arithmetic changes invalidate
it. The replacement's existing-install reuse passes as recorded above. These short host checks (~5.1s plus
0.6s), and the complete texture proof (~2s), overlapped the fresh movie stage.

Completed baseline (13 October, `setup-lattice-rgb8-resume-complete-new-v1`):
both genuine attempts total 8,252.839 host seconds including menus. Application
phases total 8,226,133 ms; removing proven repeated source verification
(318,635 ms) and unchanged completed-geometry verification (73,741 ms) leaves
**7,833,757 ms = 2h 10m 33.757s**, 70m 33.757s over the one-hour target. This
retains partial-stage recovery and recorded host contention, so is not an
uninterrupted fresh-run duration or physical prediction. Textures total
5,593,250 ms (93m13.250s); movie 1,033,674 ms; final publication 131,537 ms.
Original/performance geometry and all 3,754 texture variants match references.
The complete byte-level validator rejects the movie's different compressed
size. Follow-up `movie-pixel-comparison.json` proves all 1,797 decoded native
frames, audio and source identities exact; 1,786 compressed payloads differ.
`conversion-complete-content-verification.json` now passes, additionally checking
each journal's exact file hash/CRC and reconstructing the movie header/frame
links/padding from its verified spool. The initial byte-level failure is retained.
The emulator exited normally, source/save/prior checkpoints unchanged.

The authorized metadata-only search shortcut is implemented locally; see
[search hints](INSTALL_TEXTURE_SEARCH_HINTS.md). Every ETC block passes exact
output comparison, including the production API under sanitizers. The paired
sample is 3.81x faster on host CPU; this is separate from the ARM check below
and does not establish a whole-conversion improvement.
Shared new build cc0a00b4c399 (3DSX9697ab8dd604/CIA7408845f17e0) is built,
verified and not deployed. Focused paired ARM timing: serial2641.266->772.620ms,
worker1485.828->419.029ms with source/output hashing included. Actual startup
percentage/ETA capture and cancellation pass. A fresh complete ISO-only run
`setup-search-hints-fresh-complete-new-v1` completed within its3hour safety bound;
its measured duration and acceptance are above. No seeds contain converted assets. The
baseline content verification (~10s host) overlapped this run's source check.
The read-only movie-validator regression added another7.55s of host work around
the source/original-stage boundary. It rejects15 virtual corruptions, including
changed pixels/audio/source identity with valid checksums, damaged headers and
frame links, nonzero padding and trailing data. Both genuine movie files remain
unchanged. Existing1797-frame lossless-difference acceptance is preserved.

The same follow-up also includes `install_task.c` suppression of
identical overall-progress metadata events within a phase. The new movie ETA
path otherwise repeats its same overall fraction for every native tile.
`install-progress-coalescing-v1-test` runs the actual before/after callbacks
under ASan/UBSan: all1,841,926 ordinary events have identical ordered labels,
values and cancellation results, while metadata events fall1,841,926->1,798.
Five cancellation points and first/final phase values are preserved; the ARM
task unit compiles. This is a callback-count improvement, not a measured runtime
speedup. The current fresh benchmark binary is unchanged. This short check
(~6hostseconds including compile) overlapped its performance-geometry stage.

A separate build-tool fix now preserves the lattice build's texture recipe for
the exact reviewed ETA/progress-only edit. `install-progress-recipe-test` proves
that removing only the added event number, metadata helper and three callback
sites reproduces the previous two source files byte-for-byte. The compatibility
rule matches both complete new source hashes; any other edit is still versioned.
All 36 dependency mutations and policy/library/arithmetic mutations invalidate
the recipe. Actual compiled hashes remain recorded in `sources`; the explicit
`recipe_sources` field records the previous identity used for the key. No codec,
pixel quality policy, journal validation or runtime asset reader was changed.
This has not been built, deployed or checked for actual checkpoint reuse yet.
The existing ETA v2 binary still has its old changed recipe. Build the next
candidate after the ongoing conversion ends, and verify it with
`verify_install_eta_build.py --label <new-build-label>` before runtime reuse.
The <1-second host identity check overlapped the current conversion; it was not
a timing benchmark and no second emulator ran.

03:21 UTC update: the same resumed attempt remains active. Its latest committed
prefix verifies 1,707 records / 2,002 of 3,754 texture variants byte-for-byte,
including 2,089,380 of 2,918,508 retained ETC blocks. This output-area count is
not an overall percent or a time estimate. The 03:16 private-display capture
shows active texture encoding; movie/publication completion is still pending.
No new emulator was started and no running converter was replaced.

`install-cross-image-cache-v1-audit` checks whether a larger exact-block cache
could avoid substantial encoding. Across 1,135 aligned original CMPR images,
the existing image-local cache hits 83,722 of 2,244,696 blocks. Two private
cross-image caches of 1,024 / 4,096 / 16,384 entries avoid only another
3,784 / 7,134 / 10,679 calls: 0.175% / 0.330% / 0.494% of baseline misses,
for 155,648 / 622,592 / 2,490,368 bytes including packed output storage.
This uses reference-pack order, excludes reduced/padded images and is not a
timing benchmark. No larger cache was added. The audit (~2 seconds) and the
prefix proof (~1 second) overlapped the resume; retain the contention caveat.

Timing interpretation audit (13 October, `install-emulator-clock-audit`): the
seven relevant source files match the recorded emulator base commit
`9e6f523a57fac9564ac0bf8286db3c3702d301ec`, normalizing checkout line endings.
The worker calls `threadCreate(...,2,...)`; the emulator's thread constructor
retains that processor assignment. However SVC0x11/GetCurrentProcessorNumber
is unimplemented in this revision, so the app's `worker,core=0` field is **not**
proof it ran on core0. Do not remove parallel conversion based on that field.
New mode creates four schedulers, while the timing scale is driven by the
configured CPU percentage. The `running_804MHz` flag is stored/read for platform
reporting, not applied to that timing scale. This is another reason that a
100%-speed New-mode emulator run cannot establish New3DS console duration.
No clock adjustment, multiplier, application or emulator change was made.
The audit is read-only source/build-provenance evidence, not physical validation;
its ~8-second source scan overlapped the resumed run around03:09UTC.

Update at 03:00 UTC: that fresh run terminated cleanly at its one-hour bound
(3,576.020504 host seconds including exit) without completing textures. Final
prefix: 1,286 records / 1,476 exact variants, 1,382,308 retained ETC blocks.
The target is not met in this emulator attempt; physical duration remains unknown.
At the user's explicit request, `setup-lattice-rgb8-resume-complete-new-v1`
continues the **same binary** from copies of those genuine checkpoints, with a
three-hour safety bound. It is the sole emulator (handle83205, PID726).
Host UI tests/builds overlapped roughly the first five minutes of this resume;
do not describe its host duration as uncontended. The old evidence is unchanged.
`install_duration.py` accounts for the complete chain after completion, with
repeated source hashing and byte-proven unchanged completed-stage revalidation
listed separately. It does not turn a resumed duration into a fresh-run claim.
The full verifier's `--allow-resumed` is explicit; its default remains fresh-only.

Separate local build `normal-setup-eta-v2-builds` adds a rough whole-job percent,
elapsed time and remaining ETA; see ISO_SETUP.md. It is **not** the binary under
measurement and has not been deployed or captured in the emulator yet. Estimated
stage weights must not be cited as measured overall completion or console timing.

Previous observations of fresh run: `setup-lattice-rgb8-fresh-new-v1`, started 13 October
01:47:34 UTC, one-hour bound with ordinary HID cancellation if incomplete.
It uses only the original ISO; no prepared assets or resume journals are seeded.
Original geometry began at 330.880425 host seconds; performance began at
532.588602 seconds; textures began at 1,088.848567 seconds. Both complete
geometry stages pass byte comparison, including their compressed caches and
all 2,102 rendering packets. Performance output retains the same 467 patched
sources and 1,376,568 triangles. At 02:08:53 UTC, the first 391 committed texture
records / 400 variants also match the independent 3,754-variant reference.
At 02:25 UTC the committed prefix reaches 733 records / 817 exact variants,
including 638,096 of the reference's 2,918,508 retained ETC blocks (21.86%).
The verifier now reports this output-area measure. It excludes cache hits,
rejected attempts, exact formats and scans, so it is not a linear time estimate.
These were partial texture observations, not completion estimates. That test is now terminal;
do not infer completion or console timing. Other emulators, builds and
benchmarks are idle. A brief (~1.2 second) package assembly overlapped source
verification; its completed-stage quality check took about 1.2 seconds during
performance conversion. The performance-stage verifier took about 1.5 seconds
and the first partial texture verifier about 0.85 seconds during texture work.
The run was otherwise left isolated until two low-priority host codec checks
around 02:18 and 02:20 UTC (roughly tens of seconds total, plus a subsecond ARM
syntax check). Record that overlap when assessing this run's wall duration;
it must not be described as completely uncontended. No second emulator ran.

Additional prototypes remain outside the application:

* `install-etc-plane-bound-v1-test`: 632 blocks, three paired passes, exact
  output and high-quality ASan/UBSan pass. Whole-plane pruning adds 32 bytes;
  high-quality host CPU changes 0.397857 to 0.396933 seconds (0.23%). That does
  not establish a useful gain, so it was not promoted or sent to a console.
* `install-etc-channel-histogram-v1-test`: the same comparison and sanitizers
  pass, but weighted scalar lookups change host CPU 0.407743 to 0.421189
  seconds (3.30% slower). No application promotion. This is distinct from the
  earlier weighted RGB candidate: only lower-bound preparation changes here.

`install-subblock-reuse-v1-audit` checks 1,135 aligned original-size CMPR images
with the pinned palette decoder and current 256-entry full-block cache first.
Out of 2,244,696 source blocks, that cache reuses 83,722. A hypothetical bounded
128-entry cache of unordered eight-pixel partitions finds 482,332 potential
hits out of 8,643,896 queries (5.58%). This deliberately ignores mode constraints
and early exits, excludes reduced/padded images, and is not actual saved encoder
work. No sub-block cache was added to the application. The compile/audit took
about two host seconds during this run; retain that overlap caveat too.

The latest bounded read-only New223 timing collection at
`console-visual/20261013-022935-223` was refused before login; no remote changes.
Current console conversion duration remains unknown.

These host checks overlap the live conversion and do not predict physical
performance. Existing low/medium-mode sanitizer exclusions still apply; setup
uses high quality. Both original sources/results are preserved for review.

The checked development package is
`build/console-validation/foxhollow-iso-setup-ed726864a20be261.zip` (4,465,506 bytes).
It contains the shared binaries, instructions/build records and licenses;
no private game assets, firmware, saves or graphics preferences. It has not
been deployed. The bounded New223 timing collection at
`console-visual/20261013-015103-223` was refused before login; no remote files changed.

Latest local candidate: **3DSX `3a407e49ac05` / CIA `afd36ca65186`**,
startup build ID **`c341525a9535`**, under
`normal-setup-lattice-bound-v1-builds/console-setup`. Built, restored and
verified; **not deployed**. It combines the experimental lossless RGB8 movie,
existing asynchronous movie reader, bounded texture checkpoints and the
new exact-search encoder. The runtime renderer/game archive is unchanged.

The encoder uses an immutable 192 KiB scalar-distance table, then rejects
an entire red-coordinate search row if its conservative error lower bound
cannot improve the incumbent. This cannot remove a successful refinement:
refinement starts only after a strictly improving candidate. Candidate order,
tie rules, search radius, clamping and quality are unchanged. The row minimum
adds 32 temporary bytes. Only this startup codec is compiled with `-O3`;
fast-math stays off, contraction stays off and aliasing settings are preserved.

The focused actual ARM 128x128 test, same image/output:

| Candidate | Serial ms | Worker ms |
| --- | ---: | ---: |
| Channel bound, previous optimized codec | 5,740.06 | 2,968.76 |
| Scalar table only | 5,110.70 | 2,652.02 |
| Previous codec with `-O3` only | 4,621.83 | 2,407.89 |
| Table + row bound + `-O3`, now built | 2,641.91 | 1,486.06 |

These are isolated Azahar component timings, not physical seconds or a
whole-installation estimate. Worker stack upper bound is 23,988/131,072 bytes.
The whole-collection sample checks 9,528 blocks across 1,752 texture images
and 12 movie frames, byte-exact including 8,610 existing-pack blocks;
high-quality ASan/UBSan passes. Scalar bounds have the exhaustive and mutant
checks described below. Neither comparison covers every block of every asset.
The row-bound v1 prototype had a compile-only two-argument minimum/maximum
error; v2 fixes it and preserves that failed test evidence.

`install-texture-payload-arm-v4-new` also passes all 130 mixed-image writer,
pack-equality, interruption, cancellation and failed-sync cases. Sync counts
match the host: 262 eager, 8 batched, 102 with the small test byte bound.
Earlier probe versions used libctru's small default stack and timed out at
pack assembly; v4 uses the actual installer's 512 KiB reservation. No production
stack change was made. V1's callback typedef mismatch was test-only and fixed.
The successful ARM test overlapped a build and is correctness evidence, not
filesystem latency evidence.

The exact application passes its uninterrupted-movie consumer check in
`setup-lattice-rgb8-full-movie-new-v1` (140.14 host seconds), then the shorter
Old-mode check `setup-lattice-rgb8-short-old-v1` (39.78 seconds). Both use the
explicitly synthetic `install-lattice-game-fixture-v1`, which is not a
converted-on-ARM installation and must not be reported as one. New reports at
least 3,136 prepared decodes at periodic checkpoints. The ready screen, movie
middle/late/end window, title/menu and Old game captures were inspected.
This is not frame-by-frame display or full audio/video synchronization proof.
Both exit cleanly with queued GPU work drained; ISO, saves and fixture are
preserved. All owned processes are now terminal.

Memory for New / Old respectively: normal heap peaks 53,918,016 / 39,040,688
bytes within 71,303,168 / 50,331,648 reservations; minimum linear free
18,633,088 / 3,955,072 within 25,165,824 / 10,485,760 reservations. Process
commitment peaks 103,329,792 / 68,186,112 within 130,023,424 / 83,886,080 grants.
Both retain 4,723,712 free VRAM bytes and measured stack upper bound 72,936
of 524,288 reserved bytes. This accounts for the added codec table without
claiming whole-game worst-case memory coverage.

Consolidated evidence: `artifacts/research/install-lattice-acceptance-v1/result.json`.
The next whole-conversion measurement must use this substantive new candidate;
the previous failed one-hour test does not measure it. No sub-hour result or
physical speedup is claimed, and no console files have been replaced.

## 13 October: separable ETC bound and asynchronous lossless movie playback

Final fresh run: `setup-channel-rgb8-fresh-new-v1` **missed the one-hour gate**.
The deadline requested ordinary HID cancellation at 3,571.99 host seconds;
the menu exited normally at 3,576.38 seconds and the owned emulator retired.
No movie conversion or final bundle publication was reached. Saved progress,
ISO metadata and the isolated save sentinel remain intact. The top-level test
correctly returns failure because its requested bundle was not completed.
Do not restart it unchanged or present the cancellation as installation success.

The actual application phase report contains inclusive milliseconds:
source 318,702; original geometry 200,514; performance geometry 556,198;
textures 2,493,096 **partial**. These include execution, services, scheduling
and UI. Short host tests/builds overlapped textures as described below;
this is not uncontended PC timing or physical-console measurement.
The completed geometry proofs pass. The final committed texture prefix has
743 records / 832 native variants, all byte-exact against the 3,754-variant
reference. Cancellation left a valid durable cursor, not a claimed full pack.
The allocator arena watermark is 10,813,440 of 71,303,168 reserved normal bytes;
25,165,824 linear bytes are reserved, not a measured usage peak. The worker
retired with measured stack upper bound 23,968 of 131,072 bytes. Azahar reports
processor zero for its unimplemented processor-ID query; affinity was requested
on core 2, so that value is not proof of physical worker placement.
The before-cancel capture was inspected and shows the current texture's local
progress, not whole-installation progress.

Earlier live observation (13 October, about 01:10 UTC): source verification reached
original geometry at 324.53 host seconds; performance geometry began at 524.38;
textures began at 1082.61. At that time the test was **incomplete**, and no under-hour
result is established. Complete original output has independently passed:
2,110 records, 2,102 rendering packets, the full native pack, compressed cache
and empty original topology are byte-identical to their checked references.
The exact archived-source performance reference under
`install-performance-channel-full-reference` has completed. All 2,110 ARM
performance records, 467 modified sources, topology, geometry and cache match
it byte for byte. The policy removes 7,090 of 1,383,658 source triangles;
conversion settings have not been relaxed. Do not substitute the older
reference whose converter dependencies differ.

The fresh run initially had no competing tests. During its texture phase,
short host correctness/codec tests, the host performance-reference build and
the texture checkpoint application build
ran concurrently. This overlap must be retained when reporting its wall time;
it is not an uncontended timing comparison. There is still only one emulator,
with unchanged binary, 100% configured clock and genuine ISO-only inputs.

Additional **local opt-in, built/verified but not deployed** improvement:
`install_texture_payload_patch.py` groups complete texture records under the
existing durable transaction, bounded by 64 records or 1 MiB. Explicit cancel,
finish and bank changes commit progress; unexpected interruption regenerates
only the uncommitted tail. Encoding, RMSE checks, exact fallbacks, opacity and
layout are unchanged. It adds four owner bytes and no worker/buffer. Checked
builder option is `--texture-payload-batch`, with all modified sources restored
in its existing `finally` path.

`install-texture-payload-v3-test` passes actual mixed CMPR/NPOT/I8/I4 writer,
ASan/UBSan, ARM compilation, two interruption/resume cases, cancellation inside
the next encoder, failed flush and full-pack equality. Filesystem sync counts
are **262 eager / 8 record-batched / 102 with the smaller test byte bound** over
130 images. These are counts, not SD timings or whole-conversion savings.
The missing-cancel-flush mutant fails as intended. Earlier test-only failures:
V1 patch anchor accidentally included the separate pack-assembly close;
an intermediate Python parenthesis error prevented execution; V2 passed the
runtime test but ARM compilation found the old transitive test-copy header.
V3 fixes the isolated header selection; none altered the running application.

`normal-setup-texture-batch-v1-builds/console-setup` contains that change:
3DSX **7a9865e05576**, CIA **460c99857cb1**. The source comparison admits exactly
the three texture transaction files; renderer, shaders, game archive, codec,
movie reader and original/performance/movie recipes remain unchanged. The
changed texture recipe preserves earlier generations rather than relabelling
their journals. Shared CIA profiles remain Old80/New124. Verification passes;
the application has not yet executed this batching change.

Two more codec prototypes remain unaccepted: immutable scalar-distance lookup
(`install-etc-scalar-table-v1-test`) preserves the tested bytes, but needs
192 KiB of executable data and only shows 12.9% lower high-quality host CPU
in that short sample; no ARM/app measurement yet. Cached red/green joint lower
bound (`install-etc-pair-bound-v1-test`) also preserves bytes but adds 9,248
stack bytes for only 1.6% lower sampled host CPU; no application promotion.
Both tested 632 blocks over three alternating passes and high-quality
ASan/UBSan. Their timing overlapped the live converter and is not hardware
evidence. The production codec remains the previously verified channel bound.

The scalar-table actual-method proof also passes all 294,912 scalar cases,
80,000 RGB inequalities and 10,512 source/mode reinitializations under
ASan/UBSan. Missing invalidation and overestimating the bound are rejected.
Its application/ARM and executable-data cost still need evaluation.

A source-identity reuse audit (
`install-cmpr-reuse-audit`) covers 1,135 aligned original CMPR images:
2,244,696 blocks, 2,150,679 image-local unique blocks, 2,053,667 collection-wide
unique blocks. Only 97,012 image-local unique blocks repeat across images;
this does not support a large gain from a new global cache. It is not a
simulation of the existing decoded-RGBA cache, ISO order, reduced/padded
images or a bounded cache, and makes no timing claim.

At about 01:21 UTC the live fresh run's committed texture prefix contains
682 records / 756 native variants; all native bytes match the independent
3,754-variant reference. This remains partial conversion evidence, not a
completed installation or a physical one-hour result.

`verify_install_conversion_movie.py` and
`verify_install_conversion_complete.py` are ready for the completed ARM output;
the latter requires an exact-source performance reference. The game runner's
`--uninterrupted-movie` mode leaves controls untouched through a longer movie
window, records periodic decode coverage, then uses ordinary Start/Select.
Those new full-run modes have not yet run. A second bounded read-only FTP
attempt at `console-visual/20261013-004456-223` was refused before login; no
console files changed.

Previous regular ETC setup build: **`d96baaa51b07` / CIA `4b99a53d7a11`**,
menu `f7cf253520a7`, `normal-setup-channel-bound-v1-builds/console-setup`.
Previous experimental RGB8 setup build: **`635ecf89a502` / CIA `3de4fc7c0d75`**,
menu `5edbb9ff4e7f`, `normal-setup-rgb8-async-v1-builds/console-setup`.
Both are built, restored and verified locally; neither is deployed. New223
refused the bounded read-only timing collection at `console-visual/20261013-002043-223`.
No remote files or saves changed. The **fresh one-hour-bounded** New-mode ISO-only
test under `setup-channel-rgb8-fresh-new-v1` has now ended as recorded above;
do not infer physical time from it.

### ETC search rejects provably uncompetitive candidates

For each source sub-block, base-channel code and intensity table, the encoder
caches the minimum scalar squared error for red, green and blue separately.
Allowing separate selectors for each channel relaxes ETC's shared RGB-selector
constraint, so their sum is a lower bound on the actual candidate error.
If this bound is at least the incumbent error, the candidate cannot produce a
strict improvement and is skipped. Remaining candidate order, refinement,
tie handling, clamping and quality settings are unchanged. No lossy threshold
or reduced search radius was introduced. The cache is private to the optimizer,
invalidated for every new source/mode, and adds 3,084 temporary stack bytes.

Measured evidence:

- `install-etc-channel-bound-v1-test`: 632 mixed real/adversarial blocks,
  three alternating passes; high-quality host CPU **1.082221 -> 0.507626 s**.
  Every encoded byte matches; high-quality ASan/UBSan passes. Low/medium remain
  unused by setup and retain their previously documented unaligned-sort issue.
- `install-etc-channel-bound-corpus-v1-test`: **9,528** sampled blocks from
  **1,752** texture images and 12 movie frames exactly match the prior encoder;
  8,610 also match existing prepared pack bytes. Not every pixel of every asset.
- `install-etc-channel-bound-proof-v2-test`: actual bound method, all 294,912
  scalar palette/value cases, 80,000 RGB lower-bound inequalities, 10,512 cache
  initializations under ASan/UBSan. Missing-invalidation and overestimate
  mutants fail. V1 was a test-only compile error from using the three-argument
  minimum helper with two arguments; retained, no emulator run.
- `install-etc-channel-bound-arm-v1-{new,old}`: same 128x128 texture and exact
  expected output. New serial **19,795.54 -> 5,740.06 emulated ms**; existing
  worker **9,904.04 -> 2,968.76 ms**, versus the preserved ARM32 baseline.
  Old creates no worker and takes 5,740.06/5,739.98 ms. These are component
  measurements, not whole-install or hardware estimates. The Old run overlapped
  some host checks/build work; its host elapsed time is not a benchmark.
  Worker stack upper bound **23,960/131,072**, previously 20,784 bytes.
- `install-movie-channel-bound-v1-test`: four half-size and one full-size
  movie frame remain byte-exact, serial/parallel, with original error metrics,
  cancellation, CRC and SDK compilation checked. Its host timings were taken
  alongside a build and are not used for a speed claim.
- `install-etc-channel-bound-movie300-arm-v1-new`: real frame 300, exact
  existing ETC reference; resize/encode/error/LZ4 take **7,693.63 ms parallel**
  or **15,166.81 ms serial**. JPEG decode, SD reads/writes and assembly are
  excluded. Even after the substantial encoder improvement, this shows why
  ETC movie conversion remains a concern for the one-hour target.

`verify_install_channel_bound_build.py` checks the sole source change against
the preceding resize build, actual linked bound/DSP code, unchanged game
archive/renderer/shaders/quality policies and shared Old80/New124 profiles.
Texture/movie recipe source hashes change; geometry recipes do not. Older
production checkpoints are never relabelled.

### Read-ahead restores the lossless candidate's initial playback cadence

The synchronous RGB8 candidate below remains a failed playback configuration.
Testing the existing owned movie reader with `movie_prefetch.txt=1` restores
the initial movie interval on both emulated hardware modes. New explicit-mode
run completes in 49.80 host seconds; Old in 39.59. No renderer or codec changes
were made for those two tests. Captures were inspected, assets/saves unchanged,
Select exit and queue draining pass.

The new experimental build automatically enables that same reader for a
validated RGB8 movie header. Explicit diagnostic modes are preserved; ETC's
existing default is unchanged. This adds no new reader implementation or
thread: existing allocation/headroom limits, private read buffer, publication,
suspend, cancellation and joined retirement still own the work. Automatic
policy adds approximately 179,232 heap bytes in these runs, including the
existing worker/buffer. Pinned Melee ownership rules remain unchanged.

The worktree's reader files contain unrelated WIP. The checked builder instead
applies `patch_movie_async` to its archived stable owner/header/DVD source, then
restores the worktree. `install-movie-auto-reader-v1-test` tests those exact
patched sources: six format/mode combinations and **550 byte-exact queued
reads**, seeks, sleep/resume, failures and in-flight closes pass ASan/UBSan.
No global mode leakage; patch is idempotent. Build verifier matches these hashes.

`setup-rgb8-auto-reader-new-v1` uses the real experimental binary with **no
prefetch configuration file**. It completes in 50.06 host seconds with intact
saves/source/assets, expected native drawing and drained shutdown. Its initial
movie interval is **59.788 FPS / 16.726 ms**, versus regular ETC **59.722 FPS /
16.744 ms**. Movie phases diverge, so these are cadence checks rather than
equal-work or physical-console measurements. Raw RGB8 decode still costs about
9.008 ms; it has not disappeared. Reported audio underruns are 2/0 across the
two channels versus ETC 3/0, not proof of completely gapless or synchronized
audio. Whole-film/hardware playback acceptance remains open.

New automatic-reader peak heap is 53,917,912/71,303,168; minimum linear free
18,635,136/25,165,824; VRAM free 4,723,712; measured stack upper bound
72,936/524,288; process 103,124,992/130,023,424 bytes. Old explicit-reader heap
39,040,688/50,331,648; linear free 3,955,072/10,485,760; process
67,981,312/83,886,080. Source pixels/audio and the 203 MB lossless movie are
unchanged. Physical SD throughput remains unverified.

`install-movie-runtime-v2-comparison/result.json` retains both failed synchronous
and improved asynchronous results. The experimental build combines this policy,
the exact resize, ETC texture bound, previous geometry checkpoint batching and
source-SHA overlap. Its fresh conversion test starts from **only the ISO**,
without synthetic fixtures or seeded journals. The short gameplay checks used
explicit synthetic consumer fixtures and are not evidence of conversion time.

## 13 October: exact resize acceleration; RGB8 playback candidate rejected

The default converter still produces ETC1-LZ4 movies. The complete under-hour
console target remains unproven. `install_movie_resize.c` now accumulates RGB
in three 32-bit lanes after proving every partial sum fits signed 32-bit using
the absolute coefficient sum. Coefficients, tap order, rounding and intermediate
clamps are unchanged. Identical interior coefficients are calculated once per
axis; border kernels keep their independent normalization. No dimensions,
encoder quality, audio, frame count, progress/cancellation boundaries or
persistent allocations change. Coefficient scratch remains private to the call.

Evidence:

- `install-movie-resize32-v2-test`: all **1,797** frames exactly match the
  independently compiled archived 64-bit resize. Aggregate resized SHA-256:
  `eada4aa1b50f884f41e58d4118b7b609f18d8adfd7ce76a6d28dbc73dfec59e3`.
  Forty extreme/random images, all 37 cancellation boundaries, bounds and
  overlap checks pass ASan/UBSan.
- `install-movie-rgb8-resize32-arm-v2-new`: real production ARM resize, frames
  0/300/900, AB/BA order and exact output. **276.787 -> 131.698 emulated ms**,
  about **52.4% less component time**. This is not a physical or whole-install
  measurement. V1, before coefficient reuse, was 140.028 ms.
- `install-movie-resize32-frame-v2-test`: four half-size and one full-size ETC
  frame match existing prepared bytes, serial and actual worker/pthread adapter,
  cancellation/CRC/SDK link pass. It exercised V1 resize; V2 whole-film equality
  proves identical encoder inputs, with the encoder/worker unchanged. No repeat
  complete-film ETC encode is claimed. First invocation failed compilation on an
  existing unused vendor constant; retry suppresses that warning only for the
  vendor unit. Neither compile failure launched an emulator.

### Lossless movie experiment: quality passes, gameplay cost fails

New `install_movie_rgb8.c` converts the same resized RGB to PICA BGR/Morton
layout and independent LZ4 blocks. `FHRGB801` packets and `FHMOVS02` spool keep
source SHA, checksums, audio, ordering, duplicate reuse, resume and publication
checks. Only the explicit builder `--rgb8-movie` option selects it; the default
task remains ETC. `install-movie-rgb8-full-v2-build` converted the complete film
on the **PC in 52.80 seconds**, not console time. Its 202,844,608-byte movie SHA:
`1ea62624e7c76d4e72d5628f953bd10f5484e82d15549c3c75633d48655a0415`.

`install-movie-rgb8-v1-test` checks all 1,797 source JPEG/resized frames using
independent inverse Morton/BGR/padding, every audio byte, 47 malformed packets
and six cancellation points. `install-movie-rgb8-resume-v1-test` verifies actual
stop/resume with an uncommitted tail and exact committed prefixes. No real
application journals are seeded or relabelled.

Native ownership supports three RGB8 slots, bounded **576 KiB** total, retains
15% linear headroom and retires GPU readers before reuse/free/format changes.
No additional draw, shader, queue or worker. Pinned Melee texture lifetime/cache/
VRAM owners were consulted. Actual owner/decoder ASan/UBSan tests cover 3,000
ETC and 1,200 RGB8 cycles, no per-frame allocation, hidden/re-enabled frames,
three-slot decode, corrupted CRC and rejected missing-retirement mutant.
Initial integration caught the old 128 MiB file limit: storage now matches the
reader's 300 MiB cap, still validated through 16 KiB scratch, not a huge RAM
allocation. Initial failed build/fixture records are preserved.

Experimental build `d4e41ca96f2f` / CIA `87ad25c46a5d`, menu `66673142b318`,
in `normal-setup-rgb8-v2-builds` is **not deployed or accepted as default**.
New/Old application runs complete title/intro/menu and Select exit; captures
inspected, saves/assets unchanged and queues drained. But playback regresses:

| Emulated movie interval | Default ETC | Experimental RGB8 |
| --- | ---: | ---: |
| New rendered cadence | 59.72 FPS | 28.10 FPS |
| New mean interval | 16.74 ms | 35.59 ms |
| Reported mean video decode | 1.527 ms | 9.107 ms |

Old RGB8 averages 28.37 FPS in this window. Same HID sequence and renderer
sources; movie phases diverge, so these are regression observations, not an
equal-work speedup comparison or console prediction. Actual bounded decoder
plus CRC costs 9.3–9.6 emulated ms before SD/GPU work. The 203 MB movie requires
up to 4.268 MB/s in its busiest second. No frame, simulation or CRC is skipped
to pass. Physical SD acceptance is unverified. Existing movie read-ahead is
disabled unless selected by `movie_prefetch.txt`; this test did not change it.

`install-movie-runtime-v1-comparison/result.json` records log hashes/captures.
Default New run `setup-resize32-playback-new-v1` passes in 47.21 host seconds;
RGB8 New/Old take 81.50/72.24. Default New heap peak 53,738,680/71,303,168;
linear minimum free 19,126,656/25,165,824; VRAM free 4,723,712; measured stack
upper bound 72,936/524,288; process 103,124,992/130,023,424 bytes. RGB8 costs
491,520 more linear bytes. Old RGB8 heap 38,861,456/50,331,648; linear free
3,955,072/10,485,760; process 67,981,312/83,886,080. These remain within budgets.

The default build retains ETC and adds the exact resize optimization atop prior
checkpoint batching/source-SHA overlap. `verify_install_resize32_build.py`
checks snapshot, game archive, scene shaders, Old80/New124 profiles, ETC default
and output proof. Original/performance recipes are unchanged; texture policy
is unchanged but its shared enum header source hash changes. Movie recipe now
explicitly names the same ETC format and includes changed resize source.
No older production checkpoint is relabelled. Full conversion time still needs
substantial high-quality ETC savings or faster lossless playback before adopting
that format. No FTP, installed files or saves changed.

## 13 October: overlap source verification and assess the movie bottleneck

Latest local shared setup build: **`b164611b5ea2` / CIA `cb2e5ad6aabf`**,
`normal-setup-fingerprint-v1-builds/console-setup`; menu/source ID
`fbf5e53bbb97`. Built, restored, verified and short-run checked; not deployed.
This includes the geometry payload batching below. Full conversion under one
hour is still unverified, and the high-quality texture/movie encoding workload
remains a major concern.

The actual source fingerprint owner now overlaps the next 64 KiB ISO read with
the previous block's SHA-256 update on the existing New startup worker. The
producer alone owns the FILE cursor. Hash updates remain ordered; publication
and join precede SHA-context access, buffer reuse, finalization and cancellation.
This adapts the pinned Melee render-worker borrowed-resource/fence discipline;
the existing startup worker's affinity and release/acquire code are unchanged.
Old, unavailable-worker and allocation-failure cases retain the serial hash.
No source bytes or validation checks are skipped. This adds 64 KiB of temporary
normal-memory scratch on New and 20 bytes to the ARM fingerprint owner; it adds
no worker, linear allocation, VRAM allocation or gameplay memory reservation.
The buffers retire before geometry conversion and the worker retires before audio.

The C compilation route explicitly defines `__3DS__` for this worker consumer;
otherwise its portable no-worker inline adapter would silently remain active in
the console build. Linked-code verification checks the actual worker calls.
All conversion recipe keys, quality policies, renderer sources/shaders, game
archive and Old80/New124 CIA profiles match `b03333240984` exactly.

Evidence:

- `install-fingerprint-pipeline-v1-test`: 24 exact digests through actual
  fingerprint/worker code and pthread SDK adapter, ASan/UBSan; cancellation,
  foreign cursor, truncation, allocation failure and rejected submission pass.
  Removing the cancellation join is rejected by the ownership assertion.
- `install-fingerprint-pipeline-full-v1-test`: the complete 1,459,978,240-byte
  private ISO still produces SHA-256 `f5e5a9a29147f80648dbef00ed68cd84d0f08c7120645e6f653333296944facf`.
  This full-file test uses the serial host adapter, not ARM full-file timing.
- `install-fingerprint-pipeline-arm-v1-new`: actual production fingerprint
  compiled with the game Clang flags plus SDK worker/crypto. An 8 MiB private
  source sample takes 3,067.691/3,067.629 emulated ms serial versus
  1,799.200/1,803.693 ms pipelined in AB/BA order: approximately 41% less elapsed
  time for this component. Both digests and cursor/cancellation checks pass;
  128 worker jobs, stack-probe upper bound 4,096/131,072 bytes. This is not a
  whole-ISO or physical-console timing estimate.
- `setup-fingerprint-cancel-new`: exact shared binary, fresh ISO-only isolated
  profile, 24.45 host seconds. Real HID cancellation during verification returns
  to the menu and exits normally. Captures before/after cancellation inspected;
  1,176 hash jobs retired, allocator arena peak 475,136 bytes. Source and save
  unchanged, no prepared artifacts yet, owned emulator exited. No full-stage
  completion claim.
- `verify_install_fingerprint_build.py` gates source hashes, recipe equality,
  linked worker calls, renderer/game/shaders, actual binary hashes and profiles.

The earlier isolated hash investigation also compared 256 KiB serial reads:
2,961.393 ms versus 3,067.484 ms with 64 KiB. Increasing the serial read buffer
was much less useful than overlap and was not adopted.

### Lossless movie prototype, not yet a runtime format

`run_install_movie_rgb8_arm.py` now tests original JPEG decode, unchanged
Lanczos resize, native RGB8 tiling and LZ4 on actual ARM. Frames 0/300/900 take
337.666/362.961/364.474 emulated ms respectively. Resize dominates at 276.788
ms; tiling takes 7.358 ms and compression 2.677–7.142 ms. Exact native RGB8
SHA-256 and decompression round trips pass. The SDK LZ4 decode alone takes
1.252–1.488 ms; this excludes the production bounded decoder, CRC, SD, GPU
publication and rendering, so it is not a playback cost claim. Evidence:
`artifacts/native/install-movie-rgb8-arm-v2-new`. V1 was a compile-only failure
from a missing stdio include; retained, no emulator was started for that failure.

This would remove about 5.6 million high-quality ETC block encodes without
lowering the existing resized image's precision. It is **not promoted**: the
prior complete-film audit finds 202,844,608 bytes versus 37,014,432, average SD
traffic 3.383 MB/s and worst one-second traffic 4.268 MB/s. Three RGB8 texture
slots need 589,824 bytes, 491,520 more than the current half-resolution slots;
the existing 384 KiB owner cap must not simply be bypassed. Runtime format,
buffer lifetime, headroom, actual decoder cost and physical SD acceptance remain
necessary. The shipped installer still produces the verified ETC movie.

`install-movie-lossless-temporal-v1-audit` checks all 1,797 frames and unchanged
audio using exact XOR/subtract deltas with a keyframe every 30 frames. XOR is
slightly larger; subtraction only reduces the pack to 195,048,512 bytes and
worst one-second traffic to 4,250,624 bytes. The small saving does not justify
adding 196,608 bytes of history and seek dependencies; neither is adopted.
This audit took 13.88 host seconds, not a console conversion measurement.

Next work should address movie preparation and playback together, then the
remaining high-quality texture encoding cost. Do not extrapolate the short hash
or movie samples into a claimed complete sub-hour installation. No FTP or remote
files changed during this follow-on.

## 13 October follow-on: batch complete geometry records

Latest local shared build: **`b03333240984` / CIA `d4d33e99ef90`**, in
`normal-setup-payload-v1-builds/console-setup`. Built, restored and verified;
not deployed. The full under-one-hour console target remains unproven.

Original and performance geometry now checkpoint complete records in batches
of 64 scan records or approximately 1 MiB, whichever is reached first. A bank
change flushes the previous cursor **before** any new bank's payload is written.
Orderly cancellation and completion force a checkpoint. The existing dual
journal, payload checksums, complete-artifact validation and final publication
rules remain unchanged. Texture and movie encoding still checkpoint each newly
converted payload immediately.

An unexpected interruption may require repeating up to 63 successfully returned
records plus the current record being converted/committed; the uncommitted byte
range is less than 1 MiB plus one record. This trades a bounded amount of repeated
geometry preparation for fewer filesystem commits, without changing generated
geometry, reduction thresholds or image quality. Normal cancellation retains
all completed records. Each geometry spool adds only four bytes of configuration
versus the preceding duplicate-only build; no new buffer or worker is introduced.

The pinned Melee `native-3ds/port/3ds/file_io.c` and `card_storage.c` were reviewed
for filesystem operation cost, single-owner writes and flush-before-publication.
This installer-specific batching differs from Melee's save replacement; it uses
the existing validated resumable transaction instead of changing SDK I/O,
threading, cache coherence or game-save handling.

Evidence:

- `install-payload-cursor-v1-test`: actual transaction sync calls fall from
  400 to 8 over 200 new 16 KiB payloads. Byte/count limits, bank transition,
  cancellation, interrupted-tail recovery and injected flush failure pass
  under ASan/UBSan, with ARM compilation.
- `install-geometry-payload-v1-test`: all 2,110 unique original records and
  2,102 rendering packets match the preceding pack exactly. The deliberate
  interruption causes 20 source records to be repeated (4,633 visits instead
  of 4,613), demonstrating recovery; the final pack hash remains
  `3b93a639bf8983af22c5993c101e9168d1c436fe012e827ab2534274f9d3c5ec`.
- `install-performance-payload-v1-test`: eight real sources, five admitted
  patches, 200 duplicate updates and three resume cases pass; both final packs
  retain their previous bytes.
- `install-payload-stage-cancel-v1-test`: the actual original/performance ISO
  stage wrappers, compiled from this build's archived dependency closure, retain
  a pending completed record on cancellation. Injecting its flush failure
  returns failure and recovers the prior checkpoint. Four sanitized cases pass.
- `install-payload-cursor-arm-v1-new`: actual ARM code, same 2 MiB of prepared
  geometry bytes, AB then BA order in an isolated silent Azahar run. Eager
  commits take **14,975.473 ms**, batched commits **835.699 ms**, with **256 -> 4**
  flush calls; both orders agree. Exact output and recovery pass, owned process
  exits. This isolates transaction/read/write work, not conversion, actual SD
  latency, or console speed. No full-installation multiplier is justified.
- `verify_install_payload_build.py`: exactly five installer source changes;
  renderer, shaders, game archive and quality policies unchanged. Shared CIA
  remains Old80/New124. Only original/performance recipe keys change.

A weighted repeated-colour ETC prototype remains **unpromoted**. It matches
9,528 sampled texture/movie blocks and passes high-quality sanitizers. Although
sampled host CPU falls 6.89%, the actual New ARM texture check regresses from
19,795.539/9,904.043 to 20,090.443/10,195.983 emulated ms (serial/parallel).
The production encoder remains unchanged. Evidence:
`install-etc-weighted-{prototype,v1-test,corpus-v1-test}` and
`artifacts/native/install-etc-weighted-arm-v1-new`.

One read-only request for saved console timing was refused before FTP login at
New223; `console-visual/20261012-230905-223` records it. No remote changes were made.

## 13 October: startup-only parallel encoding

### Subsequent exact optimizations (local; no hardware timing claim)

The ARM encoder now uses ARM11 dual-halfword DSP operations for exact RGB
squared distances, and 32-bit internal error sums. A complete 16-pixel RGB8
block cannot exceed 3,121,200 squared error; invalid-result sentinels remain
separate. The candidate does not change the high-quality search, filtering,
dimensions, thresholds, or output policy. `install-etc-arm32-corpus-v3-test`
checks 9,528 blocks sampled from 1,752 prepared texture entries and 12 movie
frames against the prior codec; all output bytes/errors match, with high-quality
ASan/UBSan checks. This samples every eligible image, not every pixel.
Both actual ARM runs check 523,265 distance cases and the real 128x128 image:
New serial/parallel 19,795.54/9,904.04 emulated ms versus the earlier
22,224.13/11,181.11 ms. Old remains serial. New worker measured stack upper
bound is 20,784/131,072 bytes. Emulator service limitations prevent interpreting
its current-core-number return as affinity evidence. No physical timing claim.

Duplicate texture cursor updates are now batched in groups of at most 64;
new encoded planes still receive an immediate durable commit. Changing banks,
finishing, and orderly cancellation force pending cursor publication. Unexpected
interruption can repeat up to 63 duplicate planes, but cannot discard committed
new image bytes. Exact source comparisons remain in place. The complete TEX
audit found 20,208 planes: 2,454 unique and 17,754 duplicates. The modeled scan
checkpoint count falls from 20,208 to 2,735 (40,416 to 5,470 filesystem syncs).
Fonts/assembly/other transaction overhead are excluded. Counts do not measure SD
latency. `install-duplicate-checkpoint-v2-test` checks actual filesystem syncs,
recovery, error injection and identical final packs. Actual installer cancellation
through 70 duplicate records, colour/opacity/font transitions, and padded output
pass in `install-texture-duplicate-prepare-v2-test` and
`install-texture-duplicate-padded-v2-test`.

Geometry preparation no longer clears unused vertex/index arrays for every
group, or all 2 MiB of GPU scratch for every asset. Counted fields and published
alignment gaps are initialized; actual vertex/index entries are written before
consumption. `install-scene-clear-v3-test` compares every generated packet across
4,613 source records (18,616 draws, 17,207 groups), with all bytes identical and
ASan/UBSan/ARM compilation passing. Paired host compiler CPU is
1.397365 -> 0.717686 seconds. This excludes scanning, hashing, SD checkpointing,
packing and other stages; it is not a whole-conversion speedup estimate.

Built milestones: ARM32 `53abc74065ed` / CIA `f92aab3541f2`, duplicate
batching `1d74450b7fb8` / CIA `bbef415ac0ee`, then geometry scratch clearing
`c540f879c2f4` / CIA `9a4528d93a1d`. Their verification scripts prove
the precise installer source changes, unchanged renderer/game archive/shaders,
unchanged quality policies and shared Old80/New124 CIA profiles. None is deployed.
The fresh `setup-conversion-arm32-fresh-new` run has a one-hour bound and contains
only the ISO, not seeded or relabeled checkpoints. It runs the earlier ARM32
binary; later source edits do not affect that run. Source verification took about
561 host seconds before the original stage appeared. Do not infer console speed
from that number or call an incomplete run an under-hour success.

The subsequent shared build `f1f8f8f48a47` / CIA `94a5756db17a` batches final
pack copying into approximately 1 MiB checkpoints, forcing a checkpoint at the
last record and on orderly cancellation. This applies only to assembly from
already durable converted spools; the producer's per-new-image/model/frame
commits and complete output validation are unchanged. An unexpected interruption
may repeat less than 1 MiB plus one record of copying, never the conversion.
No extra write buffer or memory owner is added. Exact compared pack sizes imply:

| Assembly | Previous copy checkpoints | Batched copy checkpoints |
|---|---:|---:|
| Original geometry | 2,102 | 47 |
| Textures | 3,754 | 79 |
| Movie | 1,797 | 35 |

These counts exclude header/finalization/cancellation calls and performance
packs. `install-assembly-checkpoint-v1-test` verifies actual filesystem syncs
(600 -> 10 over 300 copied 16 KiB records), cancellation, interrupted-tail
recovery and injected flush failure under sanitizers. All four actual packers
pass their resume/validation tests with byte-identical output. The movie fixture
explicitly seeds 1,795 prepared frames; its final two frames reuse duplicates,
so its 1,797-frame equality proves assembly and unchanged audio, not fresh movie
encoding. `verify_install_assembly_build.py` gates the precise five source changes,
unchanged producer checkpoints and policies, unchanged renderer/shaders/archive,
and shared Old80/New124 profiles. Built and verified, not deployed.

The fresh ARM32 run reached performance geometry at 1,543.646 host seconds.
Its completed original stage contains all 2,110 records: 2,102 rendering packets
are byte-identical to the reference and eight records are nonrendering. This
is not a full-installation result and does not include the subsequent SD or
geometry scratch changes.

Exploratory codec candidates that reorder error evaluation, cache visited
endpoints, or project unclamped palette errors are not promoted. In particular,
chromatic pixel ordering was byte-exact but made the sampled high-quality host
test 2.8% slower. The lossless RGB8 movie audit retains all 1,797 frames and exact
resized RGB, but grows the LZ4 movie from 37,014,432 to 202,844,608 bytes and raises
worst one-second SD traffic to 4,267,712 bytes. It is not a runtime codec change
or accepted alternative; physical throughput and memory costs would need proof.
An exact byte-difference predictor plus DEFLATE level 6 reduces that alternative
to 143,620,416 bytes, still 3.88 times the ETC movie, with worst one-second reads
of 3,384,960 bytes. All frame pixels and audio were verified, but native runtime
decompression, extra texture storage and physical SD throughput remain untested.

The original under-one-hour console target remains unmet/unmeasured.

The bounded fresh ARM32 test subsequently ended at 3,578.668 host seconds after
normal scripted cancellation and menu/LCD retirement. Its runner reports failure
because the requested complete bundle was not reached; it did not crash. Both
geometry stages are complete and byte-exact. Textures stop at 325 committed
records, containing 329 of 3,754 reference variants, all compared exactly.
Movies and publication were not visited. Inclusive emulated phase times are
551.356 seconds source verification, 981.661 original geometry, 1,389.542
performance geometry, and 642.892 partial textures. Host tests/builds overlapped
this run. These are not hardware timings or a whole-conversion prediction.
The allocator arena peak was 10,813,440 bytes; the temporary worker retired with
24 jobs and a measured stack upper bound of 20,784/131,072 bytes. Source and
save sentinels are unchanged; all owned processes exited. The newer local build
has not yet received a full conversion run, and nothing was deployed.

### Geometry duplicate cursors and completed ARM geometry proof

The following shared build is `080ea8606b99` / CIA `0cd75b6240aa`, in
`normal-setup-cursor-v1-builds/console-setup`. It additionally batches duplicate
scan cursors for both original and performance geometry, at most 64 records.
Bank changes, orderly cancellation and completion flush pending cursors. New
model payloads still receive immediate durable checkpoints. This adds 16 bytes
to each startup spool owner; no image buffer, renderer or worker changes.
Interrupted scans can repeat up to 63 duplicate records without reconverting
any committed model. SHA/CRC identity checks remain unchanged.

`install-cursor-v1-test` measures 400 -> 8 actual sync calls for 200 duplicate
cursor updates, with bank changes, interruption/cancellation, injected sync
failure and exact payload checks under sanitizers. The complete original ISO
test processes 4,613 records and produces the same 2,102 rendering packets and
pack SHA `3b93a639bf89...`; the performance fixture's two packs also match.
`verify_install_cursor_build.py` checks the exact seven changed sources,
unchanged producer commits/quality/renderer/shaders/archive, and Old80/New124.
Only the original/performance recipe keys change versus the assembly build.
This build is verified locally, not deployed or physically timed.

The original performance reference was correctly rejected because its converter
dependency hashes were stale. `prepare_native_iso_performance.py --source-build`
rebuilt the host reference from the running ARM build's archived dependency
closure, preserving the older evidence and the strict source check. The fresh
ARM run's completed performance stage now matches all four artifacts exactly:
2,110 records, 467 patched sources, 1,383,658 -> 1,376,568 recorded triangles,
topology, geometry and lossless cache. This establishes those conversion outputs,
not complete installation or physical timing.

A bounded 14 KiB triangle-invariant cache was also tested but **not adopted**.
All 10,590 actual full-ISO fidelity decisions and reported errors matched, and
206 independent shape/UV/colour/seam/allocator cases passed ASan/UBSan and ARM
compilation. Host fidelity CPU was 2.322435 -> 2.323294 seconds, showing no
benefit in this workload; adding the stack cost is not justified by that result.
The prototype and failed-to-improve timing remain in
`install-fidelity-cache-{prototype,v1-test,oracle-v1-test}`.

Local shared build `cb71f019653a` / CIA `a5b2531acd17` adds a New 3DS core-2
worker for movie and large texture encoding. It is not deployed. The worker
exists only inside the measured preparation call and is joined before returning
to the menu or starting gameplay/audio. Old 3DS uses the unchanged serial codec.
The explicit core, release/acquire job publication, LightEvents and join-before-free
follow pinned Melee `render_worker.c` and `gx_thread.c`; the worker also inherits
the producer's FPSCR. No worker SD, UI, GPU or gameplay work is introduced.

Textures split into contiguous row halves above 128 native tiles, each with its
own existing 19,456-byte exact block cache. Movies split alternating visible tile
rows; padding runs on the producer after joining both visible halves. Inputs and
outputs are shared immutable/disjoint ranges. Progress and cancellation remain
on the producer, cancellation joins before releasing pointers, and only complete
conversions may enter the existing checkpoint transaction. No extra image buffer
is allocated. The worker reserves 128 KiB of normal RAM for its stack; the second
cache lives on that stack. No linear RAM, VRAM, quality/size/filter change or
gameplay worker allocation is added. The normal asset and movie error thresholds
remain unchanged.

Evidence:

- `install-worker-v1-test`: actual owner under a pthread SDK adapter, 64 lifetimes,
  576 borrowed jobs, active-job shutdown, creation failure/Old mode, missing-join
  mutant rejection, sanitizers and strict SDK compilation.
- `install-texture-parallel-v1-test`: 19 real full/reduced, alpha/opaque texture
  cases through 128 pixels, 114 cancellations, exact bytes/error totals versus
  archived serial code and prepared assets, source immutability and output guards.
  Sanitized host wall time 8.076320 → 4.767749 seconds; process CPU
  8.076254 → 8.087349 seconds. No 256+ pixel case was exercised by that fixture.
- `install-movie-parallel-v2-test`: four original half frames and one full frame,
  exact prepared ETC bytes, Lanczos pixels, LZ4/CRC and error totals, eight
  cancellation points and producer-only callbacks. Sanitized host wall time
  12.348077 → 6.891100 seconds. Final padding block counts remain 3,136/12,416.
- `install-profile-owner-v3-test`: actual startup wrapper starts/stops the real
  worker and writes the phase log only after retirement, including cancellation
  and diagnostic-log failure. Host sanitizers pass.
- `install-worker-arm-v3-new`: actual ARM worker and both codecs run silently in
  isolated Azahar. One real 128×128 texture takes 22,224.129/11,181.106 emulated
  milliseconds serial/parallel with exact bytes/error totals, cancellation joins,
  a constant movie frame matches serial output and the worker retires. Measured
  stack upper bound is 20,896 bytes of 131,072. This is a small execution check,
  not full conversion or hardware throughput. Azahar lacks SVC 0x11
  GetCurrentProcessorNumber: its logged core=0 is not a valid affinity reading.
  The explicit core-2 request is retained; physical core/FP/cache validation is open.

`verify_install_parallel_build.py` checks the exact tested source hashes, linked
worker calls in both ARM codecs, unchanged renderer/shaders/game archive,
unchanged geometry recipes and quality policies, and shared Old80/New124 CIA.
Native C++ codecs now explicitly define `__3DS__`; the SDK worker is compiled by
the SDK compiler. This prevents accidentally linking the host-only serial adapter.
The first build failed before linking because the worker had initially been routed
through the game compiler without SDK headers. Its restoration and failure logs
are retained; v2 repairs the build routing.

The earlier movie test v1 had a bad cancellation assertion beyond the compressor's
maximum progress and was corrected in v2. ARM check v1 rejected incompatible
SDK integer typedefs at compile time. V2 exceeded its bound after requesting a
processor-number assertion unsupported by this emulator; it is not a passing
result. V3 records that limitation, logs completed stages, and uses early active-job
cancellation; it passes without changing the production worker or codecs.

The old deployed converter's genuine ISO-only emulator run
`setup-conversion-resume-bundle-v1-new` reached its 10,800-second bound without
finishing textures. It cancelled cleanly through HID, retired its menu, preserved
saves/source/checkpoints and exited its owned processes. Geometry stages are
complete; textures stop at committed cursor `[42,879226]`, 4,873,600 bytes.
There is still no complete fresh conversion or one-hour acceptance measurement.

A larger movie-block cache was investigated over three independent 120-frame
windows. A 32,768-entry cache hit only about 3–4% of nonuniform blocks; most hits
were already cheap uniform colours, so no large resident cache was added.
An exact repeated-colour trial prototype passes block comparisons and high-quality
sanitizers, with about 8.37% less host CPU than incumbent pruning in a short paired
sample. It remains unpromoted in `install-etc-duplicates-prototype` and
`install-etc-duplicates-current-v1-test`; concurrent build activity limits timing
interpretation. Neither experiment demonstrates the whole-install target.

The high-quality ETC1 encoder now stops evaluating a colour candidate once its
accumulated squared error cannot beat the incumbent solution. The original
encoder only bounded this work by other intensity tables for the same candidate.
Both callers consume a trial only when it strictly improves the incumbent;
equal-error trials do not affect the chosen selectors or refinement sequence.
All error contributions are nonnegative. Search order, quality, interpolation,
rounding, alpha coverage and output format are unchanged. This is exact search
pruning, not a lower quality encoder or a renderer change.

The earlier image-local exact block cache is retained. Together these changes
reduce measured complete-texture conversion CPU time on the host:

| Sample | Deployed encoder | Candidate | Scope |
| --- | ---: | ---: | --- |
| 19 complete game textures | 32.786317 s | 12.615512 s | Unsanitized process CPU; full/reduced and opaque/alpha images through 128 pixels |
| 632 individual blocks, three passes | 9.718092 s | 4.141222 s | High-quality encoder only; 504 game blocks and 128 patterned/random stress blocks |

The complete-texture sample is about 2.60 times faster (61.52% less CPU time).
Every output byte matches the archived deployed converter and existing private
texture pack. Dimensions, visible-error totals, coverage, tile progress and
57 cancellation cases match. The cache collision mutant is still rejected.
The block benchmark alternates baseline/candidate ordering; all three quality
modes retain identical bytes. Only high quality is used by the installer.
An independent padded-image sanitizer test covers 14 cases, 8,832 edge pixels,
and 42 cancellations with the changed codec.

These are bounded host samples, not a complete asset collection, ARM instruction
measurement, SD benchmark or prediction of console installation time. A separate
unchanged ARM conversion was running concurrently. Process CPU time excludes
descheduling but does not eliminate effects of host contention. Do not infer
that the entire installation is 2.6 times faster: ISO fingerprinting, geometry,
movie preparation, checkpointing and final verification also take time.

## Reproduction and evidence

- `tools/benchmark_install_etc_bound.py`: paired block correctness/timing,
  high-quality ASan/UBSan, and ARM compile/static stack usage. Evidence:
  `artifacts/research/install-etc-bound-v3-test`.
- `tools/test_install_texture_cache.py --label install-texture-bound-v1-test
  --encoder-bound --host-benchmark`: complete-image comparison with separate
  archived/current codec namespaces. Evidence uses that label.
- `tools/test_install_texture_padded.py --label install-texture-bound-padded-test`:
  actual converter padding/alpha/error/cancellation under ASan/UBSan.
- `tools/test_install_profile.py` and `tools/test_install_profile_owner.py`:
  inclusive phase sums, reset/cancel, actual callback forwarding, preserved
  status/error, logging failure and writes only after converter retirement.

Failure evidence is retained. The first block harness accidentally duplicated
the green channel in its extracted samples; its broad sanitizer run was stopped
and its numbers are not acceptance evidence. V2 corrected the sample extraction
and found existing unaligned loads in the archived low/medium-quality radix
sorter. Those modes are not used by setup. V3 retains their unsanitized byte
checks, and gates ASan/UBSan specifically on the actual high-quality path. This
does not claim the unused low/medium paths are sanitizer-clean.

## Console timing report

The startup owner now appends a small `SD:/3ds/foxhollow/install-timing.csv`
report after each preparation invocation completes, fails or is cancelled.
It records build ID, Old/New mode, result and inclusive wall milliseconds for
source verification, original geometry, performance geometry, textures, movie,
publication and other work. Unvisited phases have an explicit seen flag of zero.
Repeated invocations reset totals; completed-stage verification on resume is
included in that invocation's measurements. Do not confuse resume time with a
fresh installation.

The clock is the existing startup SDK system tick. Phase spans include converter
CPU work, SD/service waits, scheduling and progress UI; they are not CPU usage
or isolated SD time. One small append occurs after converters release their
files. No per-image/tile diagnostic writes, new worker or gameplay allocation.
An abrupt power loss can prevent this final report from being saved; existing
transaction checkpoints remain independent. Timing write failure cannot change
conversion success/cancellation or authorize an incomplete installation.

The pinned Melee bottom-screen implementation was consulted; existing framebuffer
publication, redraw throttling and SDK lifetime remain unchanged. The codec still
uses the pinned Melee CMPR palette semantics. The new pruning adds no allocations;
the previously introduced block cache retains its 19,456-byte temporary storage.
The ARM high-quality evaluation frame is 136 bytes; the converter's 19,920-byte
frame and the 512 KiB stack reservation are not a whole-chain peak measurement.

Local build and runtime acceptance are recorded separately. Texture/movie recipe
keys change because the codec source changes, even though tested outputs match.
Never relabel old journals or replace the executable of a live conversion. The
genuine full conversion of deployed build `87ca5200d925` continues unchanged.

## Movie padding and whole-work inventory

The movie encoder now reuses the first encoded padding strip below each visible
frame. Every pixel in the padded region repeats the final visible RGB row;
all later four-row strips therefore have identical ETC blocks. Native tile
offsets select the previously encoded bytes. Visible pixels/error totals and
tile-granular progress/cancellation are unchanged; no extra allocation or worker.

Half-size frames require 3,136 encoder calls instead of 4,096 (23.44% fewer).
Full-size frames require 12,416 instead of 16,384 (24.22% fewer). These are
verified call counts, not equivalent time reductions: different blocks cost
different amounts to encode. `test_install_movie_frame.py --padding-reuse
--label install-movie-padding-v1-test` checks four original half-size frames
and one full-size frame against existing tex3ds assets, exact Lanczos pixels,
LZ4/CRC, six cancellations, ASan/UBSan and SDK compile/link. All pass.

`audit_install_encoding_work.py` inventories the real source and independent
texture pack without re-encoding them. Accepted full-size CMPR images account
for 2,509,932 blocks, reduced variants 408,576. Only 30 images/3,716 blocks use
exact pixels after an ETC quality rejection, so an early rejection optimization
would target a small fraction of this collection. The movie has 1,797 frames
and 1,787 unique source images. Existing whole-frame deduplication is retained;
the new padding reuse reduces those images' encoder calls from 7,319,552 to
5,604,032. Every frame and its independent audio remain in the output.

A separate unclamped-colour selector prototype retained exact bytes and passed
high-quality sanitizers, but its paired host timing showed no improvement over
incumbent pruning alone. It is retained under `install-etc-selector-prototype`
and `install-etc-selector-v1-test`; it is not promoted into the converter.

Local normal build `daf887cc1a92` / CIA `4aab1c78a5cb` contains incumbent pruning
and phase reporting, with unchanged renderer/shaders/game archive and Old80/New124
profiles. It is built/verified, not deployed or ARM-runtime validated. The later
movie-padding build is tracked separately; no long-running converter was replaced.

Final local v2 adds the tested movie-padding reuse: 3DSX `80012791a56e`, CIA
`37a82894ae6d`, in `normal-setup-etc-bound-v2-builds/console-setup`. The verifier
finds exactly four setup-only source changes versus the previous local index-cache
build. Renderer, shader and game archive bytes remain unchanged; Old80/New124
profiles are verified. Geometry recipe keys match that previous local build;
texture/movie keys change. V2 is built and verified, not deployed or runtime-tested.

`benchmark_install_movie.py` additionally compares complete half-size frame
preparation (resize, encode, visible error and LZ4) against the deployed sources.
Frames 300 and 900 together fall from 48.327349 to 13.546799 host CPU seconds,
about 3.57 times faster. Each complete output and error total matches exactly.
JPEG decode and codec initialization are outside this timed interval. Two frames
are not a whole-movie timing or a console estimate. Evidence is in
`install-movie-timing-v1-test`; its sources match v2.

`collect_visual_console.py --install-timing` retrieves only the startup receipt,
32 KiB-bounded timing log and build identity when ftpd is available. Normal visual
collection also includes those small startup logs. This turn made no FTP requests
or remote modifications and did not interrupt the user's conversion.

Final packaged-app checks use the exact shared build: New source verification,
progress, HID cancellation and post-retirement timing log pass in 10.75 host
seconds; Old CIA-content startup, disabled-play/missing-ISO handling, presets,
persistence and exit pass in 16.63 seconds. Captures were inspected. Both retain
private save sentinels; no complete conversion or gameplay was claimed. The
standalone Old ARM codec check also passes (48.16 host seconds), creates no
worker, and reports 22,224.129/22,224.045 emulated ms for its two serial texture
calls. All owned tests/builds/emulators are terminal. No FTP or deployment.
