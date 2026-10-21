12 October MOVING COLLISION COVERAGE / SPATIAL PROTOTYPES progress.
Goal ACTIVE/INCOMPLETE, no blocker. Hardware unavailable: no FTP, new probe,
or hardware request. Both installed normal apps and all saves remain unchanged.

Current collision V2 original-geometry Thorntail moving replay PASS 77.92hostsec,
state/save preservation/drain/owned exit. Images1500/1650 inspected: Fox/world/
HUD/Arwing/ground silhouette present.25 receiver builds,0declines/0missing plans.
2149casters/2454authored masks/7085draws/1391343 submitted POINT records.
Range telemetry1413393 includes22050 points subsequently culled by texture bounds;
do NOT require range count equal final draw count on this moving route.
NewAzahar53.3319 vs priorworldvisibility52.9963, source poses/counts differ;
scene+submit +.039889ms, NOT a speedup or physical projection. Scene1.992383,
submit4.427078; heap56582928/71303168, linearfree16033280/25165824,
VRAMfree4723712, stack73068, process103325696/130023424.
Evidence collision-current-test/moving.json; check_collision_current_moving.py.

Inspected capture shader owner: it ALREADY binds receiver_caster_program,
not main511 shader. Do not pursue duplicate dedicated shader under false premise.
Shared linked instruction totals are not per-vertex executed instruction counts.

Added UNWIRED bounded source-order gap prototype fh_pica_shadow_spans.{h,c}:
<=4 ranges,<=256 extra tree visits,8 exact-key cache entries, no geometry copies,
no reorder/no skipped potentially visible point. ASan/UBSan15841400 inclusion
checks, direct/reversed routes,65535point limit,offset extremes/cache reuse PASS;
offset/boundary mutants rejected; strictARMv6K compile and stack report retained.
Initial test caught adjacent empty nodes not coalescing (768 instead of512 points
in fixture); pending-gap coalescing repaired it, final removes1536/2048 safely.
Full actual-source audit1267blocks/60816queries/six moving poses EXACT3643219
triangles. Broad12980246 -> disjoint11898761 points (8.33% extra reduction),
36439 resulting draws,1916136 node visits. Not integrated: command/CPU/memory
tradeoff unaccepted, no hardware request justified. Sources only in worktree,
snapshot builders must continue shelving unrelated WIP modules.

Separate live-bound prototype (artifact-only live_ranges.c) intersects current
triangle extents with authored query bounds. Same full original-source audit
PASS;11920880 points (8.16% vs broad),423455 visits. It would require correct
mutation refit/cache invalidation; not wired, not a claimed zero-cost win.
Source audits rebuild per query to isolate selector correctness; that is NOT
proposed runtime behavior. Neither reduction is additive or measured GPU FPS.
Evidence shadow-spans-{test,source-test},shadow-live-bounds-source-test,
collision-current-test/spatial-experiments.json. Full effect cave CPU still
.729173ms vs .5ms gate. No defaults changed/no new normal build/deployment.

All owned sessions TERMINAL:71226moving,11491source-gaps,54591live-bounds.
No emulator overlapped host-heavy checks. Goal remains incomplete: physical
sky/shadow acceptance, receiver/source-limit/softness parity and overall target
performance remain open. Continue locally; hardware unavailability alone is not
a blocker. Prefer one consolidated ordinary gameplay check when available.
