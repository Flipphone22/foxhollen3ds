# Compatible native world command runs

The previous shipping candidate capped load-prepared groups at four draws even
though its resident packet compiler and sealed GPU call mechanism support
sixteen. This change uses that existing capacity. It does not reconnect GX,
change geometry, or reconstruct command lists in the frame loop.

The load owner walks draws in source order, ending a group at an unready or
hidden draw, vertex-format change, material fog-state change, or sixteen draws.
The next iteration begins at the boundary, so eligible draws following it can
still form groups. If the proposed group exceeds the existing 128 KiB linear
or 24 KiB CPU cap, or the 15% linear-memory reserve, the owner shortens the
unpublished prefix until it fits. Allocation failure retains individual native
submission. It never removes a source draw.

Per-frame bounds checks, live visibility and fog validation, epoch ownership,
dirty-range flushing, sealed command calls and fenced retirement are unchanged.
Larger groups can reduce batching coverage when one member is culled; replay
coverage must be measured before claiming a submission improvement. The pinned
Melee `native-3ds/port/3ds/command_cache.c` explicit segment-flush rules were
reviewed. This adaptation changes only load-time grouping, not cache ownership.

`world_packet_runs_patch.py` applies the change to the archived tick-split
candidate. `build_world_packet_runs.py console-setup --revision v1` isolates
that archive and restores unrelated root work afterward. The game archive,
all eighteen GPU programs, shared Old80/New124 profiles and setup recipe must
remain unchanged. No extra worker, shader, pass, geometry or scratch capacity
is introduced. Group headers and exact command lengths determine resident cost.

`test_world_packet_runs.py` executes the actual candidate owner and archived
packet compiler on ARM11. It passes 3,403 checks: lengths 1–33, exact ordered
draw membership, mixed fog/format/readiness/visibility, same-epoch rejection,
failed publication, exact CPU/linear admission limits, 15% reserve and allocator
failures. SDK fences and selectors are simulated in this test; it is not a
physical GPU result or full-game visibility proof.

The unrestricted candidate source19d46d03e0a6 passed the Old palace runtime
and capture check, but is rejected for promotion. Of 768 native world draws in
the measured eight-frame window, grouped coverage falls from528 to320 and
individual submissions rise from240 to448. Group calls fall144 to32, which
alone would give a misleading impression of improvement. Individual append
work rises146408 to262000 ARM encounters. Whole-window work rises1.10% with
other phase differences; this is not an attributable FPS change. No deployment
or hardware request follows this candidate.

The follow-up `world_packet_spatial_patch.py` retains source order but stops a
group when its meshes no longer share an intersecting bounds region. Bounds
are computed from the actual native geometry at load time. This is a grouping
heuristic only: live frustum checks remain unchanged, including after mutable
world positions change. It introduces no persistent bounds allocation and
does not reorder transparent draws. Actual archived bounds code plus the new
owner passes3,456 ARM checks, including a separated mesh at an internal run
boundary. Runtime coverage must determine whether this heuristic helps.

Full rendering parity, progression, sustained performance and hardware
endurance remain incomplete.

The spatial candidate source5da2d942f73c also completes the Old palace replay
and preserves the inspected scene. Coverage improves528 to584 resident draws
out of768, but packet calls increase144 to184. The same nine world-specific
functions total499760 ->502719 encounters; this subtotal excludes shared SDK
and memcpy work. It does not establish a submission improvement. The complete
window increases2.13% and includes35 lightning calls absent from the baseline,
so it cannot establish an attributable full-frame regression either. It is not
promoted. Its load-time spatial heuristic saves some resident memory but is
not the sought performance solution.

The next candidate, `world_leaf_selected_patch.py`, assigns each admitted
world draw its own persistent command leaf. It consumes the successful live
material selection already produced by the scene, after common pipeline
publication. The leaf checks immutable draw identity and patches only changed
live values. The scene no longer repeats selection in the packet consumer.
Per-draw frustum visibility is unchanged; no larger-group visibility decision
is required. Existing caps, same-epoch protection, explicit flushes and fences
remain. Unadmitted or already-used leaves retain the current native command
writer; this is not a GX renderer fallback.

Tradeoff: independently callable leaves require self-contained attribute and
layout state and more GPU calls. Less CPU copying does not guarantee less GPU
work. The replay must measure remaining copied draws and all submission costs;
physical command-fetch cost remains unproven. No larger memory cap is allowed.

`test_world_leaf_selected.py` passes464 ARM checks with the actual packet core
and owner. Its caller reproduces the scene selection/publication contract;
it is not an execution of the complete scene renderer. Wrong draw identity,
hidden/fog-rejected selection, epoch reuse, failed publication, reserve/cap
exhaustion, allocation failures and release are covered. The intermediate
unselected-leaf variant has461 checks and a build, but no gameplay runtime or
promotion; the direct-state variant supersedes it before another replay.

The first direct-state candidate failed its Old palace load before the measured
window: source132 texture allocation was rejected with1,567,104 linear bytes
free, below the15% reserve. Optional command storage had displaced a later
essential texture. This is a real admission-budget failure, not a timing test
or a missing-draw acceptance. The isolated process exited and user files were
preserved. That candidate remains undeployed. Revision2 caps world command
leaves at64KiB (parent measured peak64,688 bytes), rather than allowing the
previous128KiB ceiling. The per-admission15% reserve remains. The ARM ownership
and cap checks pass again with the smaller ceiling. This is not a proof of
full-game memory headroom; later areas still require validation.

Revision2 source81091d1a5410 completes the Old palace replay in104.51 host
seconds, with inspected capture1600, saves/settings preserved, logs drained
and owned process exited. No asset omission or texture allocation error occurs.
Observed heap41,859,696/50,331,648; linear free1,613,824/10,485,760; VRAM free
4,723,712; stack72,984/524,288; process69,595,136/83,886,080. All18 GPU programs,
game archive and Old80/New124 profiles remain unchanged. Runtime memory is
still tight, and this is not full-game peak validation.

This revision is also **not promoted**. The fixed cache fits496 of768 visible
world draws, leaving272 copied draws versus240 in the parent. Packet calls
rise144 to496. Named world functions cost499760 ->572392 ARM encounters,
and shared command-chain sealing/call bookkeeping also rises. The complete
window22527356 ->22501707 (-0.114%) includes different audio/particle activity;
it is not an attributable FPS gain. Eliminating duplicate selection works, but
the added leaf calls and reduced coverage outweigh that local saving here.

The accepted local candidate remains tick-split source4e9ce0e40ad2 and its
package3992cc5acc2393d9. None of these experiments was deployed, and no extra
hardware test was requested. Avoid another batch-size sweep: the evidence
shows that these world-only changes cannot close the full frame-time gap.
Use the preserved profiles to target larger remaining costs. All rejected
sources, build identity/restoration records and runtime failures are retained.
