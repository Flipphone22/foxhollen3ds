11 October New223 deployment COMPLETE: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`. New texture
page caching plus the accumulated environment-normal/unlit-material corrections
are installed. Eight changed application/identity files received full staged
byte readback, rename and final-size checks. Ten original asset/cache files
reuse prior completed byte proof plus current descriptor/topology bytes and
sizes; large asset bytes were not freshly reread. Saves/preferences preserved.
Old229 is unchanged. Matching3dsx ready; CIA requires FBI installation.
Physical stalls, fur and reported visibility/lighting acceptance remain open.
Evidence: `artifacts/console-settings-update/20261010-190758-223/manifest.json`.

# Source-selected unlit actor textures

11 October 2026: local correction, not deployed. Normal `08e74edfe122` /
CIA `f3a54758eea7`, replay `9f352cf3933d` / CIA `ce0d9a9cad42` are built.
The native load compiler now
preserves the original game's white-selector texture materials without diffuse
or secondary-light maps. `addShaderLayerStages` chooses the Swizzled family
before testing the object-color override; its mode-0 RGB is texture times
constant white. The native path incorrectly multiplied those textures by
vertex/object lighting.

`fh_pica_actor_unlit_texture` selects this immutable family at load time.
`fh_pica_material_actor_unlit` replaces only primary-color RGB operands in the
existing SDK texture-combiner descriptors. Alpha operands, masks, source
transparency, texture frames, poses, geometry and saves remain unchanged.
Descriptors still pass through the existing material-program cache and SDK
publication owner. No runtime flag, per-draw branch, additional stage/pass,
texture, buffer, worker or CPU vertex work is introduced. Pinned Melee's
texture-combiner setup and publication ownership were consulted; its GX
interpretation is not reintroduced.

The original-asset audit identifies 37 used single-layer material sections,
including front cockpit seats, ship and environmental objects. It parses 829
unique models and explicitly retains eight instruction-overflow variants as
unparsed. This is not a claim that all 37 appear in the tested view. Neither
SH_FernTree model238 nor Landed_Arwi model1152 belongs to this family, so this
is not a demonstrated fix for the reported disappearing trees or white Arwing.
Projected-light contributions and other omitted material families remain open.

## Evidence

`tools/test_pica_actor_unlit.py` executes actual original Swizzled RGB/alpha
statements against actual native load-compiler functions and SDK descriptors.
196,608 comparisons cover varying vertex colors, object alpha, intensity
channels, opacity masks and secondary layers; 3,072 cases check the immutable
selection predicate. Disabled-policy descriptors remain byte-identical, and
the old-policy mutant is rejected. Linux ASan/UBSan checks pass. This is a
source/descriptor test, not physical GPU raster acceptance.

Serial New/Old cockpit replays pass state assertions, preserve isolated saves,
drain logs and exit their owned processes in 37.01/36.40 host seconds. Both
retrace-900 captures were inspected: Fox, cockpit, menu and HUD remain present,
with Old touch controls. Loaded source assets include affected cockpit-seat
model469. Captures do not isolate all affected material pixels or prove whole
image parity.

| Azahar profile | Before scene / submit ms | After scene / submit ms | After FPS |
|---|---:|---:|---:|
| New | 0.758775 / 1.796348 | 0.754484 / 1.794344 | 59.8305 |
| Old | 0.757804 / 1.796599 | 0.759609 / 1.796437 | 59.8317 |

These short paced runs show no meaningful CPU regression, not a speedup or
physical-console FPS result. New fur counts are identical; Old animation phase
differs slightly. Heap, linear, VRAM, stack and process peaks are unchanged:
New heap47,848,144/71,303,168, linear free18,742,144/25,165,824;
Old heap40,508,112/50,331,648, linear free4,062,080/10,485,760;
VRAM free4,846,592; stack72,740/524,288; process103,198,720/130,023,424 New
and67,547,136/83,886,080 Old. Existing 511-instruction scene and
214-instruction fur programs are unchanged from the hemisphere correction.
Normal and replay share the same checked game archive, 318 native source
files and both exact tested shader blobs. CIA memory profiles remain
Old80/New124. Unchanged normal-app UI evidence is reused; no new normal-app
runtime is claimed for this material-load-only change.

Builders, snapshots and reports are under
`artifacts/research/pica-scene-rewrite/actor-unlit-*`; `inventory.json` records
every admitted section and unparsed variant. The build replaces only the
generated main/model game unit and native material compiler, while isolating
and restoring the six unrelated VRAM work files. Never blindly build that WIP.

## Reference correction

The archived desktop reference `artifacts/desktop/final/frame-040.png`, whose
run reached the title-menu state and exited cleanly, visibly has green cockpit
illumination too. The prior hemisphere report's observation of green lighting
must not be interpreted as proving a cockpit tint bug. Native and reference
images have different aspect/resolution and animation phase; this is a
qualitative correction, not a pixel-parity metric. User-reported red/green
glitches elsewhere, fur wind, day/night/shadow fidelity and physical visibility
and filesystem stalls still require repair or acceptance evidence.
