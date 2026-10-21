11 October New223 deployment COMPLETE: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`. New texture
page caching plus the accumulated environment-normal/unlit-material corrections
are installed. Eight changed application/identity files received full staged
byte readback, rename and final-size checks. Ten original asset/cache files
reuse prior completed byte proof plus current descriptor/topology bytes and
sizes; large asset bytes were not freshly reread. Saves/preferences preserved.
Old229 is unchanged. Matching3dsx ready; CIA requires FBI installation.
Physical stalls, fur and reported visibility/lighting acceptance remain open.
Evidence: `artifacts/console-settings-update/20261010-190758-223/manifest.json`.

11 October texture reader cache LOCAL: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`, not deployed.
Native materials now attach the existing staged page reader: 256KiB New /
64KiB Old normal RAM, reclaimable under heap pressure. No GPU allocations,
per-frame geometry work, new worker or changed shaders. Full pack/lifetime/
corruption tests and serial original-Arwing New/Old replays pass; images
inspected. Pack file reads 15.10->6.88MB New, 15.10->13.32MB Old, identical
bytes returned to the renderer. Rendering cost effectively unchanged;
Azahar55.04/47.37FPS is not physical performance. Physical stalls and reported
visibility/fur/lighting defects remain open. See [evidence](NATIVE_TEXTURE_WINDOW.md).

11 October cache follow-up now DEPLOYED New223, normal70038555df22 /
replayde1d418ee54c. See ORIGINAL_GEOMETRY_CACHE.md for scope, memory, initial
loading cost and completed byte-readback evidence. No physical stall-fix claim.

11 October local follow-up: original geometry now has a matching lossless
7MiB working cache, validated on the New Arwing route. Cache reader also uses
its existing staging without redundant stdio buffering. See
[original geometry cache](ORIGINAL_GEOMETRY_CACHE.md). Physical stalls remain
unvalidated; this is not a claim that every asset read is resident.

# Prepared asset read amplification

10 October 2026. Deployed New 3DS; not yet physically validated.

New223 normal `3ddc672fddee` / CIA `afc13aa448b5`, replay `a05052e6976b` /
CIA `c1a572eb0c1e` were staged, read back byte-for-byte, then renamed and
size-checked. `artifacts/console-settings-update/20261010-154949-223/manifest.json`
is complete. Save hashes and graphics preferences were preserved. Original
geometry was reused from prior completed byte proof plus current manifest,
topology and sizes; its large scene file was not freshly reread. Old229 was
not updated. CIA requires FBI installation; matching 3dsx is ready.

Normal and replay share the checked game archive, all 67 rendering source
hashes and the previously tested exact 511-instruction shader. All three
changed asset I/O files also match. Shared CIA profiles remain Old80/New124.
No fresh normal UI run was needed for this I/O-only follow-up; unchanged UI
evidence is reused, not represented as a new normal runtime check.

The New 3DS run collected in `artifacts/console-visual/20261010-153312-223`
has 10.482, 5.300 and 5.182 second presentation gaps. Corresponding cumulative
counter windows contain 2,278 / 973 / 1,189 reads and 9,091.727 / 4,381.954 /
4,859.751 ms of read-call spans. Only 21.184 / 691.977 / 0 ms belong to the
disc/movie subset. These overlapping service spans are not CPU utilization or
an exact partition of the presentation gaps. Many requests are below the
50 ms slow-trace threshold; the trace's longest individual call misses this
amplification.

The native scene and prepared texture loaders already use 32 KiB CPU staging.
Their streams previously retained default newlib buffering. The actual linked
ARM library can prefetch during a seek and split a bulk read into a direct
portion plus a buffer refill. `fh_asset_open_staged` now disables that second
buffer before any stream operation. This applies only to these two explicitly
staged binary pack readers. Configuration, saves, audio, disc/movie, logs and
the optional compressed block-cache reader retain their existing policies.

The pinned Melee implementation was consulted at
`reference/melee-3ds/native-3ds/port/3ds/file_io.c`: its bounded buffered readers
avoid redundant seeks and keep storage alive until close. Foxhollow instead
already owns staging and texture windows, and uses random packet boundaries;
adding another large stdio buffer overreads data here. This adaptation removes
the redundant buffer rather than copying Melee's 64 KiB stream policy.

No GPU ownership, resource lifetime, cache publication, source-byte identity,
CRC, admission limit or rendering code changed. Filesystem reads still land
in CPU staging before copying into unpublished GPU memory. No new worker,
allocation, per-frame I/O or logging was introduced. Short reads and failures
retain the readers' existing rejection behavior.

## Checks and measurements

`tools/test_arm_asset_stdio.py` executes the installed devkitARM newlib under
ARM11 emulation, with a deterministic read-only device and single-thread lock
stubs. For 128 seeks/32 KiB reads: default buffering makes 382 device reads;
the actual new helper makes 128; a separate 32 KiB stdio buffer makes 191 and
reads 6,258,688 bytes instead of 4,194,304. It checks returned bytes, short-read
continuation, EOF and seeking after EOF. This is a synthetic access pattern
and device-call count, not a physical FS latency or throughput measurement.
The library hash and results are in `artifacts/research/asset-stdio/result.json`.

The actual C texture reader passes 2,054 fixture checks with 647 rejected
malformed cases under ASan/UBSan. All 482 entries of the private texture pack
also compare byte-exactly in uncached and 32/128/256 KiB window configurations,
including eviction and EOF; no filesystem read targets GPU/window output.

Serial New/Old Azahar cockpit replays compare against the alpha-source build,
with the same assets, options, recipe and fixture. State assertions, save
preservation, log drain and owned-process exit pass. Captures at retrace900
were inspected on both configurations. Whole-run non-disc/movie read calls
fall 562 to 302 on both; corresponding bytes fall 2,923,230 to 2,860,294.
Movie/animation phase differs slightly. Synthetic FS timings are not forecasts
of the console's SD card or its service scheduling.

Steady scene/submission means, in ms:

| Configuration | Before scene / submit | After scene / submit |
| --- | --- | --- |
| New | 0.773680 / 1.943840 | 0.769276 / 1.944681 |
| Old | 0.776623 / 1.942155 | 0.773013 / 1.942761 |

These noise-scale rendering differences are not a speedup claim. Both paced
replays remain approximately 59.8 Azahar FPS. Heap peaks decrease 2,072 bytes:
New 47,853,400 / 71,303,168; Old 40,513,368 / 50,331,648. Minimum free linear
RAM is unchanged at 18,484,480 / 25,165,824 and 3,804,416 / 10,485,760.
Free VRAM remains 4,846,592; measured stack is 72,740 / 524,288. Process peaks
remain 103,186,432 / 130,023,424 and 67,534,848 / 83,886,080.

Evidence and builders live in `artifacts/research/pica-scene-rewrite/`:
`asset-stdio-runtime-test`, `build-asset-stdio-v1.py`,
`build-asset-stdio-normal-v1.py`, `check-asset-stdio-v1.py`,
`analyze-asset-stdio.py`, `verify-asset-stdio-builds.py`.

## Remaining work

This removes demonstrated redundant calls; it does not establish that the
physical multi-second pauses are gone. Substantial synchronous asset work,
source verification and conversion remain. Original geometry currently lacks
an installed compressed cache, so that route must not be described as resident.
Physical read counts/gaps should be compared on a run that crosses areas.

The included alpha-source correction and fur coverage work have their own
limited evidence. Disappearing trees/objects, white Arwing, day/night lighting
and shadows, and faithful fur remain open. The current fur appearance is not
accepted. This I/O change makes no claim to repair them.
