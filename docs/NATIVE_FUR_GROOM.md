# Native fur direction stream

Current integration (11 October): build `b195b26ac1d3`, optional **FUR GROOM**,
off by default. Enable FUR and SOURCE FUR, then compare FUR GROOM on/off in the
same view. This is a static direction approximation, not full original fur.
The deployment record is in AGENTS.md and docs/IMPLEMENTATION.md.

The original map is sampled once while loading each eligible native fur plan.
Four signed bytes per vertex carry its direction to the dedicated PICA shader.
No geometry subdivision or extra rendering pass is introduced. A bounded owner
publishes aligned, flushed buffers and releases them after the parent mesh fence.
Global limits are 64 KiB linear and 16 KiB metadata; the tested Fox uses 2,688
aligned linear bytes and 60 metadata bytes, even with the option off.

Compiler, lifetime, linked-shader and two serial normal-app runs passed. Both
runs drew 11,056 shells and 37,114,992 fur indices. Captures show changed texture
direction, but do not establish improved resemblance to the original. Runtime
evidence: `artifacts/research/pica-scene-rewrite/fur-groom-runtime-test/comparison.json`.
Physical GPU behavior and cost remain to be tested. No Old hardware/runtime
result is claimed for this candidate. Moving wind distortion, projected lighting,
interpolation error and unsupported native streamed-fur families remain open.

The following records the earlier prototype investigation; statements about
integration not existing refer to that earlier stage.

# Fur direction detail investigation

11 October user clarification: SOURCE FUR ON is **underwhelming compared with
the original**, rather than obviously corrupted. Do not interpret this as a
report of broken geometry, and do not offer more source shells as the fix.
SOURCE FUR currently selects source density; it does not promise the complete
original fur material.

The original `objFuzzRenderCb` has two indirect coordinate contributions:
moving caustic distortion and a static authored direction map. The native
material consumes the latter's alpha as coverage but ignores its blue/green
directions. Source sampling expands those channels to bytes, subtracts128,
then applies the layer-dependent indirect matrix. This is a specific missing
component even at full density. Projected fur lighting also remains incomplete.
Neither finding by itself proves which difference dominates the user's view.

`tools/audit_fur_groom_materials.py` resolves used fur declarations from the
private original MODELS banks and their original textures. Crucially,
`ObjModel_Load` calls `textureLoad(-(id | 0x8000),1)`: model IDs bypass TEXTABLE
and select TEX1 directly. The initial audit used the generic positive-ID remap
and resolved the placeholder texture; that audit was corrected before drawing
conclusions. Final inventory finds nine unique used fur model/material pairs
in those MODELS banks, three with direction maps. This is not a claim of every
runtime callback/material family being covered. Fox465 and model1 share the
same64x64 RGB5A3 map; its blue/green channels are nonconstant. Only metadata and
statistics are written to the report, not private images.

An immutable per-vertex direction stream would avoid per-frame sampling and
CPU vertex rebuilding. Its quality was measured before runtime integration:

| Fox465, outer shell14 | Existing mesh | Uniform4x triangles | Uniform16x triangles |
|---|---:|---:|---:|
| Fur triangles | 1,119 | 4,476 | 17,904 |
| 95th-percentile coordinate error, noise texels | 2.186 | 0.949 | 0.348 |
| Maximum sampled coordinate error, noise texels | 8.138 | 3.228 | 2.119 |

The samples cover four barycentric points and three clip-W configurations,
excluding zero-coverage points. They use base-mip linear repeat filtering;
original minification/mipmap behavior is not reproduced in this audit. They
are not pixel differences or a maximum-error proof. The comparison between
omitted and approximated coordinate offsets does not establish a visual win:
the noise repeats, and sampling different coordinates can change its appearance
non-monotonically. Uniform subdivision would add substantial GPU work across
every shell; it has not been accepted as a solution. A quarter-texel threshold
is an investigative accuracy gate, not a proven perceptual requirement.

`tools/pica_fur_groom_shader.py` implements the candidate vertex kernel separately
from the gameplay generator. A proposed attribute6 carries signed B/G offsets;
an unused component of the existing shell uniform carries layer/4096. Two
instructions add the offset multiplied by Q, preserving division after
interpolation. The specialized program grows214→216instructions, retaining96
float registers,16booleans and7raster outputs. No additional texture sample,
geometry pass or output is needed by this arithmetic kernel. These are static
shader limits, not a measured full-feature cost; publishing/reading an additional
vertex stream still has CPU/GPU/memory cost and is not implemented yet.

Actual Azahar float24 checks pass32,768ON/OFF vertices: OFF output exact,
851,968other lanes exact, maximum homogeneous UV error5.27407974e-6. A missing-Q
calculation disagrees in30,456cases. This proves the vertex arithmetic only;
it does not resolve interpolation quality, physical shader behavior or visual
acceptance. The initial test harness expected an older fixture's shell-uniform
initializer; updating that fixture reference fixed the assertion without changing
the shader.

Evidence: `artifacts/research/pica-scene-rewrite/fur-groom-audit/` and
`fur-groom-shader-test/result.json`. No gameplay renderer, game archive, build,
console installation, asset, settings or save changed during this investigation.
The fixed C-stick menu remains deployed. The local shadow depth-band candidate
remains unaccepted for physical cost and is not deployed.

Next work is a bounded immutable direction representation with visible comparison
at native resolution, including how much approximation can be accepted without
the cost of uniform subdivision. Moving distortion and projected lighting remain
separate requirements. Fur quality, full renderer parity and performance goals
remain incomplete; there is no external blocker.
