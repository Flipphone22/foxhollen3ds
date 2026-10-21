# Committed installation bundle

`install_bundle.c` supplies the final publication/readiness layer for native
on-device preparation. `install_task.c` now connects the real four-stage
conversion sequence to the startup controller's task/readiness interfaces.
`install_boot.c` now connects menu readiness to immutable runtime asset bindings.
An isolated normal-entry patch and `--on-device-setup` build option integrate
these modules. The shared normal app has now been built through
`normal-setup-v6-builds` (3dsx `c7fd38d4ef18`, CIA `d7508c0627eb`), with the
Old 80 MiB/New 124 MiB profiles. Startup-menu and cancellation checks ran on v5;
v6 reuses that evidence only for the unchanged menu, after a bundle-header fix.
The complete prepared installation and startup-to-game handoff remain untested.
Nothing from this setup work has been deployed.

Four independent stage transactions avoid overloading the eight-artifact limit
of an individual conversion journal. Their generated directory names and artifact
IDs are fixed, not taken from disc filenames or user-supplied relative paths.

| Stage directory | Required stage artifacts | Game files referenced by bundle |
|---|---|---|
| `original` | 0 spool, 1 geometry, 2 cache, 3 empty topology | 3, 1, 2 |
| `performance` | 0 spool, 1 topology, 2 geometry, 3 cache | 1, 2, 3 |
| `textures` | 0 spool, 1 typed pack | 1 |
| `movie` | 0 spool, 1 prepared stream | 1 |

The task owns one read-only disc handle and validates its size, FST checksum and
full SHA256 against a caller-supplied expected identity before opening any
conversion transaction. The full hash runs once per Prepare/resume invocation,
never in the readiness callback. Normal readiness checks the disc header/FST and
size plus the bundle metadata/child sizes. It cannot detect same-size disc data
changes outside the FST; actual preparation performs the full verification.

Converters run sequentially, retaining their existing checkpoints. Original
geometry also gets its native cache and exact empty topology; performance
geometry gets the paired reduced topology and matching cache. Texture preparation
uses all typed base planes, including opacity. Movie preparation uses scale 2.
Progress names distinguish scanning, reduction, packing, cache generation and
movie work. Bars count the current phase's actual units, not estimated remaining
time. Cancellation occurs at converter callbacks; filesystem calls and individual
bounded conversion units are not preempted. Resume journal checks can also read
completed data before the next callback.

All source handles, scratch and transactions close before returning to the
startup menu. Source storage closes before final bundle verification. No worker,
GPU target, linear allocation, gameplay/save hook or runtime render work is added.
Re-entrant preparation is rejected. A task success requires all four complete
stages, successful whole-bundle publication and a fresh bundle-open check.

`artifacts/research/install-task-test/result.json` records eleven cancellation
points using the actual converters under ASan/UBSan. The 44,015-byte private ISO
fixture contains unchanged actor/world and RGBA8 texture records. Original and
performance geometry/cache/topology and texture output complete; its deliberately
missing movie is rejected, and no whole installation activates. Repeating the
failed preparation retains the completed stages. Hash/FST mismatches, re-entry,
missing-disc readiness and file-handle cleanup pass. This is not a complete-game
installation test. Tracked explicit host allocations peak at 9,069,512 bytes and
return to zero after each cancellation/exit; library internals, movie encoding,
larger full-disc inputs and runtime stack are outside that measurement.

`artifacts/research/install-task-sdk-link/result.json` records a complete ARM SDK
link with all four converter symbols retained. The link-only ELF has 705,636
bytes text, 7,552 data and 47,560 BSS, including SDK/codec/C++ scaffolding; those
are not incremental game costs. The task's own GCC static frame is 1,896 bytes
(Clang 1,936), excluding callees. Stack reports distinguish compiled functions
from symbols actually retained by the linker and do not establish maximum call
depth. The bounded simplifier retains C++ exceptions; only the JPEG decoder's
private SDK structs use short enums. An ARM-only callback type mismatch found
during this check was fixed and the host and SDK checks rerun.

Remaining integration includes completion of all ISO conversion stages,
space/memory checks and actual startup/display/game handoff. Native reduction
currently saves too few triangles to qualify as the desired fast preset. See
[the geometry performance audit](INSTALL_GEOMETRY_PERFORMANCE.md) for the full
collection counts and rejected alternatives. Texture residency costs also remain
open. Building the application does not establish release readiness.

`install_recipe.py` now generates the four stage keys from their converter source
closures, recursive local includes, relevant options and SDK codec archives.
Renderer source and application build ID are excluded. Ten isolated mutation
checks verify the appropriate stage invalidations and renderer independence.
Checkout location does not change the keys. The original ISO was reverified:
1,459,978,240 bytes, FST CRC 1,578,060,896 and the expected pinned SHA256. These
keys differ from research-driver keys; existing partial journals remain untouched
and must not be relabelled without verification.

`install_bindings.c` copies eight asset paths plus the ISO path into 2,880 bytes
of static storage and uses two state flags. Failed selection retains the previous
valid selection. Successful menu Start locks bindings for the process; getters
allocate nothing and perform no file reads. Actual copied geometry-selection
functions pass ASan/UBSan checks for both modes, original-file validation and
private replay isolation. The copied DVD reader compiles for ARM and selects the
bound movie/disc before environment/default paths. Playback remains unvalidated.

`install_boot.c` loads graphics preferences once, supplies the real task and
binds during final readiness. Four synthetic dependency cases cover menu exit,
failed readiness, failed binding and successful handoff. Diagnostics may be
supplied by the caller; the initial normal-entry patch currently registers none.

`build_native.py --on-device-setup` generates the recipe/header/source ID and
requests `console-setup` support. `build_install_codecs.py` supplies the five C++
units; linking includes SDK LZ4/crypto and C++ exception support. The normal entry
patch is explicitly required; replay/test-only combinations are rejected. This
remains one shared Old/New build. Existing CIA memory profiles are unchanged.
Apply `install_runtime_patch.py --startup` and the typed-texture patch to a checked
snapshot copy; do not blindly compile unrelated graphics WIP.

The combined SDK setup link has 734,348 bytes text, 7,640 data and 51,208 BSS,
including SDK/runtime/codec scaffolding; these are not incremental game costs.
It is link-only. Evidence: `install-recipe-test`, `install-bindings-test` and
`install-boot-test` under `artifacts/research`. The latter also compiles the actual
copied normal entry with setup enabled. No full game ELF or framebuffer handoff
was executed, and installed applications/saves remain unchanged.

The empty original-topology file must be exactly the existing 16-byte FHMLPK1
empty set. Performance topology and geometry must come from the paired native
conversion stage. A completed stage alone never makes a game installation ready.

Publication requires all four complete stage descriptors with matching source
and recipe identities. It verifies all eight runtime files' sizes, bounded
format headers and whole-file CRCs. Cache headers must identify the matching
geometry size/CRC. Their actual deep format validation remains the responsibility
of the converter at artifact completion; bundle checks do not replace it.
Cancellation is observed between 16 KiB verification chunks and file boundaries.

Only after those checks does the owner write a 500-byte FHBNDL01 manifest in a
separate `bundle` transaction and commit it through the existing two-slot journal.
The manifest references immutable stage generation numbers. Updating another
stage's current state cannot silently switch part of an active game installation.
Previous generations are retained; this module contains no deletion policy.

The bundle compatibility key is SHA256 over its format identifier and the four
stage recipe identities. It is independent of the application build label.
Callers must provide actual converter compatibility identities, not substitute a
changing renderer build number. Source SHA256, disc size and FST CRC are checked
against the supplied spec. Full source hashing still belongs to preparation/resume;
the application must supply the validated source spec during ordinary startup.

`fh_install_bundle_open` reads both compact committed candidates and selects the
newest whose manifest and referenced file sizes are usable. A torn commit,
damaged manifest or missing child can select the preceding usable candidate.
It does not scan generation directories or rehash every pack during ordinary
startup. Same-size child corruption is not discovered by this fast check;
full checks occur at publication and the existing runtime asset readers retain
their own validation. Do not describe fast readiness as a fresh full-content audit.

The pinned Melee `native-3ds/port/3ds/file_io.c` was consulted for the FAT directory
cost and indexed-file-access rationale. This owner uses fixed small generated
directories and a bounded manifest instead of per-frame discovery. It adds no
worker, GPU/linear/VRAM allocation, cache-coherence change or rendering path.

## Evidence and limits

`artifacts/research/install-bundle-test/result.json` records ASan/UBSan and ARM
compilation. The fixture uses independently validated real geometry, topology,
textures, cache encodings and the complete prepared movie, totaling 38,031,929
bytes. These fixtures have differing source coverage: the test is **not a complete
game asset installation** and its temporary generated directories are removed.

Tests cover incomplete stages, mismatched recipes/sources, cancelled publication,
same-size corruption before commit, a missing newer texture pack, damaged newer
manifest and torn commit, plus mismatched source/FST/recipe on open. Input fixtures
remain unchanged. With both candidates valid, normal open performs four `fread`
calls totaling 1,384 bytes, plus referenced-file size checks. This measures API
reads, not physical FS IPC time or stdio internals. An initial host compile needed
the SDK mbedTLS include path; that test configuration error was corrected.

ARM compiler static stack reports 18,608 bytes for publication (including its
inlined verification buffer) and 2,504 bytes for open, excluding deeper library
and filesystem calls. These are compiler estimates, not a measured whole-installer
stack peak. This module owns no explicit heap allocation; stdio/crypto internals
and the existing transaction owner remain part of the final memory budget.

The real four-stage task, native compatibility keys, source-fingerprint owner,
original empty topology and caches, and runtime asset/movie bindings now exist.
The full shared setup build has passed isolated menu and source-cancellation
checks. Remaining work includes complete native conversion, temporary SD/RAM
budgets, effective safe performance assets, runtime typed-texture acceptance,
and captured menu-to-game/gameplay checks. Normal/replay shared-build and
Old 80 MiB/New 124 MiB requirements remain unchanged.

## Padded texture publication repair, 12 October

The first padded-texture application (`normal-setup-v5-builds`) could prepare
FHTXPK4 but its final bundle check still required FHTXPK3. The startup-only menu
checks could not expose this, and no complete installation was claimed. The
bundle owner now accepts exactly those two typed texture headers. Unknown
versions still fail even if the caller supplies the matching whole-file CRC.
There is no new file scan, allocation, worker or gameplay path.

Both `install-bundle-test` (v3) and `install-bundle-padded-test` (actual v4 output)
pass ASan/UBSan, ARM compilation, cancellation and prior-generation recovery.
The v4 fixture includes the real writer's opaque/alpha/quality-rejected images.
These mixed-coverage fixtures are not full-game installations. Normal readiness
still reads 1,384 metadata bytes in four fread calls; physical FS costs are not
inferred from these counts. Compiler stack estimates remain 18,608 bytes for
publication and 2,504 for open, excluding deeper calls.

`test_install_task.py --padded` additionally runs real original geometry,
performance geometry and v4 texture converters from a small private ISO under
the actual startup task. Eleven cancellations resume correctly, explicit
tracked allocations are released, and the deliberately absent movie prevents
activation. The explicit host allocation peak is 9,069,512 bytes; this excludes
library allocations, stack, largest full-game sources and movie encoding and
does not prove whole-installer RAM safety. An initial harness link omitted the
padded codec unit; the failure log is retained, and the corrected test passes.

`normal-setup-v6-builds` contains the publication fix. Its 547 source identities,
unchanged stage recipes, game archive, linked shader and Old80/New124 profiles
are verified. Only `install_bundle.c` differs from v5, so v5's inspected menu
and cancellation evidence was initially reused explicitly; v6 was not booted
as part of that publication-only check and has not been deployed.
The new verifier permits this reuse only for that exact source difference and
passing current publication tests. Complete conversion and menu-to-game handoff
remain outstanding.

The later `setup-additive-menu-v2-new` check does boot exact v6 on the New
profile: menu/preset persistence, inspected captures, isolated save preservation
and owned exit pass. The optional read-only HID counter sum supports a startup
menu clock followed by the existing gameplay clock; it does not write either
clock or change simulation. In this menu-only run the gameplay counter stayed
zero, so the result is not a complete handoff test. The first attempt failed
before launch on an ambiguous ELF symbol; the resolver now identifies the
gameplay counter by its owning source file.

`prepare_install_game_fixture.py` requires complete audited native outputs,
including every original texture source and admitted variant, before creating
an explicitly synthetic integration bundle through the actual archived v6
publisher. It does not seed or relabel conversion journals. Such a fixture can
test runtime consumption and menu-to-game ownership, but cannot prove ARM
conversion, cancellation/resume of a whole installation, or console speed.

## Completed runtime handoff and shader-build repair

The complete host-produced bundle now passes the actual v6 publisher/open code
under ASan/UBSan. It remains an explicit synthetic integration fixture; its
research conversion journals were not relabelled as application journals.

The first real-game handoff (`setup-game-v1-new`) failed at the receiver shader
guard. Source isolation had restored the stable three-entry receiver owner, but
the retained installer build script generated an older single-entry shader.
The 511-instruction scene shader check alone could not catch this mismatch.
The old receiver binary has 43 instructions/one entry; the correct preserved
program has 73 instructions/three entries. The runtime guard was correct and
was not weakened.

`build_install_normal_v1.py` now isolates the stable shader-generation block
alongside the stable renderer while retaining the installer compiler additions.
It compares every embedded DVLB against the preserved baseline before accepting
the build. Local generator imports are included transitively in source hashes
and the source archive, including the previously omitted shadow-entry generator.
`verify_install_shader_handoff.py` rejects the actual broken v6 ELF and accepts
v7, checks all 556 archived sources, unchanged native sources/asset recipes,
restored worktree, and shared Old80/New124 CIA profiles. The scene shader and
game archive are unchanged. No GPU algorithm or ownership rule was altered.

Local v7 is source `80897ecc9088`, 3DSX `df1e4709da32`, CIA `950e8c50e091`.
It is **not deployed**. A second test-profile failure omitted the private DSP
firmware; the runner now supplies the same firmware as the established console
runner, retaining real audio. Both failure records remain. Their identical
derived asset copies were retired only after byte comparison with the retained
canonical fixture; reference manifests retain every path, size and SHA256.

`setup-game-v3-new` (3DSX) and `setup-game-v3-old` (80 MiB CIA content) pass:
startup readiness, menu LCD retirement, prepared movie, cockpit title rendering,
Select shutdown, drained logs, owned process cleanup and isolated save/source
preservation. New movie and both cockpit captures were inspected. This is not
opening gameplay progression or physical-console validation.

| Observed resource | New profile | Old profile |
|---|---:|---:|
| Normal heap peak / reserve | 53,699,384 / 71,303,168 B | 38,822,160 / 50,331,648 B |
| Minimum free linear / reserve | 18,982,912 / 25,165,824 B | 4,302,848 / 10,485,760 B |
| Minimum free VRAM after initialization | 4,723,712 B | 4,723,712 B |
| Measured stack upper bound / reserve | 72,936 / 524,288 B | 72,936 / 524,288 B |
| Process commit peak / limit | 103,104,512 / 130,023,424 B | 67,960,832 / 83,886,080 B |

The original summaries included a zero VRAM sample before renderer initialization.
`summarize_install_handoff.py` corrects the analysis from the existing logs and
preserves those summaries; no rerun was needed. These are sampled budgets for
this route, not guarantees for every area or full installer scratch usage.
Both runs load 369 textures without errors, peak at 1,522,848 texture bytes and
release their tracked texture/CPU storage. The 26 remaining conversions per run
need attribution. Host elapsed times (45.85/35.40 seconds) are not a New/Old FPS
comparison. Effective safe fast geometry, whole on-console preparation and the
broader rendering/progression/performance goal remain incomplete.
