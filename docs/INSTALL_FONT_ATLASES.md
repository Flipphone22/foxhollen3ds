# Native preparation of game-text atlases

The TEX-bank inventory did not include `gametext/*/*.bin`. A bounded load-time
capture found three actual cockpit/title misses belonging to those files:
two I4 font pages and an RGB5A3 menu-icon atlas. Their source bytes match original
files exactly. None of the 26 captured misses belonged to the TEX-bank inventory.
The other captures look like generated noise, masks and zero placeholders; their
dimensions and source producers support that classification, but this is not an
exhaustive byte-level attribution of every procedural owner.

`install_font.c` reads the original `gameTextFinalizeLoad` file structure, without
relocating or mutating the input. It validates counts, offsets, dimensions, format,
tile coverage and the terminating image header. It permits at most the original
three texture slots and caps a source file at 128 KiB. The largest actual file is
78,308 bytes. Runtime random state and all source texture producers are unchanged.

The existing texture preparation stage now appends every language/charset atlas
after the TEX banks. A tagged FST-entry/image-boundary cursor distinguishes the
font phase. A cancelled image is retried; durable images are preserved and exact
duplicates are reused by the existing spool. The conversion recipe includes the
new implementation and header. Earlier completed outputs and their journals are
preserved; the full-conversion driver uses new output directories instead of
relabeling them as the new recipe.

This is load/preparation work. No renderer, shader, skinning, GPU command stream,
simulation, audio or save behavior was replaced. The pinned Melee write-site
publication rule remains intact: the existing native image owner allocates,
copies once and flushes before first GPU consumption. The adaptation is typed
font-source lookup in the existing prepared-image directory, not a new GPU owner.

## Evidence

- `install-texture-miss-analysis/result.json`: all 26 bounded snapshots validated
  by size/FNV/source hashes; no dropped captures. The contact sheet was inspected.
- `install-font-test/result.json`: all 1,020 original files and 1,395 atlas
  occurrences agree with an independent source parser; 893 unique images.
  Includes malformed input, immutable source, ten interrupted/resumed traversals,
  three invalid cursors, ASan/UBSan and ARM compilation.
- `install-font-pack-test/result.json`: actual journal/spool/exact conversion/
  publication checks, including a real cancellation and reopen. Every output
  pixel matches the independent scalar decoder. The three captured file-backed
  misses are covered. This test does not activate an application bundle.
- `install-texture-all-prepare-fonts-test/result.json`: the actual combined
  texture-stage entry point crosses TEX banks into a real two-atlas game-text
  file; colour/opacity and pack-assembly cancellation/resume still pass.
- `prepared-fonts-image-owner-test/result.json`: the built image-owner bodies
  consume a combined fixture of 3,347 unique sources and 3,754 variants. Across
  both memory profiles and texture modes, 13,388 load/release cycles plus 13,388
  duplicate acquires pass with zero fallback conversions or leaks. SDK allocation
  and publication are mocked here; this is not GPU or scene-memory evidence.

The font-only pack is 12,738,284 bytes, with 7,258,112 bytes of native pixels across
the entire collection. The combined consumer fixture is 84,776,872 bytes. Its
directory is 165,176 bytes, versus 125,884 previously. The reader's allowance is
now bounded by the existing 4,096-entry format limit (180,224 bytes); actual
allocation is proportional to count. No whole font collection is made resident.

`install-texture-fonts-consumer-fixture` joins already verified native TEX and font
payloads byte-for-byte. It is explicitly a synthetic consumer fixture, not a
claim that the application completed the entire ISO conversion or resumed its
own journals. The combined publisher/open check uses the actual app owner under
ASan/UBSan. Full on-console conversion and its time/RAM budget remain unverified.

## Shared application check

Local normal build `17d50ce53c4c` is archived in
`artifacts/research/normal-setup-fonts-v1-builds`. One `.3dsx` and one CIA retain
Old 80 MiB/New 124 MiB profiles. All embedded renderer shaders and the game
archive match the preceding stable renderer. This build has no diagnostic
capture buffer and is **not deployed**.

`artifacts/native/setup-fonts-game-v1-new` passed the isolated, silent, headless
startup-menu -> prepared movie -> cockpit menu -> Select sequence. All three
captures were inspected; user saves, source disc and fixture were preserved;
logs drained and the owned process exited. It took 46.69 host seconds, which is
not a comparative performance measurement or physical-console evidence.

| Observation | Earlier setup v7 | Font build |
|---|---:|---:|
| Actual texture loads | 369 | 369 |
| Load-time conversions | 26 | 23 |
| Texture load errors | 0 | 0 |
| Measured heap use | 53,699,384 B | 53,738,680 B |
| Minimum linear free | 18,982,912 B | 18,982,912 B |
| Active VRAM free | 4,723,712 B | 4,723,712 B |
| Texture CPU staging peak | 107,992 B | 42,456 B |
| Texture GPU payload peak | 1,522,848 B | 1,522,848 B |
| Process allocation | 103,104,512 B | 103,104,512 B |
| Prepared texture bytes read | 2,595,628 B | 2,775,304 B |

The extra directory costs 39,296 measured heap bytes including alignment. The
larger prepared input also adds 179,676 bytes read in this run; removing conversion
does not establish a loading-speed improvement. Main stack measured 72,936 of
524,288 reserved bytes. No FPS gain, physical timing, whole-game visual parity,
opening progression or complete installation is claimed by these checks.
