11 October New223 fur-projection deployment COMPLETE: normal
`d6068a861f8f` / CIA `238bbeb7a501`; replay `e6178b4e5cd0` / CIA
`b72e38226b55`. Eight changed app/identity files staged and byte-readback
verified, then renamed and sizes checked. Unchanged original asset/cache
payloads retain prior completed byte proof plus current descriptor/topology
and sizes; no fresh bulk asset readback. Saves and preferences preserved.
Old229 unchanged. Matching3dsx ready; install the CIA with FBI to update HOME:
`SD:/3ds/foxhollow/foxhollow.cia` (replay sibling folder/app name).

The normal app also passed its own32.43-second headless check: FUR enabled
through the bottom menu, ordinary Start to cockpit,1140 actual fur draws,
saved option mask1064961 with valid checksum, Select/drain/owned exit;900
capture inspected. New/Old replay checks and measured cost are detailed in
[fur projection](NATIVE_FUR_PROJECTION.md). This repairs one confirmed noise
coordinate error, not all fur effects or other reported visibility/lighting/
shadow defects. Physical projective-sampler performance and appearance still
need validation. FTP collection230702 contains the prior run, not new evidence
for the installed logging changes or a physical stall improvement.
Manifest: `artifacts/console-settings-update/20261010-231226-223/manifest.json`.

# Native fur projective sampling

11 October: a source comparison found that the native renderer discarded the
third coordinate of the original fur-noise texture projection. This affects
both native shell pose paths, even when wind is absent. The correction is
implemented locally; runtime and physical acceptance are recorded below as
they complete. It does not establish full fur fidelity.

`objFuzzRenderCb` in the pinned Foxhollow `objprint_dolphin.c` normalizes
`(s,t,1)` and applies the postmatrix `diag(37.5,37.5,1)`. Its Aurora shader
preserves all three outputs, interpolates them, and divides S/T by Q at the
fragment. The old native shader kept normalized S/T but discarded Q. Dividing
at the vertex instead would also be wrong between vertices with different Q.

The native shader now emits `(37.5*s*q, q-37.5*t*q, q)` to PICA texture unit0.
The subtraction flips the homogeneous T coordinate for native image storage.
Picasso packs `texcoord0w` into the unused `outuv.z` lane, retaining seven
rasterizer output registers. The original image is borrowed through a static
24-byte `C3D_Tex` descriptor with `GPU_TEX_PROJECTION`; its image, filtering,
wrap, LOD and other fields remain unchanged. The shared HUD descriptor is never
modified. Texture1 still carries the base image; optional texture2 coverage
still uses UV1. Every subsequent ordinary binding republishes its own type.

Consulted pinned Melee `port/3ds/citro3d_fix.c` and `renderer.c`: retain sampler
descriptors through SDK publication and invalidate bindings across owner
changes. This adaptation uses the existing SDK binding/publication path, with
no direct register writes, new image allocation, CPU vertices or extra pass.

The extra Q initialization replaces a mandatory nested-IF padding instruction:
the existing final colour write moves after the fur branch, where it provides
the required control-flow unwind. Picasso still inserts all required padding;
none is manually removed. Main/shared and standalone fur instruction counts
are unchanged. Colour, positions, lighting vectors, secondary UV and fog
outputs are checked independently of the changed projective coordinates.

Validation so far:

- Actual Azahar float24 VM: 32,768 ordinary plus 32,768 matrix-pose fur
  vertices; 1,736,704 retained lanes exact; maximum S/T/Q error
  `6.48431347e-6`. All 32,768 fur cases reject the old dropped-Q version.
- Separate 16-bone GPU-shell program: 32,768 fur vertices, 851,968 retained
  lanes exact, same maximum S/T/Q error. This test covers both shell steps and
  all fifteen layer indices. The first fixture attempt had uninitialized
  shape divisors and was corrected; that was a host fixture failure.
- Actual SDK binding and command emission: 100,000 projected/ordinary
  handoffs pass ASan/UBSan, preserving image identity and the original
  descriptor. A missing-projection mutant fails.
- Existing material equations and coverage checks pass with projected
  sampling bound. These arithmetic checks do not measure GPU throughput.

Evidence lives under `artifacts/research/pica-scene-rewrite/` in
`fur-projective-shader-test`, `fur-projective-binding-test` and the build/run
records. Earlier normalized-XY tests proved only that earlier approximation;
they did **not** establish the original projective mapping.

Wind remains separate and unbound. The new `pica_fur_wind_shader.py` prototype
reconstructs the original caustic lookup with GPU arithmetic. Its standalone
float24 check covers131,072 coordinates with at most0.21875 noise-texel error;
the combined326-instruction prototype preserves442,368 other output lanes.
Those prototypes were tested against the preceding, nonprojective shader.
They are not included in gameplay. Sampling wind only at existing vertices
still has unacceptable interpolation errors on some actual fur triangles;
the arithmetic check does not resolve that limitation. Full indirect colour
displacement, layered fur, projected shell shadows and original-image
acceptance also remain incomplete.
