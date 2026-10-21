# Normal-lighting substitution gate, 14 October

`tools/audit_bump_lighting_substitution.py` now compares two possible fragment
normal-lighting substitutions before connecting the candidate texture compiler.
It evaluates17,458 authored vertex/format references on the17 audited materials
under12 rigid views:209,496 samples, including104,344 with positive view-normal
Z. It uses actual auxiliary pixels and original UV fractions. Where the source
inherits the indirect scale, this experiment explicitly assumes scale0.

The reference is the *idealized hemisphere-coordinate equation*, with clamped
XY and reconstructed positive Z, followed by three specified directional test
lights. It is not the original rasterized/filterable32x32 image, a captured
lighting setup or a PICA execution. The comparison excludes pose deformation,
visibility, area weighting, material composition, quantization and quaternion
interpolation. Positive-normal-Z filtering is a diagnostic subset, not actual
triangle culling. Results cannot be presented as observed game image errors.

Even granting exact non-orthogonal basis handling, directly lighting the
perturbed vector does not reproduce the source hemisphere lookup. In the
positive-normal-Z subset it is worse than leaving bump off on8of17 materials;
on9 it improves this conditional RGB metric. Across all sampled views it is
worse on17of17, but many such views can be invisible in gameplay, so that alone
is not a deployment verdict. For Arwing1152, positive-Z RGB RMS is0.022282 for
direct lighting versus0.013421 for omitted bump. A nearest orthogonal basis,
even generously allowing reflections that a quaternion cannot represent,
gives0.020250. Thus there is no basis for enabling this as a blanket repair.

The field/RGB8 compiler remains an unconnected experiment. Its precision tests
are still valid **only for the prepared local field**. They do not validate a
source-compatible fragment renderer. Any consumer must demonstrate affected
scene images and cost, preserve the hemisphere operation or explicitly validate
a narrower approximation, and count unsupported material families. Do not add
normal mapping globally merely because the pixel encoding passes.

Evidence: `artifacts/research/pica-scene-rewrite/bump-lighting-substitution/result.json`.
This changes the next implementation decision: reject unconditional direct
normal-lighting integration; investigate a source-compatible lookup strategy
or a measured material-specific approximation. Do not emit the rejected64x
geometry expansion or reconnect the retired CPU/GX path.

The pending uniform-retire performance comparison was also rechecked. Four
live foreign `cc1plus` processes were observed (PIDs5111/5114/5118/5122) before
any replay launch. No new timing run was started under that contention and no
foreign process was modified. Existing performance gates remain open; this is
not a verified wait or a reason to mark the overall goal blocked.

No app build, runtime rendering change, FTP operation, deployment, private asset,
save or preference modification occurred. Hardware shadow flicker remains
unresolved. The broader rendering, sustained-performance and final gameplay
acceptance requirements remain incomplete.

---

# Fragment-field texture compiler, 14 October

A second native preparation component is implemented locally, still **not a
connected bump renderer**. `fh_pica_bump_field` compiles the source auxiliary
A/B displacement into an RGBA8 local normal-field candidate.
`fh_pica_bump_texture` emits the same RGB directly into native three-byte RGB8
Morton tiles, with no full-image intermediate or per-frame CPU work. The
compiler allocates nothing and retains no source pointer. No game build,
runtime image owner, asset replacement or console deployment was made.

For original strength `v`, the represented field is
`(2*v*(alpha-128)/256, 2*v*(blue-128)/256, 1)`.
Components are encoded into unsigned bytes after division by
`max(1, abs(v))`; the consumer must recover that scale. The third component
must remain authored, rather than being reconstructed as a unit normal.
Pinned Citro3D `C3D_LightEnvBumpNormalZ(env, true)` and the local Azahar fragment
generator were checked for that meaning. An 8-bit signed-normal encoding has
no exact zero: the compiler tests and reports this quantization, including the
zero-strength case. A future consumer can omit zero-strength bump work entirely.

This is only a possible native fragment-lighting input. It does **not** solve
non-orthogonal authored B/T/N reconstruction, the source's dependent
hemisphere-map lookup, its source Z reconstruction, light/material composition,
animated sampler state, or physical performance. Do not connect it by silently
substituting ordinary normal mapping for those operations. No image or hardware
acceptance is claimed.

The CPU implementation preserves the original signed A/B channels and repeat
independence. It checks dimensions, counts, capacity and pointers before writing
pixels or decode scale. Native output accepts power-of-two dimensions8..1024,
uses PICA B,G,R byte order and preserves source Y orientation. These are data
preparation functions: a future owner must still provide aligned GPU-visible
allocation, cache publication, memory accounting and existing-queue retirement.
Pinned Melee texture-repacker layout and command-cache rules were consulted;
no new queue, render pass, GPU register override or worker was introduced.

`tools/test_pica_bump_field.py` passes under ASan/UBSan:

- 1,048,576 source byte-pair/strength combinations, bounded maximum component
 error0.006722689 before filtering.
- 15,728,640 repeat-equivalence cases; the repeat nibble changes sampling
 coordinates, not the immutable field.
- 24,969,600 texel comparisons over square and rectangular dimensions8..1024,
 against linear field data and independent Morton decoding, plus malformed
 publication and sentinel checks.
- Wrong channel, signed zero, Z reconstruction, Morton ordering and RGB order
 mutations are rejected. Strict ARM11 hard-float compilation passes.

The previously tested input compiler's two function bodies are byte-identical;
its source-parameter and authored-basis checks are reused explicitly after the
last packing addition. The full source now hashes326b0e2770c5..., so older
whole-file hashes must not be presented as this revision's identity.
Evidence: `bump-field-test/result.json` under the usual research artifact root.

`tools/audit_bump_fragment_field.py` validates all17 audited materials using
actual original auxiliary images and139,264 filtered samples. Maximum local
component error is0.011718473, within encoding plus source byte-rounding bounds.
This measures a local vector field, **not** source rendered colour or the
previous audit's view-dependent environment coordinates. Native tiled output
recovers the linear RGB exactly. The shared `tools/bump_sampling.py` decoder
and filtering helpers also reproduce the preceding rejected vertex route's
results; no geometry expansion was adopted.

There are9 unique auxiliary-image/strength field combinations in this static
inventory. Full-collection RGB8 storage is2,018,304bytes, versus2,691,072bytes
for RGBA8:672,768bytes less, before residency, mipmaps and owner overhead.
Largest512x512 field is786,432bytes in RGB8. These are preparation estimates,
not allocated memory or a claim of reduced gameplay RAM. Such a collection
must not be allocated eagerly on Old3DS. A bounded resident consumer and its
actual working-set budget remain to be implemented. All fields and source
images stay private; no converted game data is embedded into shipping code.

No emulator boot was repeated for disconnected compiler changes. The installed
normal/replay programs and existing frame timings remain unchanged. Whole-shadow
hardware flicker and the broader rendering/performance goal remain incomplete.

---

# Native environment-bump preparation, 14 October

The native load-time input compiler is implemented locally in
`fh_pica_bump.h` / `pica_bump.c`, but **no bump gameplay renderer is integrated**.
No normal/replay executable, deployed file, private asset or save changed.
This is progress toward the missing material family, not a visual repair or
performance improvement. The hardware whole-shadow flicker remains unrelated
and unresolved.

The compiler retains the two independent authored signed-byte axes, with
explicit padding for an eight-byte PICA fetch extent. Existing normal and bone
weights remain separate. It does not orthogonalize, normalize, allocate, retain
source pointers or rebuild geometry each frame. References are fully checked
before any output is written. Source backing extents remain the caller's
responsibility; invalid input leaves output unpublished.

The material compiler preserves the original strength expression, texture
repeat, indirect-scale exponent and whether the original function writes that
scale at all. A no-write does not mean scale zero: preceding state is inherited.
The original table has nine entries; inputs that would read beyond it reject
instead of inventing an answer. Original simulation and source renderer files
are unchanged. Pinned Melee command-cache publication rules were consulted;
there is no new live GPU owner, upload or lifetime change at this stage.

`tools/test_pica_bump.py` compiles the actual original
`addEnvMapBumpStages` with recording GX adapters and tests 73,280 parameter
cases. It validates all 17 selected material declarations on 12 models,
17,458 distinct source references and 51 rejected malformed groups under
ASan/UBSan. Swapped B/T and wrong indirect-scale mutations fail. The native
module also compiles with the ARM11 SDK compiler, hard-float and
`-Wall -Wextra -Werror`. These are input/setup checks, not rasterization tests.
Aligned basis storage across this inventory is 139,664 bytes before palette
splitting; it is an offline estimate, not allocated resident RAM.
Evidence: `artifacts/research/pica-scene-rewrite/bump-native-test/result.json`.

## Vertex-sampled route rejected before runtime integration

`tools/audit_bump_vertex_sampling.py` uses original ordered triangle UVs and
NBT axes, and original TEX1 auxiliary images (direct model texture IDs bypass
TEXTABLE). The source indirect operation samples **alpha and blue**, then
subtracts 128. The proposed shortcut sampled that field only at vertices and
interpolated the resulting displacement. No retired per-draw GX path was
reintroduced.

The audit evaluates 665,680 interior/edge sample comparisons across the 17
materials. It tests every legal inherited scale where the source does not
write one. Eight source images decode pixel-for-pixel like the native decoder;
8,192 bilinear samples agree within 0.000204 byte levels. Rounding at half-byte
boundaries can still differ with floating precision. Initial audit setup
incorrectly admitted only VAT5/7 and stopped before results; it now admits the
actual supported VAT3/5/6/7 and keeps their existing UV fraction rules.

For Arwing1152/material0, at inherited scale0, the repeat is17 and source
auxiliary image32x32. The source-vector displacement comparison gives:

| Triangle count | Uniform subdivision | Coordinate-error p95, in 32-map texels |
|---:|---:|---:|
|3,786|none|1.4340|
|15,144|2 per edge|1.3955|
|60,576|4 per edge|1.3385|
|242,304|8 per edge|1.2420|

The unsubdivided case is worse than omitting the displacement at64.33% of
sampled points. This metric alone is not a final colour-quality verdict.
All three refinement candidates fail the provisional half-texel p95 threshold,
while adding4/16/64 times the triangles. No candidate mesh was installed.
Some highly downscaled inherited cases pass this coordinate threshold, which
is not a license to change the source scale to make the test pass.

These are conditional static measurements: level-zero bilinear filtering,
authored/identity-pose bases, ten barycentric sample locations, and no camera,
visibility, raster area, mip selection or dynamic material mutation. The
three-vector error bounds the two-coordinate error under a rigid camera;
32 is an explicit illustrative environment-map size, not every material's
actual target. Final image fidelity, source-relative GPU cost and physical
performance remain untested. The sampling assumptions and every scale case
are retained in `bump-vertex-sampling/result.json`; subdivision adds3,180,240
point comparisons to the main inventory.

The next native design must retain fragment-frequency detail without this
geometry expansion. PICA fragment lighting/LUTs and prepared field textures
need comparison against the actual source diffuse/specular functions; a
conventional orthonormal normal-map substitution is not automatically valid.
Do not attach the eight-byte basis stream to every actor until a measured
consumer needs it. Keep all general scene programs and existing renderer
budgets unchanged meanwhile.

---

# Source environment-bump inputs

11 October: source audit only. No native bump implementation or new visual-fix
claim. The original game applies sampled texture offsets to environment-map
coordinates; this differs from conventional tangent-space normal mapping.

`shader_dolphin.c::addEnvMapBumpStages` samples an auxiliary image, biases its
S/T inputs, transforms authored binormal/tangent coordinates, accumulates two
indirect offsets and forwards them into subsequent lighting lookups.
`addEnvMapTexCoord` selects its sampling scale from the original image sizes
and packed material parameters. A PICA conversion must preserve those inputs
or quantify an intentional visual approximation; replacing the material with
an unrelated normal-map shader is not source parity.

Pinned Citro3D `include/c3d/light.h` and `source/lightenv.c` provide SDK-owned
bump mode, texture-unit selection and normal-Z controls. These alone do not
perform the game's two dependent coordinate shifts. Existing material, light,
sampler and shader ownership rules still apply.

`tools/audit_pica_bump_materials.py` reads the private original model collection
without modifying assets. It found17 used material declarations across12
models with auxiliary environment-bump inputs; all17 use authored NBT arrays.
Eight unrelated alternate model streams remain unsupported by this inventory,
so this is not an exhaustive whole-game coverage claim.

The affected declarations contain63,528 triangle corners. Retaining two extra
signed-byte basis vectors for each distinct source vertex reference would add
104,748 bytes across the whole inventory, before native palette splitting,
alignment, texture storage or runtime residency decisions. This is an offline
size estimate, not an allocation or measured RAM peak.

Landed Arwing1152 material0 uses4,381 distinct source vertex references,
2,310 authored bases and11,358 triangle corners. Its extra six-byte basis
attributes would occupy26,286 bytes before the same packing considerations.
Vectors are near unit length (0.9750–1.0), but1,828 bases have a normalized
pairwise absolute dot product above0.05, and21 have determinant magnitude
below0.05. Both handednesses occur. A single rotation quaternion with assumed
orthonormal axes therefore cannot preserve all authored bases. A future
persistent native plan must retain the source vectors and binding semantics,
or measure and visually validate its approximation.

Evidence: `artifacts/research/pica-scene-rewrite/bump-material-audit/report.json`.
The archived `texture-window-v1-arwing-new/hid-1410.png` shows a textured Arwing
and trees. That image does not reproduce the reported intermittent physical
visibility fault and must not be used to declare it fixed.
