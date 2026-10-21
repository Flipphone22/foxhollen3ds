# Native startup mesh reduction

Local ISO-to-reduction-and-paired-packs pipeline, **not yet a complete game
installation or console startup feature**.
They run during one-time preparation before gameplay. Simulation, collision,
animation and installed normal/replay applications remain unchanged.

Meshoptimizer is pinned to v1.0 commit
`73583c335e541c139821d0de2bf5f12960a04941`, under
`reference/meshoptimizer/LICENSE.md`. Only its simplifier and a bounded native
allocator are linked in the candidate. Desktop frontend vectors are not linked.
Pinned Melee startup allocation/error cleanup and indexed I/O were consulted in
`native-3ds/port/3ds/main.c` and `file_io.c`. The adaptation keeps the single
ISO/FST reader and bounded private startup scratch; no extraction directory,
application worker or GPU queue is introduced.

| Component | Behavior | Evidence |
|---|---|---|
| `install_surface.c` | Original bind-space positions, both UVs, RGBA and CPU-stream coefficients per material context | All2,110 unique sources,13,559 draws and4,174,335 corners exactly match the independent host decoder |
| `install_fidelity.c` | Bidirectional seven-sample shape/UV/colour/winding checks, exact cyclic faces and BVH nearest-surface queries |206 comparisons including32 real reductions, seam/missing-surface cases and eight allocation failures |
| `install_lod_guards.c` | Morphs, actual position/normal CPU blends, matrix/texgen boundaries, alpha gradients, coincident deformation seams | All13,559 draws match source locks:163,169 locked packet vertices and54,075 required face occurrences |
| `install_simplify.cpp` | Pinned attribute-aware simplifier in a private bounded arena; source attributes unchanged |66 inputs including48 real material inputs match host indices/error; exhaustion cleanup and subsequent reuse pass |
| `install_list_pack.c` | Ordered oriented immutable packets encoded as consecutive strips or ordinary triangles, whichever is smaller |594 cases:546 representable outputs match host bytes and native primitive reader;48 oversized outputs reject |
| `install_reduce.c` | Per-connected-feature simplification, protected-face admission and all-context fidelity |40 sources/85 lists, five admitted reductions; independent host acceptance and native scene compilation |
| `install_model_reduce.c` | Whole-model `FHMLD01` patch; private source validated by the actual runtime patcher |Eight actor/world sources, allocation failures, cancellation, bad CRC, source immutability |
| `install_performance_pack.c` | Checkpointed paired topology/geometry spool, then existing runtime pack formats |Both pack resumes, duplicate reuse, discarded uncommitted tail and actual patched-source lookup |
| `install_performance_prepare.c` | ISO scanner through reduction and both pack assemblers |Four cancellation resumes, explicit allocation accounting, unchanged empty topology and unsupported-record rejection |

All components pass ASan/UBSan host checks and ARM compilation. The simplifier
also passes a complete link against the actual SDK with ARM11/VFP hard ABI,
soft TLS and startup-only C++ exceptions. The link-only ELF was **not executed,
packaged as a probe, deployed or used as emulator performance evidence**.
Exceptions stay inside the C adapter; arena exhaustion exposes no partial
candidate. Linking uses `libsupc++`, `libpthread`, libctru and ordinary3dsx SDK
specifications. No worker is created. Execution of the eventual full startup
owner, including SDK exception/TLS behavior, remains to be checked.

The fidelity limits retain actor bind-space distance0.5, world distance1.0,
UV1/128, RGBA1/32 and geometric normals within20 degrees. Seven finite samples
are checked in both directions. This is **not a continuous error proof, animated
image comparison or screen-pixel guarantee**. One nearest triangle must satisfy
all attributes and winding. Unchanged cyclic faces include every active
attribute in their identity; reversed winding never uses that shortcut.

The deformation guard distinguishes absent and explicitly empty CPU blends,
preserves non-unit coefficients, and checks normal bindings independently.
Vertex locks alone cannot protect a rest-pose degenerate that becomes visible
during animation. A separate post-simplification multiset check preserves
required faces' cyclic orientation **and multiplicity**. Every material context
sharing a changed source list must pass before publication; one palette is not
enough. The final per-list admission calls are now connected. Shared visible or
shadow list storage stays unchanged, because an in-place edit would alter two
owners. Lists with differing layouts also remain unchanged.

The list writer allocates no heap. It measures before writing, keeps source
corner bytes, and rejects output beyond the caller's original capacity or the
16-bit display-list limit. It never welds vertices or reorders triangles. Its
first test incorrectly expected oversized synthetic lists to pass the existing
primitive reader. The final implementation explicitly rejects those encodings,
and tests verify that rejection leaves the output untouched.

Installation memory bounds and measurements:

- Surfaces use88 bytes/corner, with a32,768-triangle cap. The largest actual
  source surface is1,688,016 bytes. A comparison may need two surfaces; release
  them before compiling the final resident packet.
- BVH/hash/index scratch stays below1 MiB at the cap. The comparison fixtures
  peaked at29,136 bytes, excluding borrowed surface arrays.
- Morph and sparse blend maps use bounded original16-bit array counts. Per-list
  reference/seam arrays have a32,768-vertex cap.
- The simplifier arena has an8 MiB ceiling and caller-selected capacity. Tested
  inputs used at most234,576 bytes; reserving8 MiB still costs8 MiB. Input arrays,
  output indices and SDK internal allocations are separate costs.
- The link-only ELF has115,432 text/7,544 data/14,504 BSS bytes, including its SDK
  scaffold. This is not incremental full-app size. Whole-installer RAM/SD budgets
  and physical conversion time remain unmeasured.

The kernel's44,598 ->20,244 candidate indices do **not** establish admitted asset
reduction or game FPS. The integrated per-list sample instead admits five of85
lists:15,139 ->14,895 triangles. The whole-model sample processes46 lists from
six actors/two worlds:7,357 ->7,113 triangles. These are small, overlapping test
samples, not whole-collection savings or an adequate-performance claim.

Connected features use source matrix packets and actual CPU position/normal
blend signatures when welding coincident positions. Original packet bytes stay
immutable. Each feature gets independent normalized simplification; small
features stay original. Actor normal/colour/UV weighting follows the visual
metric of the host recipe, with UV density weighting. Float metric construction
is not claimed byte-identical to the host's double-first scaling. Every accepted
surface is independently admitted afterward. World metrics retain the source
recipe's unscaled weights.

`FHRDSPL1` stores original SHA256/CRC identity, the admitted topology patch and
the compiled geometry together. Geometry is keyed to the **patched** source CRC;
the topology pack is keyed to the original. A completed source never needs
reducing again after interruption. CRC ambiguity fails explicitly. A source
checkpoint is appended only after both outputs validate. Assembly then writes
`FHMLPK1` (32 MiB cap) and `FHNSSET1` (64 MiB cap) separately; the spool has a
160 MiB cap,4,096 records and96-byte record headers. Source identity/directory
storage is bounded. None of these operations activates the whole game.

The integrated fixture uses8 original actors/world sources, cancels inside a
model, after a completed source, during topology assembly and during geometry
assembly. All four resumes pass, as do empty topology, unchanged source and
unsupported-record failure. Explicit peak heap is9,158,590 bytes, including the
reserved8 MiB arena. This excludes library internals, stack and the full-disc
FST, and is not a full-collection or console maximum. Shared pack tests retain
private outputs11,368 bytes topology/335,344 bytes geometry. Actual runtime
patch application and native scene lookup both pass for every fixture source.

Full-disc preparation now completes and passes an independent pack audit: all
2,110 original source identities, including8 non-rendering sources.79 models/
80 lists change; unique-list triangle totals are1,383,658 ->1,381,976, a reduction
of only1,682 (about0.12%). The resulting native geometry pack is50,926,092 bytes,
only45,420 bytes smaller than the original pack. **This is not a useful fast
asset recipe and must not replace deployed performance assets.** These counts
are not rendered frame workloads or an FPS estimate. In the earlier85-list
sample,39 candidates had no gain and41 failed the surface guard. Refinement must
preserve visual/fidelity requirements, not remove those checks to improve counts.

Full topology:78,464 bytes/SHA256
`544de97cc3fdccd751b11489109964c024622f283f2a09fadc4c9db38bc912c6`.
Full geometry:2,102 entries/SHA256
`40b4e6863b99810e5f4563b5c5fdfcbc4e0905e26ef3f0172333a5d466e26bb0`.
`install-performance-full-build/audit.json` checks independent original inventory,
all record/payload CRCs, sorted unique runtime identities, topology contents,
patched-source geometry keys and exact spool/pack payload identities. Actual
native packet readers also validate during assembly. This is not a whole-game
visual/animation acceptance check.

The first90-second conversion was interrupted during geometry assembly; its
private output/report remains retained. The driver's recipe initially omitted
some transitive local headers. Recursive include fingerprinting fixed that;
a fresh recipe preserves the previous stage rather than resuming incompatible
provenance. The correct final run took112.8 host seconds and exited normally,
not a console time prediction. `prepare_native_iso_performance.py` now rejects
concurrent converters and changed pending recipes; explicit `--new-recipe`
archives the old report/stage. No full-game generation was activated.

Remaining: effective fidelity-preserving reduction, complete installer memory/SD
budgeting, final recipe/manifest integration and console startup execution.
Original/performance choices must retain their semantics. No gameplay assets
have been replaced and no speedup is claimed.

Evidence: `artifacts/research/install-{surface,fidelity,lod-guards,simplify,
list-pack}-test/`. `tools/check_install_simplifier_sdk.py` allows SDK-only checks
without repeating source audits. Early SDK checks incorrectly rejected weak
exception symbols/pthread names in a partial link; the final complete SDK link
resolves required platform dependencies without dummy OS implementations or
suppressed final linker errors. The initial C/C++ compile issues (indentation
warning and allocator name lookup) were corrected before passing checks.


## 13 October native refitting experiment

The actual ARM performance-stage result retains 1,376,568 of 1,383,658 source
triangles (about 0.51% removed across unique source records, not frame-weighted).
That is insufficient as the main route to the frame-rate target.

`tools/prototype_native_world_refit.py` tests a different preparation route:
meshoptimizer's position/attribute-updating simplifier over native world data,
with the existing bounded startup allocator. It rounds results back to the
actual 20-byte integer vertex format before the unchanged bidirectional
position/UV/RGBA/normal guard. It preserves borders and locks opacity gradients.
The final variant feeds failing source/candidate faces back into local vertex
locks, with at most 32 retries, instead of weakening the acceptance limits.

Sixty main-world groups from ten blocks in five gameplay areas contain 20,451
triangles. Two selected shipbattle blocks had no groups in the bounded sample;
this does not cover the opening ship. Ordinary refitting admits 20,449; preserving
attribute seams admits 20,433. Local constraint refinement admits 20,345 with
vertex refitting, versus 20,327 with fixed vertices. The best reduction is only
0.61%. UV/colour and position checks reject most larger candidates. No candidate
was integrated, no pack was replaced, and none establishes an FPS improvement.

Reports and generated C++ sources/libraries are retained under
`native-world-refit-v1`, `native-world-refit-seams-v2`, and
`native-world-refit-local-v3`. Native processing took about 1.5?15.6 host seconds;
these are not console conversion measurements. The simplifier arena peaked at
471,408 bytes while reserving 8 MiB; Python inputs and fidelity scratch are extra.
Sanitizer coverage of this new update entry and hardware execution remain open.
Runtime adoption would also require handling mutable world positions, ordering,
source ownership and image comparisons. This experiment does not justify those
implementation costs yet.

An independent exact-sharing audit over 835 actors/319,752 vertices found 7,968
identical current vertex values but zero duplicates after retaining all source
references. Merging those values alone could break later attribute updates.
`tools/audit_actor_exact_sharing.py` and `actor-exact-sharing-audit.json` preserve
the result. Broader reductions need a different rendering/LOD approach; these
experiments do not close the performance gap.
