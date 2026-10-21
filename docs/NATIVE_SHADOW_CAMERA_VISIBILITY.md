# Shadow receiver camera visibility

Experimental local candidate `shadow-visibility-v1-builds`, based on checked
world-visibility-v3. Nothing is deployed and no performance gain is established
until the matched checks below have been assessed.

The projector previously tested every resident receiver against every caster's
shadow footprint, even if the receiver was wholly outside the camera. The
candidate calculates camera visibility at each distinct frame registration and
stores four words per world instance. It retains the existing asset/view/
projection/scissor key, current-frame ownership and asset-forget path.

Each caster can skip offscreen receivers before footprint tests and SDK draw
publication. An entirely offscreen instance also skips its matrix concatenation.
The first128 receivers are represented; higher ordinals always remain eligible.
This is an optimization limit, not a geometry limit. There are no per-frame
allocations, vertices, indices, image copies or additional GPU passes.

World positions can change after a receiver was registered in the same frame.
After rebuilding its bounds, the source owner therefore marks all cached draws
for that asset eligible before publishing the modified GPU storage. This fails
open until the next frame's registration instead of retaining stale rejection.
Source shadow order, capture, masks, receiver geometry, raster state and GPU
retirement remain unchanged. Camera tests use the same conservative homogeneous
side-plane helper as native world visibility, adapted from pinned Melee
`port/engine/geometry_bounds.h`; no near-plane or perspective-divide shortcut.

The fixed extra normal-RAM cost is128 instances ×16 bytes =2048 bytes, plus16
bytes of counters. No extra linear RAM or VRAM is allocated. Registration exits
before accessing camera/geometry when shadows, rendering, world or actors are
disabled. The original scene shader and game archive are unchanged.

`test_shadow_camera_owner.py` compiles the actual candidate's registration,
visibility, invalidation and forget functions with ASan/UBSan. It covers1000
frame/camera/duplicate/mutation/batched-owner/capacity cycles and5000 disabled
calls with deliberately invalid input pointers. A broken invalidation variant
fails. The patched position-mutation owner's call order is checked separately:
receiver bounds rebuilt, cached rejection invalidated, then GPU storage flushed.
This host test does not establish hardware visibility or total frame savings.

Matched New 3DS Azahar cave checks completed with shadows enabled on both builds.
Both runs preserve saves, drain logs and exit their owned emulator normally.
Scene plus submission decreased only 0.028172 ms (7.703895 to 7.675723 ms).
Caster and authored-mask counts match (715 and4290); receiver draws decrease
46475 to34320 and indices48461985 to43652895. Fixed2048-byte cache is normal
RAM; measured heap/linear/VRAM/stack and process page commitment are unchanged.
Both1410 captures were inspected: cave, player, HUD and ground shadow remain;
fog and animation phases differ, so this is not pixel-identical validation.
The candidate remains unpromoted. No physical GPU saving or console FPS claim.
See shadow-camera-owner-test/runtime.json for complete timings and counters.
