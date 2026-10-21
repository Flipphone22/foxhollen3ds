# Original geometry compressed working cache

11 October DEPLOYED New223: normal70038555df22/CIA9a72e459b998 and
replayde1d418ee54c/CIA46068c043764. One shared Old80/New124 pair; the cache is
admitted only on New hardware. console-settings-update/20261010-165737-223
is complete: all changed files received full staged byte readback, rename and
final-size verification. Both save hashes are unchanged; preferences preserved.
Original geometry reused prior completed byte proof plus current identity and
sizes, not a fresh full reread. No Old deployment. CIA needs FBI installation;
the matching 3DSX is ready. Physical cache benefit remains unmeasured.

The existing original-geometry installation had no corresponding cache. A new
lossless LZ4-HC12 container retains all51,312,616 original bytes in22,879,923 disk
bytes. It uses the existing7,340,032-byte normal-RAM working window, not full-file
residency. Full startup decode/CRC validation remains. Misses in later regions
still read SD and are counted; normal-heap pressure can release the cache before
an asset admission. The original uncompressed package remains authoritative.

The cache reader now uses fh_asset_open_staged before its first operation, just
like the recently fixed scene/texture pack readers. Its existing64KiB CPU input
and decode buffers make another stdio buffer redundant. This avoids newlib seek
prefetch/refill amplification. No GPU destination, resource lifetime, geometry,
texture, render program, simulation, save, new worker or allocation limit changes.
The pinned Melee file_io.c resident/bounded-buffer design was consulted. Here we
reuse Foxhollow's existing compressed window and staging, rather than introducing
Melee's additional reader buffers. New core2 remains occupied by audio.

## Validation and scope

Actual host cache code passes4,000 random reads for full residency, windowed and
HC containers,92 malformed rejections, guard/EOF checks and stream-adapter tests
including mid-packet pressure release and corrupt-cache fallback, under ASan and
UBSan. The actual51.3MB original package passes full-byte comparison and4,000
random reads in the7MiB window, including5,372 misses/5,378 evictions and resident
corruption rejection. See original-geometry-cache-v1/window-validation.json.

The New Arwing approach replay uses original geometry/textures with fur, specular
and silhouette shadows enabled. Native state assertions, saves, shutdown/drain
and owned exit pass. Actual captures1650 show the landed Arwing's panel texture,
blue sections, nearby tree, ground, Fox and HUD. They do not reproduce the user's
physical all-white/disappearing-object report and are not full image parity.

| Observation | No cache | Cache |
|---|---:|---:|
| Whole-run scene SD reads after opening |417|0|
| Whole-run scene SD bytes |3,311,364|0|
| Cache logical reads / returned bytes |0 /0|435 /3,378,644|
| Cache decodes / pressure releases |0 /0|127 /0|
| Startup cache attempt/verification, emulated ms |26.03|9458.33|
| Measured approach Azahar FPS |53.6088|54.2209|
| Scene CPU ms/frame |2.008877|2.009743|
| Native submission CPU ms/frame |4.236625|4.221169|
| Peak normal heap / reservation, bytes |48,836,488 /71,303,168|56,177,088 /71,303,168|

The435 figure is logical cache requests; the baseline417 figure is actual scene
SD requests. Their workloads differ slightly; do not equate them one-for-one.
The uncached run had a read-only host object observer, and candidate build work
for the normal variant overlapped startup. Animation phase/counts differ.
These are short relative CPU/counter observations, not a physical FPS win or an
exact fixed-frame A/B. The cache's526.44ms total decode/read span is whole-run
load work, not an additional per-frame cost. Startup validation explicitly moves
work into initial loading. Physical startup time/stall reduction is unvalidated.

New normal heap retains15,126,080 bytes free before stack allowance and stays
below the existing85% admission ceiling. Both runs have minimum linear free
16,539,008 /25,165,824, free VRAM4,846,592, measured stack72,860 /524,288 and
process commitment103,186,432 /130,023,424. Old disables this cache path; its
unchanged reader/memory behavior evidence is reused, no new Old runtime claim.
Normal/replay share the game archive,68 renderer sources, cache sources and the
previously float24-tested511-instruction shader. Original assets are unchanged.

The headless runner now accepts --original-scene-cache independently of the
performance geometry cache and checks source/container hashes. The installer
accepts --original-cache, verifies source identity and transfers it to each app's
original directory using full staged readback. It preserves saves/preferences.
Build/validation scripts and source snapshots are in original-cache-v1-builds,
original-cache-normal-v1-builds and original-cache-runtime-test. Initial build
verification failed only because its report folder was absent; the created
folder and successful rerun retain the actual checks.

Missing lighting/shadow families, fur wind/layers and physical visibility bugs
remain open. This is an I/O improvement candidate, not whole-renderer acceptance.

Future `prepare_original_scene.py` outputs now include `native-scene.cache` and
its identity manifest automatically. `install_visual_settings_update.py` detects
that matching companion, or accepts an explicit `--original-cache` for an older
prepared directory. Headless checks use `--original-scene-cache` and
`--allow-scene-cache-misses`; no claim of full-file residency is implied. The
normal distribution includes this document. Old hardware continues to ignore
this optional CPU cache; the shared executable profiles are unchanged.
