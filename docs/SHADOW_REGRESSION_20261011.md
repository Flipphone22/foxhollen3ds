Follow-up: [native projected-shadow components](NATIVE_PROJECTED_SHADOWS.md) are implemented and checked locally, but not yet connected or deployed. The cave regression remains unresolved.

# Cave shadow regression — unresolved

The user reports missing/distorted shadows in the cave replay save. This is a
regression, separately from the earlier shadows extending beyond ledges.
The installed New build is `86b336c4bfa5`; no replacement was uploaded during
this investigation. Its shared receiver/caster specialization and depth-band
clipping are deployed, despite historical “not deployed” text in earlier docs.

## Reproduction and isolation

Read-only collection `artifacts/console-visual/20261011-122556-223` retained the
current log, build manifest and both settings files. SHADOW SHAPE is enabled.
The saved PPM is stale (retrace20965 versus current run4819), so it cannot verify
this run's appearance. A shutdown display-transfer timeout follows completion
of the gameplay GPU queue; it is not established as the shadow regression's cause.

Two serial, isolated New-profile runs use the same cave GCI, original assets,
current console settings and normal Start/A/Select input. Both finish and drain
logs, preserve saves and exit their owned emulator. Captures1560 were inspected:

- `artifacts/native/shadow-regression-v1-new`: exact installed3dsx,88.31host seconds.
  Fox's ground silhouette remains, but the broad mushroom shadow is missing.
- `artifacts/native/shadow-band-diagnostic-v1-new`: diagnostic`b65f9f7d7534`,91.06seconds.
  Only receiver rejection and its stencil predicate differ. Missing shadows return,
  along with projection onto inappropriate surfaces. This is not a release fix.

The game archive and all other native source hashes match. Captures are
asynchronous and animation phases are not guaranteed identical. The diagnostic
retains aggregate actor accounting for the skipped receiver quad; those totals
must not be used as actual submitted workload or performance evidence.
Recorded comparisons: `artifacts/research/pica-scene-rewrite/shadow-regression-result.json`.

The narrow0.25-unit band around one collision plane rejects legitimate receiver
coverage. Widening it indiscriminately would restore the off-edge defect. The
controlled removal isolates the rejection pass, but does not measure how much
each collision/render-mesh mismatch, slope or physical rounding contributes.

An independent numerical investigation also found SDK depth-coefficient rounding
can reject27 of364751 synthetic ground samples. Compensating those coefficients
removes those27 in the host model. This is not a GPU arithmetic oracle or proof
that compensation repairs the reported cave. No production precision patch was
applied. `tools/test_receiver_precision.py` records that experiment.

## Replacement work started

The original `newshadows.c::renderShadows` renders shadow textures; its
`shadow_dolphin.c` receiver path collects finite nearby geometry and
`intersect_render.c::objectShadow_setupProjectedTextureDepthFade` projects those
textures with a depth ramp. It does not flatten all receivers onto one plane.
The source depth-fade family also scales alpha by `(a>>1)+(a>>2)` and uses
multiplicative colour blending; the current black silhouette is not full parity.

New `native-3ds/source/pica_shadow_project.v.pica` is an unconnected native
receiver specialization. It preserves receiver XYZ, projects live shadow/ramp
matrices and emits distance-fog coordinates. The pinned Azahar interpreter checks
32768 nonplanar vertices and917504 output lanes, with zero observed error.
A mutant flattening receiver height is rejected. The first version exposed an
incorrect partial-write swizzle in texcoord0.z; that was corrected in the shader.
This is21instructions in a separate program; the existing scene program is
unchanged. It does not yet allocate a target, submit gameplay draws, or establish
visual, hardware or performance acceptance.

Next integrate bounded persistent receivers and a GPU-produced shadow image,
with the main owner's cache/queue/lifetime rules, rather than widening the faulty
plane band or reconnecting retired GX work. Compare actual cave images and costs
before deployment. Softness, multiple/moving receivers and full fidelity remain
open. No new hardware replay is requested for this diagnostic.
