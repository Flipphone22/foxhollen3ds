# Early native particle visibility

The accepted producer fully prepares each static particle's alpha, orientation
and material parameters before the native stream commit rejects offscreen
particles. The new producer consumes the original scale RNG, computes the same
center/scale/shape and calls the existing conservative visibility predicate
before the remaining pure appearance work. Retained static instances then use
a prevalidated commit, avoiding a second frustum test. Dynamic trail/locked
shapes retain their original preparation and culling path. Unsupported material
requests retain their original accounting.

The scalar visibility equation, template bounds, 6.25% margin, one-unit padding
and four side planes are unchanged. No near/far clipping is added. No temporal
visibility cache, geometry reduction, new shader or extra ring buffer is used.
Original simulation, pool update, slot contents and random-call order remain.
Alpha arithmetic itself is unchanged; only its order relative to independent
scale calculation changes. Floating exception sticky flags are not part of
the game-state equivalence claim.

The original pinned Melee command-cache publication rules were reviewed.
Borrowed ring ranges still commit before texture lookup, material changes,
dynamic draws or end-of-pool. The new early test only reads current camera and
template state; it does not publish, allocate, retire or change a GPU owner.
The prevalidated API requires that all static particles in that lease have
passed the predicate for the current `begin()` camera/template state. The
general commit retains its original test for other callers.

`particle_early_visibility_patch.py` applies only to the accepted archived
tick-split source4e9ce0e40ad2. It modifies the glow owner/header and source
adapter. The builder recompiles the original game particle unit and verifies
the complete generated source against the tested file; all other game code
and all GPU programs must remain unchanged.

`test_particle_early_visibility.py` compares actual old/new producer bodies
under ASan/UBSan, with simulated renderer entry points: 30,000 comparisons
preserve ordered visible packets, dynamic corners, RNG state, slots and cleanup.
Random positions and sizes exercise offscreen cases. A separate test executes
the actual stream owner with simulated fences: both versions emit145,256
particles with identical ordered content and accounting digest. False-visible,
false-culled and omitted-RNG mutants fail. The source visibility function is
byte-identical, so its existing shader/bounds evidence is reused explicitly.

These tests do not establish hardware behavior, complete visual parity or an
FPS gain. Runtime cost must include the new early call and prevalidated commit;
saved work in `drawGlow` alone is insufficient. The performance, progression,
save/reload and full rendering goals remain incomplete.

## First integration and revised producer

The first shared build (source `907267106d68`) completed the Old 80 MiB palace
route, preserved saves/settings and exited with drained logs. Its inspected
capture retains the palace, Krystal, purple creatures, particles and HUD.
This is correctness evidence, not full visual parity. All 18 GPU programs and
the setup recipe are unchanged; only the game particle object changed in the
game archive. The New 124 MiB profile is retained, not newly runtime tested.

The eight-frame instruction sample grew from 22,527,356 to 22,595,663 (+0.30%).
Particle and audio phases differ, so this is not an attributable full-frame
regression or improvement. The relevant named particle functions grew from
1,461,398 to 1,659,016 encounters: the new predicate and wrapper introduce two
calls and a temporary instance per static particle. Removing work from the
late commit therefore did not establish a net saving. This build is not
promoted or deployed.

`particle_inline_visibility_patch.py` instead obtains a read-only camera/bounds
view once per pool and inlines the identical predicate into the producer.
Accounting is accumulated locally and published once at pool end. It removes
the per-particle temporary and ABI calls; the existing lease, dirty-range and
GPU retirement owners remain unchanged. The context is valid only inside its
begin/end pool. There is no added persistent allocation, GPU pass or shader.

The revised actual producer passes 30,000 source comparisons; the actual stream
owner retains the same ordered digest for 145,256 emitted particles. Deliberate
false-visible, false-culled and omitted-RNG versions are rejected. Tests use
simulated SDK/fences, not hardware timing. The performance comparison must count
the predicate now embedded in `drawGlow` plus the pool context/accounting calls.

## Revised integration result

Source `cacc8037a941` completed the Old palace route in 112.77 host seconds,
with state checks, unchanged saves/settings, DSP LLE, drained logs and owned
process exit. Capture 1600 was inspected. All 18 GPU programs, the setup recipe
and Old80/New124 profiles remain unchanged; only `expgfx.o` changed in the game
archive. Build isolation/restoration checks pass. No console deployment occurred.

The revised named particle subtotal is 1,424,300 versus the parent's 1,461,398
encounters (-2.54%), including `drawGlow`, stream begin/commit, context and
accounting. It is 14.15% below the unsuccessful first experiment's subtotal,
but each run has different particle phases; these are not identical-workload
cycle measurements. Whole-window encounters are 22,045,709 versus 22,527,356
(-2.14%). Audio jobs fell 28 to 27 and voice-curve calls 1,706 to 1,485, so the
whole decline cannot be credited to this producer. It is not an FPS claim.
Whole-run particles supplied/culled were 286,246/57,359; emitted quads 228,887,
with 5,961 particle draws. No unsupported material/shape or CPU vertices were
reported on this route. Higher draw counts than the parent must not be hidden
behind the instruction subtotal.

Heap peak remains 41,859,104/50,331,648 bytes; minimum linear free 1,622,784 of
10,485,760; initialized VRAM free 4,723,712; sampled main stack 72,984/524,288.
Process commitment is 69,599,232/83,886,080, one 4 KiB code page above the parent.
These are this route's observations, not full-game worst-case headroom.

The candidate is retained for integration, not promoted as an established
performance improvement or deployed. The accepted packaged baseline remains
`4e9ce0e40ad2`. A further unchanged palace boot would not resolve phase-dependent
cost attribution; subsequent work should target larger remaining costs and use
the existing profile before adding more particle micro-optimizations.
