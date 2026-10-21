# Native ground-contact shadows

Current state, 10 October: New e783636ad1e8 has this circular approximation as
an opt-in; Old remains on its prior build. The historical failed gate below
explains why shadows default OFF. The private receiver rewrite and animated
planar silhouette are local candidates, documented in NATIVE_SHADOW_RECEIVERS.md
and NATIVE_PLANAR_SHADOWS.md. They have not replaced the installed implementation.

10 October cave gate FAILED for the initially prepared build. With fog OFF,
the new Old-profile Azahar cave ON/OFF comparison measures scene preparation
5.215503/1.789037 ms and 48.791/58.210 FPS. The added 3.426 ms exceeds the feature
budget. Palace-only acceptance did not cover this. Hold the prepared upload
until receiver work is reduced and checked in the cave; a zero-alpha-before-
collision-query correction is under test. No console has received this build.

The subsequent fog/settings deployment candidate retains this effect only as
an explicit opt-in. Normal gameplay defaults it OFF, including migration of
previously deployed preferences. The alpha admission correction reduced the
cost but did not meet the cave budget. Do not describe this release choice as
a shadow optimization or as complete original shadow parity. Replay recipes
still select their own masks; all-on recipes include the expensive effect.

Local candidate only. This restores a soft ground-contact cue and the original
simple ground-decals. It does not implement projected character silhouettes,
wall receivers, self-shadowing or shadows from arbitrary lights. Those remain
required visual work. The ground-contact approximation is an authorized detail
reduction, not a claim of original shadow parity.

The renderer uses the existing immutable native quad and resident source mask.
PICA performs the plane/view/projection transforms, alpha sampling and blending.
There is no generated vertex/index stream, framebuffer readback, texture upload
per frame, new shader instruction, or general GX-state interpretation.

The original simple ground-shadow path supplies its cached S16/8 corners and
distance fade. An affine fit distributes their quantization residual across four
corners (maximum 0.001953125 world units in 30,000 tested footprints). The shared
quad shader uses two fractional position bits; the wrapper supplies a 4-by-4
rectangle to produce the intended unit square. This caught an initial scale
mistake before any hardware deployment.

For ordinary character casters the CPU queries the original collision system
for the receiver distance and normal, then derives a plane transform directly.
This is necessary live receiver information; the PICA has no access to the
game's collision-query structures. It does not rebuild the character or terrain
meshes. That query is currently the dominant added CPU cost. A future cache must
account for moving geometry, map changes and object lifetime; an arbitrary stale
ground height is not acceptable.

The original small disk is registered at the load boundary, using the existing
native image owner table and retirement fences. The native quad's depth policy
preserves the original decal viewport near=-0.05/far=1, with reversed-depth
GEQUAL, no depth writes, and source-alpha blending. This follows the pinned
Melee renderer's reversed-depth and cache/ownership conventions. Native HUD
and scene state are invalidated at the handoff, including after a particle
geometry-shader batch. No second geometry shader is introduced.

GROUND SHADOW is a separate, immediately applied graphics option; it remains
independent of HUD visibility. No simulation steps or inputs are omitted.

Initial acceptance budget: at most 0.5 ms additional CPU work in the New palace
scene and 1.5 ms on Old, with no new vertex-buffer allocation. These are feature
budgets, not physical measurements or a universal emulator scaling rule. Overall
16.67/50 ms targets and memory headroom still apply.

The v2 palace comparison drew 1,166 ground footprints across the complete run.
The measured window added about two draws/frame. Azahar stayed at 59.83 FPS in
both ON/OFF runs; scene preparation increased 0.346 ms and native submission
0.053 ms. Peak heap, linear and VRAM values were identical. Captures show a soft
footprint beneath Krystal with surrounding geometry, actors, particles and HUD
retained. Source masks and animation phases vary, so this is not a pixel-identical
paired frame. Evidence: `artifacts/research/pica-scene-rewrite/ground-shadow-v2-checks/validation.json`.

The earlier v1 cave check completed with zero shadow draws and therefore did not
validate the effect. Its live object inspection rejected a CIA mapping mismatch;
no guest memory was changed and no validation condition was relaxed.

V3 removes the prototype's per-query quantized corner scratch and derives only
the live plane transform. New/Old exact-CIA palace replay checks passed, including
shutdown, state assertions and visible footprints. New ON/OFF measured 59.831 FPS;
Old ON/OFF measured 57.714/58.430 FPS in Azahar. Added scene/submission time was
0.346/0.050 ms New and 0.370/0.049 ms Old. New OFF reuses the unchanged disabled
path from v2. Other phases vary, especially audio; these single runs do not
establish a full-frame hardware cost or a console speedup.

Peak heap was unchanged at 48,148,552 bytes New / 40,808,520 Old; minimum linear
free 19,474,048 / 4,793,984 bytes; initialized VRAM free 4,858,880 bytes on both.
Both runs retain over 15% headroom in the tested scene. Evidence is in
`artifacts/research/pica-scene-rewrite/ground-shadow-v3-checks/validation.json`.
Physical-console behavior, movement across uneven receivers and extended
coverage remain open. This does not resolve projected shadow parity.
