# Native texture preparation

The on-console installer is still being integrated. These modules run in the
startup task and do not change the live renderer, game state, or installed assets.

`install_texture_exact.c` prepares source-precision PICA tiles for GX formats
I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8, C4, C8, C14X2 and CMPR. Indexed inputs
require an explicit validated palette. Padded dimensions replicate edge texels.
The caller owns output storage; conversion adds no heap allocation, thread,
GPU resource or cache-publication owner. Largest allowed output is 4 MiB.
Cancellation is checked before and after each image, not each row; worst-case
console cancellation latency remains unmeasured.

This adapts the reviewed pinned Melee `texture_repack.h`, `texture_decode.h`,
`texture_storage.h` and `texture_visibility.h` already used in the native port.
It adds strict source/palette extents, alignment/overlap checks and cancellation.
The pinned decoder's out-of-range palette-index behavior is retained.
No legacy rendering interpreter is involved.

## Compression-quality failures

The CMPR stage retains its ETC1/ETC1A4 RGB-quality gate. A rejected lossy
candidate now gets exact RGBA8 output instead of an absent pack entry.
`FHTXSPL2` retains the failed error measurement and source identity; readers
still accept old v1 spools. Recipe identities must change with the converter:
an old completed spool must not be reused as if its omissions were repaired.

The FHTXPK2 reader admits RGBA8 only at full source dimensions, with strict byte
extent checks. That version's source identity remains **CMPR-specific**. The
new typed FHTXPK3 path described below adds explicit source format identity.

All five real quality rejects pass exact conversion, assembly, runtime lookup
and readback in a focused fixture. Their native payloads total 138,240 bytes;
largest is 131,072 bytes. These are storage quantities, not measured resident
RAM deltas or performance claims. No full new collection was encoded/installed.

Evidence under `artifacts/research/`:

- `install-texture-exact-test/result.json`: 90 independent scalar pixel-oracle
  cases, 2,986,080 compared bytes, 180 cancellations, ASan/UBSan and ARM compile.
- `install-texture-exact-real-pack-test/result.json`: 14 full/two reduced
  entries, six lossless entries (five real plus a synthetic reject), duplicate/
  CRC-collision checks, cancellation/resume, corrupt pending-spool rejection
  preserving the previous generation, and ARM component link. Fixture peak
  explicit heap 344,064 bytes excludes library internals and caller inputs.
- `texture-exact-reader-regression/summary.json`: 2,054 existing reader checks.
- `install-texture-exact-prepare-test/result.json`: unchanged real one-bank ISO
  fixture, four pack entries, image/assembly resume, no whole-game activation.
  This ran before the scanner refactor; the complete record-stream test below
  separately checks that refactor's preserved CMPR subset.

## Complete source inventory and plane boundaries

`fh_install_texture_scan_record` exposes every decoded table record. The old
CMPR convenience API retains its 3,899 eligible records in the same order.
Independent extracted-source comparisons verify all 19,264 ISO records, their
headers and decompressed CRCs.

`install_texture_layout.c` interprets base colour and secondary I4 opacity from
original `textureInitGXTexObj` and `textureInitSecondaryGXTexObj` in
`main/texture.c`. They remain separate typed images. It validates base extents
before publishing borrowed pointers, reports surrounding mip extents and checks
declared chain completeness. It allocates nothing and does not guess palette
locations or unwrap nested compression records.

Across 19,263 direct records there are 20,207 base planes, including 944 opacity
planes. They represent 2,454 unique typed source images. Keeping all uncompressed
would require 169,510,752 native bytes on SD (largest 1,048,576 bytes), before
source identities, mip levels and metadata. This is not a RAM allocation plan.
Accepted compression must remain; the current 128 MiB pack cap cannot simply be
assumed adequate for a raw pack.

Two records (FST193/offset576896 and FST3974/offset1277760) each contain
174,752 payload bytes against a declared 174,784-byte chain. Their bases are
valid; full-chain completeness remains false. One 64-byte preload record is a
nested ZLB wrapper, not a direct texture header, and is explicitly rejected by
the layout function. These are outstanding coverage items, not silently accepted
full conversions. Indexed formats do not occur in this direct-header inventory;
that does not establish whether they are generated at runtime.

`install-texture-records-test/result.json` and
`install-texture-layout-test/result.json` record actual ISO/source comparisons,
all base-plane CRCs/extents, 77,052 invalid-layout rejections, ASan/UBSan and ARM
compilation. The subsequent stage below resolves base-plane preparation and the
nested preload wrapper. Full recipe/menu integration and runtime mip fidelity
remain incomplete. No FPS improvement or full visual parity is claimed.

## Typed pack and integrated base-plane stage

`FHTXPK3` extends each directory entry from 40 to 44 bytes with a source-format
field. The reader retains FHTXPK1/2 support. Lookup compares source format,
dimensions, exact source extent, CRC and actual bytes; a CRC match alone never
establishes identity. Resident sharing also includes source format. Equal byte
strings interpreted as I4/CMPR, I8/IA4 or IA8/RGB5A3 cannot alias.

`FHTXSPL3` adds exact, typed non-paletted images to the resumable spool. CMPR
images eligible for the existing compressor retain its quality gate and optional
reduced variant; other formats and non-power-of-two images use source-precision
native storage with edge padding. A CMPR-only spool still assembles byte-identical
FHTXPK2 output. No palette layout is guessed. Preparation's fixed source directory
grows by 16 KiB (4,096 entries times four bytes); no fixed resident texture pool
or file-size cap was enlarged. The game still enforces its 128 KiB directory
budget, 128 MiB file cap and existing texture residency limits. Full-collection
fit must be checked before publication.

`install_texture_prepare_all.c` connects the complete ISO scanner, source layout,
typed spool and pack. It converts base colour and separate opacity. Between those
two planes the journal retains the enclosing record's starting cursor. A restart
there deduplicates completed colour and still converts opacity; it never advances
past an unfinished plane. Finishing this stage does not activate a whole game.

The short TEXPRE wrapper is decoded as one bounded ZLB with validated stream end,
decoded length and zero alignment padding. Invalid or recursive/unsupported
layouts fail explicitly. The two inconsistent mip declarations remain counted.
This recipe deliberately prepares **base levels**, matching current native
sampling; it does not claim original mip filtering or complete mip-chain parity.
Coverage counters describe the current invocation, not previous resumed work.

The live `platform_pica_material.c` image loader now calls typed lookup. Source
conversion remains available for dynamic or unprepared images. All changes occur
at image loading; draw bindings, GPU cache publication, fences and resource
lifetimes are unchanged. `pica_typed_texture_patch.py` applies this same change
idempotently to a build's copy of the stable source. Frozen snapshots must not be
rewritten, and future isolated builds must apply the patch before compilation.
The inactive old loader was not reconnected or changed.

Validation:

- `install-texture-typed-pack-test/result.json`: 31 source cases, 32 pack entries,
  independent pixel/reference output, three equal-byte/different-format pairs,
  cancellation and assembly resume, duplicate reuse, disk/window/resident reads,
  192 checksummed malformed directories rejected, ASan/UBSan and ARM compilation.
  Fixture pack 199,088 bytes; resident representation 181,280 bytes.
- `install-texture-typed-cmpr-regression/result.json`: the preceding fixture's
  342,448-byte pack remains byte-identical. Explicit transient fixture peak
  rises from 344,064 to 360,448 bytes from the typed source directory.
- `texture-typed-reader-regression/summary.json`: all 2,054 existing reader checks.
- `install-texture-all-prepare-test/result.json`: ten unchanged original records
  in a private small ISO; 12 planes deduplicate to 11 entries (321,268-byte pack).
  Opacity interruption/resume, assembly resume, source preservation, nested
  preload conversion and four malformed-wrapper rejections pass ASan/UBSan/ARM.
- `typed-texture-consumer-test/result.json`: stable source copy with only live
  material lookup changed compiles with the SDK; patch is idempotent. Initial
  checks exposed a formatting-only anchor difference, a missing source include
  path, and a mistaken expectation that the inactive old loader emitted a live
  lookup symbol. The old-loader patch was removed. No full application was built.

`prepare_native_iso_texture_bases.py --seconds 120` runs/resumes the actual native
stage on the complete private ISO, with cooperative deadline cancellation and
source/recipe/binary identity checks. It preserves earlier attempts and refuses a
live duplicate or changed recipe. Its output is under
`install-texture-bases-full-build`; inspect `result.json` for current completion.
Full collection audit, combined memory/SD budgets, startup activation and gameplay
cost/visual checks remain required. No new normal/replay build is deployed.

The first 120.037-second bounded run stopped gracefully after 503 visited records
and 540 completed planes (counts include duplicates). Its durable 5,105,312-byte
spool contains 408 unique sources: 336 exact entries, 72 compressed entries and
11 reduced variants. `audit-checkpoint-00000542.json` checks the journal/prefix
CRCs, every source against original bank bytes, every exact payload against the
independent scalar pixel oracle, and compressed/reduced payloads against the
preserved native reference. This is partial collection evidence, not console
conversion timing. The recipe is
`d03b1822de395c0bed448cee9940cf024fb30e26fbdcbe627ea3b117ca8982b7`;
resume it with the same driver rather than restarting or changing its sources.

Non-power-of-two CMPR currently uses exact RGBA8 in this preparation recipe,
where the unprepared renderer may use RGB5A1. That can increase live texture
storage; do not deploy this collection without checking residency and loading
cost. Source-correct padded compression is a remaining performance opportunity.
Host transfer validation also still needs typed-pack capability/size support
paired with a verified matching executable before any such pack is deployed.

A subsequent bounded 300.033-second continuation reached 502 unique sources:
354 exact entries, 148 compressed entries and 46 reduced variants, with
9,206,368 durable bytes. `audit-checkpoint-00001062.json` independently checks
every durable source/output by the same method. It ran alongside the independent
movie converter, stopped cooperatively with exit0, and preserved the ISO.
Neither conversion is complete. The same recipe and checkpoint remain resumable.

That 502-source prefix includes 44 exact CMPR images, whose RGBA8 storage totals
305,664 bytes more than the prior unprepared RGB5A1 representations; largest
single increase is 65,536 bytes. `prefix-raw-storage-cost.json` records the
individual dimensions. This is a static collection sum, not simultaneous live
residency or an FPS result. Runtime budgeting or source-correct padded compression
must address this before deployment.

A further 300.057-second continuation reached 715 unique sources: 503 exact
entries, 212 compressed entries and 81 reduced variants. The durable spool is
14,812,096 bytes, with 8,538,784 bytes of native payload. All outputs pass the
same independent original-pixel/reference comparison in
`audit-checkpoint-00002422.json`. This overlaps build, movie-conversion and short
menu checks; it is not a conversion-speed benchmark. The ISO is unchanged,
no partial output was activated, and full coverage/residency checks remain open.

The next bounded continuation reached 858 unique sources (546 exact, 312
compressed, 110 reduced), with a 20,501,504-byte durable spool and 12,019,712
native payload bytes. `audit-checkpoint-00003314.json` passes every retained
source/output comparison. It stopped cooperatively after 300.034 host seconds.

`audit_typed_texture_budget.py` independently scans all 19,264 source records,
20,208 base/opacity planes and one nested wrapper. It finds 2,454 unique sources.
Combining their native layout with the previously completed CMPR conversion
now conservatively forecasts 2,861 runtime entries (up to 2,862 with another
reduced variant), a 125,928-byte maximum directory (128 KiB budget) and an
86,865,324-byte pack (128 MiB limit). The initial forecast incorrectly omitted
five power-of-two CMPR sources absent from the older reference pack. Absence did
not prove the new preparer would omit them: it can retain exact RGBA8. Those five
are now budgeted as RGBA8; the initial report is preserved. These are layout forecasts,
not a completed new pack. All source bank hashes and record CRCs were checked.

The same audit finds 210 non-power-of-two CMPR images that would occupy more
GPU storage than the existing RGB5A1 conversion. Across the collection this adds
8,606,464 bytes; the largest single increase is 524,288 bytes (a 320x274 image
in cloudrace becomes a 512x512 RGBA8 texture). This is not a simultaneous live set.
The material owner still caps textures at 4 MiB and preserves 15% of linear RAM;
the new pack is not accepted for gameplay until this increase is addressed.
Image identities, dimensions and referring banks are in
`artifacts/research/typed-texture-budget/report.json`.

`install_texture_padded.c` is a new, **unselected** startup codec candidate. It
copies CMPR endpoints and clamps selectors at the actual image edge, then calls
the existing high-quality ETC1/ETC1A4 encoder. It changes neither image dimensions
nor coordinates. Padding uses at most 512 KiB temporary normal RAM, freed before
return. RGB error is recomputed over visible source pixels only, so a large flat
padded region cannot make the RMSE gate appear better. Caller-owned output and
the existing encoder remain additional memory costs; whole-installer peaks are
not yet measured. No runtime geometry, draw, worker or GPU state changes.

`install-texture-padded-test` passes fourteen synthetic dimension/alpha cases,
8,832 edge/output pixel checks, 56 padded intermediate comparisons and 42
cancellations under ASan/UBSan, plus bounds/overlap/source-immutability checks.
The padded intermediate exactly matches the pinned original decoder's clamping;
encoded alpha is exact. The new unit compiles with the ARM SDK (152-byte static
frame, excluding callees). This is not original-image quality acceptance or a
physical test. The existing recipe and its checkpoints remain unchanged.

Before adoption, compare original affected images under the existing RMSE gate,
then explicitly version and extend the spool/pack validator and runtime reader:
current compressed entries require power-of-two source dimensions and reject
this new case. Do not label the new output as compatible with the current recipe,
restart old conversion blindly, or deploy it through an older executable.

`check_padded_texture_images.py` has now tested all 210 padded CMPR images from
the original banks. Sources are revalidated against bank hashes, record CRCs
and image SHA256. Encoded alpha matches every source/padded pixel; visible RGB
error is independently recomputed from the output. 185 images satisfy RMSE14;
25 fail and retain their exact RGBA8 representation. The worst rejected RMSE is
31.098. Four large accepted image pairs and the worst rejected pairs were
inspected; the rejected set includes small coloured UI marks whose detail must
not be damaged to save memory.

The resulting selection occupies 2,386,208 bytes across this collection versus
17,212,928 bytes for the current exact prepared images and 8,606,464 bytes for
the old RGB5A1 representation. These are measured encoded sizes, not simultaneous
live residency, image-cache behaviour, gameplay screenshots or an FPS result.
The five separately unmatched power-of-two sources are outside this 210-image
cohort. Evidence and private comparison images are under
`artifacts/research/padded-texture-originals`. This run completed in 131.995 host
seconds after two harness issues were fixed (dependency include resolution and
a duplicate metadata field); no console conversion-time claim is made.

`fh_texture_pack_padded.h` and `pica_padded_texture_reader_patch.py` add an
explicit **FHTXPK4** reader on a selected source copy. Earlier headers retain
their old validation; the new case requires CMPR, unchanged source extent,
power-of-two padded native extent, and exact ETC1/A4 byte size. Ordinary typed
RGBA8 entries remain supported, including rejected compressed images. There is
no dimension reduction or extra draw path.

`padded-texture-reader-test` passes six actual lookups/readbacks of three original
images (opaque, alpha, rejected-codec exact), mutated source rejection, strict
version handling and patch idempotence under ASan/UBSan. The copied reader also
compiles for ARM. Its 376,212-byte fixture is private and uninstalled. The v4 spool
producer, preparation recipe and complete application are **not integrated yet**;
the full conversion already underway remains on its original recipe.

## Versioned writer and shared application integration, 12 October

The subsequent `pica_padded_texture_writer_patch.py` integrates that codec into
the selected native producer. `FHTXSPL4` stores the original source, actual
compressed or exact native payload, visible-only quality metric and checksums.
The decoder rejects incompatible dimensions, payload sizes, hidden reduced
images and quality metrics that disagree with the selected representation.
Assembly publishes `FHTXPK4`; earlier formats retain their strict validation.
Compression failures above RMSE14 keep exact RGBA8. No source dimensions, UVs,
geometry, gameplay work or GPU upload ownership were changed. Melee's pinned
`texture_decode.h` CMPR 5:3 palette and `texture_repack.h` padded-edge semantics
remain the source reference; this is preparation-time encoding, not per-frame
conversion. Additional padded-source scratch remains bounded at 512 KiB and
is freed before returning; output/encoder storage is additional.

`install-texture-padded-pack-test` runs the actual producer and reader under
ASan/UBSan with original opaque 320x274, alpha 196x512 and rejected 64x48 images,
ordinary exact formats, and an existing compressed/reduced pair. All seven
entries match their independent pixel or checked original-image references.
Four image cancellations preserve the previous durable checkpoint; assembly
cancellation/resume, duplicate reuse, file/window/resident reads, 42 malformed
directories and the wrong format version are checked. The resulting private
pack is 437,764 bytes. Writer, codec and reader compile for ARM. This is bounded
integration evidence, not a complete game's resident texture budget.

Shared application `normal-setup-v5-builds` incorporates this path. Reproduce
with `build_install_normal_v1.py --label <new-label> --padded-textures`; the
builder restores the worktree and archive afterward. Relative to v4, only the
writer, reader and recipe changed, plus three padded-codec files. The texture
recipe is `a295a41bac8c0798b56ecf6744c3351b68ee70e702d84780e9ab1df33ff44f5c`;
original geometry, performance geometry and movie recipe identities are
unchanged. All 547 archived source identities, existing game archive, linked
511-instruction scene shader and Old80/New124 CIA profiles are verified.
Old-profile preset/menu/exit and New-profile real-ISO fingerprint/cancel/exit
passed with inspected captures and unchanged private saves. No game launch or
physical performance is claimed. The build is **not deployed**.

`prepare_native_iso_texture_bases.py --padded --seconds <limit>` now runs a
separate resumable whole-ISO v4 conversion. It copies only the changed reader
and writer; the previous v3 journal and outputs are retained unchanged.
`audit_native_texture_bases_spool.py --padded` checks durable source identities,
independent exact pixels, the previously verified compressed image bytes and
visible-only quality metrics. Whole conversion and gameplay handoff remain
required before this setup build is a complete installation path.

## Actual image-owner consumption checks

`test_padded_image_owner.py` extracts the exact image acquire/release/sampler
bodies and reader from the verified v6 source archive. It links the actual
texture conversion code and uses mocks only for SDK texture allocations,
cache publication and platform counters. The private v4 fixture passes 480
load/release cycles and 480 duplicate acquires, comparing every uploaded byte
with the checked opaque, alpha, exact-rejected and ordinary/reduced payloads.
It checks per-source UV extents and wraps, the 4 MiB image ceiling, the 15%
Old/New linear reserve boundary, allocation failure and corrupt-read cleanup.
Sixteen failure cases are rejected and all 488 allocated mock textures freed.
Successful loads publish once; repeated references add no allocation or flush.
No fallback conversion or CPU staging allocation occurs for these prepared
images. The 131,072-byte peak is one fixture image, not a scene live-set peak.

An initial harness failure came from continuing after intentionally corrupting
a pack: the real reader correctly closes a failed pack, so the harness now
reopens the repaired private file before the next independent case. The failure
log is retained. No production change was needed. This is ASan/UBSan host
coverage of the actual owner code, not PICA execution, console allocation or
whole-game texture residency evidence. No further application build or repeated
menu boot was performed for this test-only addition.

The separate real v4 ISO conversion now has 678 verified unique sources
(447 exact, 231 compressed, 72 reduced variants and 42 padded entries), with
12,494,528 durable spool bytes. Audit checkpoint2083 checks the entire durable
prefix, including original-source identity, exact pixel output, compressed
reference bytes and padded quality metrics. This remains a partial collection.

## Whole-source coverage and pack budget

`inventory_native_texture_bases.py` checks all 19,264 original records in 107
banks against the recorded ISO identities. The 20,208 colour/opacity planes
contain 2,454 unique base images. There are 1,345 CMPR sources, of which 210 need
padded dimensions. The checked quality outcomes admit 407 additional reduced
variants: the final expected directory is 2,861 entries / 125,884 bytes, within
the runtime owner's 128 KiB directory limit. The expected pack is 72,038,604
bytes, within its 128 MiB file limit. Native payloads total 33,011,648 bytes
across the collection; none of these collection sizes is a live RAM budget.

The older aligned-CMPR comparison pack covers 1,130 of 1,135 aligned source
images. An initial inventory assertion incorrectly assumed complete coverage;
that failure is not evidence of five missing gameplay textures, because the
earlier renderer could convert unsupported sources at load time.
`check_missing_aligned_textures.py` checks those five directly: the actual
native codec matches the independent tex3ds output byte for byte, with matching
decoded alpha and visible squared error. All five fail the unchanged RMSE14
quality gate and must retain exact RGBA8. One also has a rejected reduced
variant. The inspected large texture loses visible edge detail under compression,
consistent with rejecting it. No quality limit was relaxed.

The inventory's earlier upper bound of 2,883 entries is superseded by 2,861;
large dimensions alone do not guarantee an admitted reduced variant.
`audit_native_texture_bases_spool.py --padded` now requires every original
source identity at spool completion and every expected full/reduced variant in
the assembled pack, including source/payload CRCs, exact payload hashes, sorted
directory, contiguous offsets, final size and the independent size prediction.
The handoff fixture builder requires this completed audit before publishing a
test bundle. Partial checkpoints never authorize activation.

These are host conversion/reference checks. Actual startup-to-game consumption,
bounded live residency, ARM conversion duration and console performance remain
separate acceptance requirements. The native converter, renderer and installed
applications were not changed by this inventory work.

## Full conversion and loader acceptance, 12 October

The v4 conversion is now complete. Audit checkpoint23077 verifies all 2,454
unique sources: 1,139 exact representations, 1,315 compressed representations,
407 reduced variants and 210 padded sources. The 57,754,944-byte spool and
72,038,604-byte pack exactly match the independent inventory prediction.
Pack SHA256 is `6153dd882cf72ea13343b1756715794cac88feb1004ea02dbb1f1291147c3278`.
All original base planes and every admitted variant are present. This does not
add mip-chain rendering or establish that every live generated texture is covered.

`test_padded_image_owner.py --full` passes 9,816 load/release cycles plus 9,816
duplicate acquires over both original/reduced modes and both hardware allocation
profiles, using the actual archived loader and reader. All uploaded bytes and
sampler extents match; 16 capacity/allocation/corruption cases reject correctly.
There are no fallback conversions or leaked allocations in this isolated check.
Its 131,072-byte peak is one loaded texture with mocked SDK allocations, not a
scene's GPU residency. No repeat small-fixture run was needed.

The subsequent real v7 startup/game checks load 369 images with zero errors on
both profiles, reaching the movie and cockpit title. Each still records 26 CPU
conversions, which remain to be attributed; do not claim all live conversion
has disappeared. Actual scene texture residency peaks at 1,522,848 bytes.
See `artifacts/research/install-game-handoff/result.json` for the live budgets
and `INSTALLATION_BUNDLE.md` for the shader mismatch caught and repaired during
the handoff. No asset or application deployment has occurred.
