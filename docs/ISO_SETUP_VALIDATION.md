## Current conversion check: 13 October, build c341525a9535

Update03:00 UTC: fresh handle92862 is TERMINAL. Clean cutoff at3576.020504s,
not a completed installation. Both geometry stages remain byte-exact; final
texture prefix1286records/1476variants matches the independent reference.
LIVE83205 `setup-lattice-rgb8-resume-complete-new-v1` uses the SAME binary and
copies of these genuine checkpoints, three-hour bound. Do not start another
emulator. The complete verifier now has explicit `--allow-resumed`, reporting
attempt totals and known repeated verification separately. No fresh full-run
or physical-time claim. Host tests/build overlap early resume (~02:49–02:54).

New progress UI build8f7007fefb51 is local only; host estimate/runner/task gates
pass and shared binaries compile. Emulator UI validation waits for this run.
The older live wording below describes the original observation, not a live handle.

The current run is `setup-lattice-rgb8-fresh-new-v1`, owned session 92862,
started at 01:47:34 UTC with a 3,600-second bound. It is a fresh ISO-only
conversion, not a resumed or seeded consumer fixture. Older live-process
references below are historical; do not restart or poll them as current work.

Original geometry completed before the performance stage began at 532.588602
host seconds. Its full verification passes: all 2,110 records, 2,102 rendering
packets, native scene, compressed cache and empty original topology are exact.
The native scene hash is `3b93a639bf8983af22c5993c101e9168d1c436fe012e827ab2534274f9d3c5ec`;
the cache hash is `f2eaab13a098db3e407ad09af90444cdc62b96fba59b31842078673aeda56373`.
Performance conversion also completed and passes full byte comparison against
the exact-source reference below: 2,110 records, 2,102 rendering packets and
467 patched sources, including spool, topology, scene and compressed cache.
Textures began at 1,088.848567 host seconds. The first committed 391 texture
records / 400 of 3,754 variants pass exact comparison; that stage is still active.
No full-installation, one-hour or physical-timing acceptance follows from these
completed stages or the partial texture observation.

Use the Linux toolchain for full geometry/cache comparison: the independent
cache encoder requires its liblz4. A Windows invocation passed the packet
checks but could not load liblz4; the subsequent Linux full check passed.
Both observation records are retained; this was a verifier-environment issue,
not an ARM conversion failure.

The existing exact-source performance reference is
`artifacts/research/install-performance-channel-full-reference/result.json`.
The current encoder changes leave original/performance recipes unchanged;
the verifier still checks source dependencies before admitting this reference.

## Earlier conversion evidence (historical process handles)

13 October: original and performance ARM conversion stages are now complete.
Both final geometry/cache sets compare byte-for-byte against checked host outputs.
Performance serial7197 has2110records/2102rendering/467patches; scene2407db330636,
cacheec69ca783140. This establishes asset output correctness, not physical speed.
Full bundle is still running under owned handle5037; textures observed in progress.

New verify_install_conversion_textures.py compares every raw source/native image
variant to independently checked TEX/font outputs, irrespective of spool order.
Actual texture serial279 committed1354016B/264variants all byte-exact against the
3754-variant reference. First verifier attempt lacked FHTXSPL4 parser support;
corrected to deployed codec schema, retained original failed observation. No app
or journal mutation. Complete-stage checking requires every expected variant and
validates the assembled v4 pack; no complete texture-stage claim yet.

run_install_game.py now accepts --converted-run as an alternative to --fixture.
converted_installation.py requires a completed owned full-bundle run, actual
startup retirement receipt, matching executable/source recipe, committed bundle
and manifest checksums, and every child file's size/CRC/SHA. It copies only eight
runtime children plus commit/manifest; conversion spools remain in original run.
No synthetic application journals are written. Prior synthetic-fixture mode stays
explicit. Both original evidence and copied runtime bytes are checked at exit.

Actual helper checks against the complete host fixture pass eight children;
2454base texture records/2861variants match the independent reference. Nine
negative cases reject wrong-recipe/incomplete commits, changed/truncated children,
wrong source metadata, genuine original-only ARM run, truncated texture spool,
CRC-valid changed GPU pixels, and missing expected image. CLI rejects the actual
partial conversion before creating a profile. Evidence install-conversion-evidence-
test/result.json. Full real-converter gameplay handoff remains UNVALIDATED pending
completion of live5037. No concurrent emulator or extra boot was launched.

## Remaining stages: actual ARM conversion continuing

`setup-conversion-resume-bundle-v1-new` advanced beyond the full source hash into
performance geometry. The inspected capture at startup poll23736 shows scan14/109;
observed allocator arena highwater was10,678,272bytes at that point. This is not
whole-conversion peak, hardware utilization or a controlled timing comparison.

`verify_install_conversion_performance.py` checks committed records against a
host reference, first requiring matching converter source/header dependencies.
It rejected the older host reference's five changed dependencies. A matching
reference was then generated from the exact deployed build's source snapshot,
without editing the worktree or relabelling any existing journals. That bounded
host run completed175.66seconds in `install-performance-deployed-reference`:
spool86d8633d4888, topology7aa59eb2d4cd, geometry2407db330636.

At live ARM journal serial1425, all907committed records (904rendering packets and
150patched sources) match the reference byte for byte. Committed prefix19,673,448
bytes, SHA256`bb37454fdb2321264012bc7b807fe9db3316ae8f7952044928cfae43b02e0aa9`.
Full reference contains2110records; this partial check is not full-stage or model
fidelity acceptance. The process remains live with its original10800second bound.

Reproduce the read-only observation with:

```
python3 /work/tools/verify_install_conversion_performance.py \
  /work/artifacts/native/setup-conversion-resume-bundle-v1-new \
  --build /work/artifacts/research/normal-setup-shadow-share-v1-builds/console-setup \
  --reference-report /work/artifacts/research/install-performance-deployed-reference/result.json
```

Add `--require-complete` only to require all four completed performance artifacts,
including a byte-exact cache comparison with independent host encoding. Do not
use partial success as a publication/playability result.

## Complete original stage on ARM (13 October)

`setup-conversion-resume-original-v1-new` completed the original geometry stage,
requested normal HID cancellation after that milestone, and exited cleanly at
poll27494in1217.92host seconds. This included another full ISO hash. It is not
an isolated performance benchmark (host checks/build overlapped), nor a console
time estimate. The cancelled-menu capture was inspected.

Independent verification with `--require-complete` passes all2,110source records:
2,102rendering packets byte-exact against the checked host reference, plus eight
nonrendering records. Completed artifacts are:

| Artifact | Bytes | SHA256 prefix |
| --- | ---: | --- |
| Original spool | 51,046,710 | 00d235935982 |
| Native geometry pack | 50,971,512 | 3b93a639bf89 |
| Block cache | 27,560,614 | f2eaab13a098 |
| Empty original topology | 16 | 5282fc33562f |

The entire scene and cache also match independent host generation, not just
checksums stored by the ARM app. Original state serial6729has required/complete15,
no partial artifact. The performance stage had begun before the scheduled B
pulse; its genuine465,400-byte checkpoint is retained for continuation.
Observed allocator arena highwater reached9,359,360bytes across the whole run,
including that initial performance stage. Normal reservation71,303,168bytes,
linear25,165,824; actual stack depth and full-installation peak are not measured.

The original ISO size/mtime, save sentinel and prior checkpoint hashes are
unchanged. Owned emulator exited. This establishes real interruption recovery
and complete original-stage correctness; performance geometry, textures, movie,
complete bundle and gameplay after actual conversion remain unvalidated.

Continuation `setup-conversion-resume-bundle-v1-new` now uses independent copies
of these actual completed/partial outputs with the same deployed binary. It is
bounded to10800host seconds and will request ordinary menu exit on a complete
bundle. No synthetic journals or source-hash bypass are used. Outcome pending.
Additional generated debug archives were losslessly NTFS-compressed with matching
before/after hashes, leaving about998MiB free before copying the new profile;
no assets, saves or evidence were deleted.

## Earlier checkpoint recovery observations

While the recovery test was running it advanced in the original generation from serial
2586 to3192 without changing its source/recipe identity. Independent prefix
comparison preserves all33,130,288previously committed bytes, SHA256
`ee426c41411f03b161fa75a570a8ccadf824adbf9526d9dfbbf66bba13090456`.
A separate packet comparison at serial2848 checked1,489rendering packets against
the full2,102-packet original reference; four nonrendering records were explicit.
These are actual outputs from the resumed ARM application, not seeded host data.
They establish forward progress with preserved checkpoints, not full conversion.

A read-only owned-display capture at startup poll23310..23311 was inspected:
original geometry scan88/109banks (80%), with build17e602b1c3a8 and safe-cancel
prompt. At that sample, allocator arena highwater was3,424,256bytes. The observer
validates PID/starttime/cwd/binary/code anchors and the private Xvfb environment;
it sends no input or guest writes. Complete original-stage acceptance is recorded above.

Local storage: NTFS compression of eight generated setup ELF archives freed
about91MiB; all before/after SHA256values match. Nothing was removed and private
assets/saves were untouched. Compression overlapped this diagnostic run, so its
wall duration must not be treated as an isolated speed benchmark.

## Adaptive completion controller (13 October)

`setup-adaptive-hid-v1-new` passed in 12.03 host seconds using the exact
currently deployed 3DSX. A future ordinary B pulse appended during execution
cancelled preparation; the second pulse exited the menu normally at poll422.
The cancelled-menu capture was inspected. ISO size/mtime and save sentinel
were unchanged; the owned emulator exited. This proves adaptive HID control,
not conversion. The appended-input emulator leaves the baseline binary and
sources intact and changes no guest clock or memory. Its optional file polling
means it is not a timing-equivalent replacement for prior performance benches.

`setup-conversion-resume-original-v1-new` ran from independent
copies of the genuine earlier 33,130,288-byte partial spool and its journals.
It retains full ISO hashing. A completed original stage triggers normal HID
cancellation; the process had an 1800-second upper bound. It passed as recorded above.

The completion observer checks recipe/source identities, journal CRC and
complete artifact sizes. This is only the cancellation trigger. The separate
`verify_install_conversion_geometry.py --require-complete` additionally compares
all spool packets, the complete scene pack and independently encoded block cache
against the checked host reference, and validates the exact empty topology.
It correctly rejects the previous partial stage. It does not claim whole-bundle,
console performance or gameplay acceptance.

13 October deployment update: user explicitly requested deployment before full
conversion acceptance. New223 installation completed and byte-readback verified:
`artifacts/console-test/20261012-174923-new223/transfer.json`.
3dsx87ca5200d925, CIA d8dd5bfc8f0f, menu17e602b1c3a8. Old229 unchanged.
This changes deployment status only; full conversion/resume remain unvalidated.

# Actual ARM ISO-only setup validation

The application under test is the archived shared setup build in
`artifacts/research/normal-setup-shadow-share-v1-builds/console-setup`:
3DSX `87ca5200d925`, CIA `d8dd5bfc8f0f`, menu source ID `17e602b1c3a8`.
The game archive and shaders are unchanged from its previously checked build.
No executable was rebuilt or deployed in this validation work.

`tools/run_install_conversion.py` starts an isolated silent Azahar profile with
only a hard link to the private original ISO and a save-preservation sentinel.
It supplies ordinary HID, uses the actual source hash and converters, and never
seeds converter journals or prepared assets. It unlinks its ISO alias and stops
only its owned processes on termination. The original ISO stays unchanged.

The initial bounded run, `setup-conversion-clean-v1-new`, passed cancellation,
menu retirement, save preservation and owned-process cleanup in 104.20 host
seconds. Its late image showed 227,803,136 of 1,459,978,240 source bytes verified
(15%). It did not reach a converter and is not a conversion success.

The longer `setup-conversion-stages-v1-new` run reached the original geometry
stage at 557.99 host seconds, startup poll 22,401. This uses the real complete
ISO fingerprint; there is no verification bypass. The stage-detection screenshot
still shows the preceding hash phase because UI presentation is throttled and
filesystem observation is asynchronous. Actual generated checkpoint records,
rather than this screenshot, establish that geometry conversion began.

A read-only observation compared the first 18,558,992 committed spool bytes:
833 records, including 830 rendering packets, matched the independently checked
host original-geometry pack byte-for-byte. Three nonrendering records had valid
spool checksums and no host rendering-packet entry. The whole host reference has
2,102 rendering packets. This is partial output coverage, not a completed stage.
`tools/verify_install_conversion_geometry.py` validates journal/source/recipe
identities and CRCs before comparing immutable committed packet bytes. It does
not read or accept the live uncommitted tail.

`tools/inspect_install_memory.py` locates the owned emulator by PID, start time,
working directory and executable, verifies two ELF code anchors and executable
segment placement, then cross-checks the live startup input counter. It reads
SDK allocator high-water marks without guest writes or pausing. Sampling uses
only the tail of the HID trace, not a repeated full-history scan.

The hash phase used a 270,336-byte allocator arena high-water mark. Geometry
conversion subsequently reached at least 3,563,520 bytes. These are allocator
arena maxima including retained free chunks, not live object counts. The normal
reservation is 71,303,168 bytes; linear reservation is 25,165,824 bytes; the sbrk
base lies 524,288 bytes after the normal-heap base. This does not measure actual
stack depth, a full-conversion peak, GPU work or physical console timing.

The source-verification wall time is emulator evidence only. It is a one-time
setup/resume cost, not gameplay FPS, and cannot be extrapolated to console time.
The current source verification restarts on Prepare/resume; ordinary ready-menu
startup only checks source header/FST/size and committed bundle metadata.

## Packaging and deployment

`tools/package_console_setup.py` now packages an explicitly archived setup
build, verifies its binary/CIA/shared ELF/source snapshot/recipe records, and
includes the converter license. The same 3DSX/CIA pair targets Old80/New124.
The actual console-transfer payload selector has a setup-specific branch that
preserves settings, saves and private assets rather than installing the older
rendering-control configuration files.

Package `build/console-validation/foxhollow-iso-setup-534149c1a120d25b.zip` is
4,367,984 bytes, SHA256
`534149c1a120d25bcd40273e6287d22b3664ee35bf0dd99f726a6a1f88c74d86`.
It remains a development package, not a fully validated installation.
`iso-setup-package-test/result.json` records reproducible package bytes, identical
Old/New payloads and nine rejected mutations: settings/save/asset injection,
wrong recipe/source, non-setup binary metadata, wrong memory grant, mismatched
CIA metadata, and altered CIA bytes. The existing unified-package CIA mismatch
check also passed. No runtime rendering/settings behavior changed.

Read-only FTP inspection at `console-setup-check/20261012-172324-223` found
`SD:/3ds/foxhollow/disc.iso` at the expected 1,459,978,240-byte size. This was a
size check, not a fresh remote hash. Existing executables remain installed;
there was no remote write, save change or deployment.

Full ARM/physical conversion, resume after real interruption, complete installer
memory/storage/time requirements, effective fast geometry and full gameplay/
rendering/performance acceptance remain unproven. Continue from retained real
checkpoints; do not relabel synthetic prepared-fixture runs as conversion proof.

## Final bounded-window outcome

The longer run reached its 900-second host deadline during original geometry
scan (53/109 banks in the inspected final image). Its summary correctly remains
**failed**: the scripted B press at startup poll 28,000 was not reached; startup
polls advance more slowly during model conversion. The runner stopped its owned
process at poll 24,984. This was a harness time-limit stop, not an observed app
crash, and does not prove normal conversion cancellation or ARM resume.

After that stop, both journal slots retain valid CRCs. The newest immutable
prefix is 33,130,288 bytes, 1,410 records, and **1,407 byte-identical rendering
packets** against the 2,102-packet host reference. The three nonrendering records
remain explicit. No stage or complete bundle was activated. The original ISO
size/mtime stayed unchanged, its temporary alias was removed, and the save
sentinel remains intact. Checkpoints and all failure evidence are retained.
The observed allocator arena maximum remains 3,563,520 bytes (3.40 MiB), plus
the separately reserved 512 KiB main stack; actual stack depth is not measured.

All owned emulator/observer sessions are now terminal. Next integration work
must resume these genuine checkpoints and use a completion-aware controller or
an appropriately bounded full-conversion run; another short fresh hash-only boot
would add no useful conversion coverage. Full-source verification remains in
place. No hardware request or deployment follows from this partial result.
