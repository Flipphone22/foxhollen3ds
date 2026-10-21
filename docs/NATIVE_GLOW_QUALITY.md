# Load-time distortion mesh quality

The selective quality policy raises two audited particle displacement fields
from 128 to 256 triangles. Other fields retain 128. This is not a blanket
particle-density increase or a replacement for full visual comparison.

The prior 128-triangle meshes worsened three of 180 virtual-particle image
comparisons relative to omitting displacement. Selective 256-triangle meshes
improve all 180 cases in that fixture. It uses one original-renderer image and
virtual placements; it does not establish full-scene or physical GPU parity.

`prepare_glow_quality_keys.py` derives two SHA256 payload fingerprints plus
dimensions and wrap modes from private assets. The compiled table contains no
pixels or converted geometry. Source texture IDs and runtime serial numbers
are not stable identities and are not used for matching.

The page loader validates the source extent and hashes eligible dimensions
once, before allocating the compilation scratch. Invalid inputs and hash
failures reject the load. Unknown valid fields retain the established default.
All four UV orientations use the selected budget. Draw-time lookup, immutable
packed vertices, shaders and simulation are unchanged.

The existing 64 KiB resident cap and 15% linear-headroom check remain. Packing,
cache flush before publication, and retirement before release are unchanged;
the pinned Melee `native-3ds/port/3ds/command_cache.c` explicit publication
rules were consulted. The five-field inventory is 52,320 packed bytes. The
quality table adds 96 static CPU bytes. Four maximum-sized staging meshes plus
training scratch require 90,288 transient normal-memory bytes. These are
allocation sizes, not measured peak process usage. Higher-density draws need
a runtime GPU cost gate before deployment.

Evidence:

- `glow-selective-budget-v1-test`: original default outputs unchanged; selective
  fixture 180/180 improves, including all four UV orientations.
- `glow-quality-keys-v1-test`: 154 asset bindings, 32,768 payload-bit mutations,
  metadata/extent rejection, actual host SHA256 and strict ARM policy compile.
- `glow-quality-pages-v1-test`: actual page-loader ASan/UBSan allocation,
  publication and release checks; strict ARM loader compile. Mock GPU allocator
  and flush do not establish hardware coherence independently.
- `glow-quality-v3-console-setup-builds`: isolated application build; seven
  intentional source changes, identical game archive and GPU programs,
  Old80/New124 CIA profiles, workspace/archive restoration verified.

Build v1 failed because the HUD compilation unit lacked the SDK crypto include
path. V2 stopped before mutation on an overly narrow patch anchor. V3 adds the
HUD owner to the existing portlibs include list. Failed evidence is retained.
No console deployment or dinosaur-rescue crash-fix claim follows from this work.

## Selected-field GPU checks

`glow-quality-gpu-v1/result.json` binds two private GPU fixtures to the exact
archived normal-app draw owner and linked adaptive shader. The source fields
produce 136 vertices/255 triangles and 133 vertices/256 triangles. All sixteen
static/dynamic, additive/modulated, nearest/linear comparisons match the
independently pretransformed reference pixel-for-pixel, with 54,000 drawn pixels
per case. The inspected captures show complete distortion patches. Removing
displacement is rejected: 35,927 differing pixels, maximum 16 byte levels.

These fixtures use the default UV orientation, an opaque mask and a procedural
capture texture. The reference uses the same approximation mesh, so this proves
GPU consumption rather than source per-pixel approximation quality. It is not
an in-game frequency/cost measurement, full-scene parity or physical timing.
Private fixture bytes and binaries remain under local build/artifact folders.

The first replay link found SHA256 was available only in the setup variant.
The isolated quality patch now links `mbedcrypto` through the shared library
list and removes the duplicate setup-only entry. Both application variants
must be rebuilt with that common link configuration before deployment.

Four-flip residency is a remaining load-time optimization opportunity, not yet
implemented. Blindly mirroring one mesh is unsafe: a single-axis reflection
exchanges the diagonals, while source geometry and sequential alpha use the
corner0–corner2 diagonal. Any shared topology must retain the source boundary
for every orientation and pass near-fade/deformation comparisons; ordinary
opaque, planar particles alone are insufficient evidence.

## Canonical shared compiler, not yet connected

`fh_glow_mesh_compile_shared` and `fh_glow_field_shared_compile` now seed both
diagonals explicitly, including constant fields. The four-triangle seed follows
the exact order produced by the ordinary compiler's first split. Existing
entry points retain their two-triangle initialization and outputs.

Actual native compilation of the five private fields once each produces 13,072
packed bytes versus 52,320 across twenty separately compiled orientations.
All 180 image comparisons still improve over omitted distortion. Against the
separately compiled meshes, 107 cases are identical, 29 improve, and 44 worsen;
the largest RMS increase is 0.539784 byte levels. This is a fidelity tradeoff,
not lossless sharing or full-scene acceptance. The inspected source-image
comparison remains a small virtual-particle fixture.

`glow-shared-contract-v1-test` verifies 255 old-entry output comparisons against
the archived compiler, 253 shared budgets with all-flip diagonal invariants,
constant-field topology, and failure behavior under ASan/UBSan. Strict ARM
compilation passes. These APIs are not wired into the page loader or shader;
the installed and archived applications still use separate flipped meshes.

The source route investigation identifies `projectileDoParticleFx` spawning
particle 0x79f (texture 0xc80), called by fireball, cloud-ball and Arwing projectile
families. Searching 0x7c6 also finds unrelated object/sequence IDs; those are
not evidence of the second selected particle's spawn route. The ordinary cave
idle route therefore remains insufficient for selected-field gameplay cost.

## Shared consumer integration

`pica_glow_shared_shader.py` maps canonical field coordinates to the source
geometry parameters using the existing full-texture UV rectangle. Corner shape
and sequential alpha use those mapped parameters. Mask and displacement remain
in canonical field coordinates. It adds no uniform uploads, vertex streams or
CPU vertex preparation. Stored shader instructions increase from 173 to 174;
the 42-float-register ABI remains unchanged.

`glow-shared-shader-v1-test` executes 115,200 static and 115,200 dynamic float24
comparisons against the original corner-emitting GS. Full-texture flips,
near-fade, signed narrowing, projection and alpha are checked; missing-flip,
missing-alpha and missing-offset mutants are rejected.

The actual GPU fixture covers both selected fields and all four flips across
64 cases. Sixty are pixel-identical; four nearest-filter cases differ at one
pixel by four byte levels (one gradient step), within the pre-existing bound.
All linear-filter cases match exactly. No missing pixels or alpha mismatch.
The inspected field0 double-flip capture shows complete geometry. These remain
opaque-mask diagnostic comparisons, not full gameplay or physical GPU timing.

`platform_pica_glow_shared_pages.h` compiles, packs and publishes one mesh per
field, returning the same immutable view for all four admitted rectangles.
Unsupported rectangles are rejected. Existing retirement, flush ordering,
64 KiB cap and 15% linear-headroom requirements remain. Temporary normal memory
is 73,272 bytes versus 90,288; view metadata shrinks by 768 bytes. Actual owner
ASan/UBSan and strict ARM compilation pass; loader stack is 112 bytes. Sixteen
flat fields occupy 1,792 packed bytes, retaining four triangles each so the
source diagonal remains valid even when displacement is constant.

`glow-shared-v1-console-setup-builds` couples compiler, owner and shader in an
isolated normal build. Nine intentional source changes, only the adaptive GPU
program changed, game archive unchanged, Old80/New124 profiles retained and
workspace restoration verified. Matching replay build and gameplay cost
acceptance remain required. Nothing has been deployed from this family.
