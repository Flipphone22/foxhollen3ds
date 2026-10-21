# ISO conversion search hints

The user authorizes hashes and search-decision metadata, excluding converted
game assets from distribution. This supersedes the earlier blanket seed ban.
The ISO remains the source of every colour, selector, coverage value and mesh.

`FHTXHNT1` stores two bits per ETC block: orientation and colour mode. The
high-quality search within that mode, including refinement and differential
constraints, stays unchanged. It stores no endpoints, intensity tables, pixel
selectors, alpha planes or native texture blocks. The private reference pack
is an offline oracle and is never packaged. Source and output SHA256 hashes
bind each image to its hints. The codec hash names the checked unseeded encoder
contract, not the current accelerated implementation's source hash.

There are 1,722 records / 2,918,508 blocks: 729,627 decision bytes, 165,312
directory bytes and a 96-byte header. Total 895,035 bytes, compressed to 649,696.
The exported app metadata is `native-3ds/data/install-texture-hints.zlib`.
It covers full, reduced, alpha and padded ETC variants. Uncovered/rejected ETC
attempts and other formats use the ordinary converter; no quality gate changes.

The immutable reader validates the expected file hash, ISO identity, codec
contract, CRCs, sorted keys, dimensions and every extent before lookup. The
encoder owner hashes actual source bytes, borrows the selected two-bit sequence
through both worker halves, and checks the output hash before publication.
Failure/cancellation never publishes the partial output. Expanded metadata is
freed after worker join at texture-stage exit. Startup temporary RAM: 895,035
bytes plus existing zlib scratch; embedded read-only size: 649,696 bytes. The
latter remains a process-memory cost during gameplay. Actual linked/process
headroom still needs measurement. No extra worker, GPU operation or SD hint read.

Consulted pinned Melee `render_worker.c` / `gx_thread.c`: retain borrowed storage
through release/acquire completion and join before freeing. This reuses the
existing startup-only worker; New core2 is available before gameplay audio,
Old uses the serial path. No new hardware/cache-coherence mechanism is added.

Evidence:

- `install-etc-mode-hints-v1-test`: 632 blocks, three paired host CPU passes,
  0.421259 s full search versus 0.110630 s hinted (3.81x); exact output,
  ASan/UBSan and ARM compile pass. Not console or whole-conversion speed.
- `install-texture-mode-hints-collection-v2-test`: 53,996 sampled blocks across
  all images, sanitizer clean; untested blocks explicitly use oracle bytes and
  are not counted as encoding evidence.
- `install-texture-mode-hints-full-v1-test`: all 2,918,508 blocks encoded with
  prototype hints, all output bytes exact, 68.7832 host CPU seconds. No paired
  full-search timing, so this is not another speedup estimate.
- `install-texture-mode-hints-api-full-v1-test`: all blocks through production
  optional-mode API under ASan/UBSan, all bytes exact (including 65 alpha,
  407 reduced and 185 padded images). 35 malformed tables, 1,722 wrong sources
  and 1,722 damaged outputs rejected. ARM reader compilation passes.
- `install-texture-search-owner-v2-test`: actual embedded owner, 16 serial/
  parallel cases, 48 cancellations, 12 worker jobs, 10 load/free cycles,
  wrong-ISO/reentrant-open rejection, guarded buffers, unchanged inputs and
  producer-only callbacks pass ASan/UBSan. Four production units ARM compile.
  V1 host checks passed but strict ARM compilation rejected misleading
  indentation; fixed before v2. Preserve that failed report.

The isolated setup builder opts in with `--texture-search-hints` (requires
`--padded-textures`), alongside RGB8 movie, checkpoint batching and checked
shadow geometry options. It patches only texture conversion calls/stage lifetime
and versions the recipe with reader, owner, crypto library and hint bytes.
Build provenance includes the exact compressed blob. Earlier caches/outputs
are never rewritten to fabricate compatibility. No deployment yet.

Shared build cc0a00b4c399: 3DSX9697ab8dd604 / CIA7408845f17e0, local only.
`hint-build-verification.json` checks all574 source hashes, exact embedded bytes,
all three redirected encoding calls, stage lifetime, symbols and build envelope.
The isolated builder restored shelved work and the game archive. Startup check
`setup-search-hints-ui-new-v1` passes25.49hostsec, real B cancellation/exit and
unchanged source/save. `before-cancel.png` inspected: readable overall rough
percentage, elapsed time and ETA, separate current task (source verification).
The initial ETA still uses conservative old phase priors; not a measured new
whole-conversion prediction.

`install-search-hints-paired-new-v1` passes the actual ARM code in headless
Azahar18.98hostsec. Same128x128 fixture, three alternating AB/BA comparisons:
serial full2641.266ms/hinted772.620ms; worker full1485.828ms/hinted419.029ms,
including source/output SHA checks. All native bytes and quality metrics match.
Metadata open287.802ms once; worker stack upper24004/131072. These are emulator
component measurements, not physical or complete-conversion estimates.
The old baseline fixture and quality remain unchanged. No overclocking.

Fresh ISO-only seeded conversion completed at 4,128.821hostseconds (68m48.821s),
8m48.821s over the one-hour target. It exited normally and passed full content
verification. All 3,754 variants and the complete
84,776,872-byte pack now match the independent reference byte-for-byte; movie
preparation began at 2,936.998490 host seconds. Application phase texture time is
1,835,028ms; complete application work4,110,818ms. No physical timing claim.
New-mode movie/cockpit loading from this genuine converted bundle passes with
captures inspected; this does not establish complete gameplay or AV sync.
The baseline's final content verification also passes, comparing
every decoded frame/audio byte and reconstructing the exact movie header,
frame links and padding from its verified spool. Original byte-level failure
is retained separately. Host/ARM LZ4 bytes differ losslessly.

`install-movie-content-validation-v1-test` additionally rejects15 virtual
corruptions without changing any converter output. Changed pixels, audio and
source identity have coherent CRCs and still fail content comparison; packed
header/link/payload/padding/trailing-byte corruptions fail assembly validation.
The real1797-frame pair passes despite1786 different compressed payloads.

13 October follow-up: user now accepts under70minutes. The readback/checkpoint
candidate d23068f52b22 freshly completes at62m58.370s with the SAME texture hints
and exact assets; full validation and New/Old startup handoffs pass. Packaged
as foxhollow-iso-setup-191072361db3f64b.zip, not deployed (New223 FTP refused).
Further geometry-hint integration is parked; it is absent from this binary.
Physical conversion duration and complete renderer/gameplay goals remain open.
