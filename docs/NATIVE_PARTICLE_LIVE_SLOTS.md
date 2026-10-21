# Live particle slot traversal

The native particle producer still scanned all 25 slots in each visible pool,
reading each slot's table index and associated material/source pointers before
checking its active bit. The replacement visits only active slots, in ascending
source order. It does not cache the active mask across particle processing:
later slots can change during a resource callback. A remaining-slot mask prevents
revisiting earlier positions, and bits 25 through 31 never name particle slots.

This changes the producer's traversal, not simulation or the native renderer's
resource owners. Alpha, scale, random-number calls, camera-facing angles, source
phase addresses, packet layout, dynamic corners, resource retirement and stream
publication remain unchanged. No CPU geometry, allocation, worker, GPU command,
shader instruction or buffer is added. Pinned Melee command-cache publication
rules were reviewed; existing explicit flushes and fences remain the owners.

The original and replacement generated functions pass 90,000 host comparisons
under ASan/UBSan with stub native/camera calls. Ordered packet data, RNG state,
slots, masks and cleanup agree. Tests include mutations of future and earlier
mask bits at RNG/page callbacks, no active slots, full pools, slot 24, and padding
bits. Four deliberately incorrect variants fail: snapshotting the mask, using
padding bits, dropping slot 24, and omitting randomness. The fixture allocates
valid canary slots beyond the pool so padding mistakes produce observable extra
draws; an earlier out-of-bounds-only mutation check did not reliably reject that
variant and was replaced. This is not physical GPU or performance evidence.

Build with `tools/build_particle_live_slots.py console-setup --revision v1` from
the accepted `unified-aux-chunks-v1-console-setup-builds` snapshot. The builder
changes only the particle generator, recompiles the actual game object, checks
the complete generated translation unit against the tested hash, preserves
all shaders and Old80/New124 profiles, and restores the unrelated root work.
The bounded normal-app benchmark is `tools/run_particle_live_slots.py benchmark
--family unified-particle-live --revision v1 --old --profile`. Profile counts
are workload evidence, not elapsed CPU time or predicted console FPS.

The separate GPU lifetime-alpha prototype remains disconnected. Its original-C
comparison found 25 errors larger than one byte in 400,000 cases, including
255-to-zero wrap differences. It must not be substituted for the CPU expression
to manufacture a performance result.

## Application evidence

Candidate `630d6d29d395` passes the Old80 normal-app palace benchmark with native
DSP LLE, unchanged saves/settings, completed state checks and drained shutdown.
The inspected frame 1602 retains palace geometry/textures, Krystal, the purple
creatures, particle glow, fire and HUD. Only `expgfx.o` changes among the game
archive's ordered members. All GPU programs and the install recipe are unchanged.

The eight-frame profile reports drawGlow 1,038,105 -> 990,181 instruction
encounters, but pool calls also change 312 -> 309 and particle activity differs.
Whole-frame encounters are 21,912,036 -> 21,828,847 (-0.38%), including unrelated
audio worker activity. This does not establish an attributable full-frame or
console FPS gain. It is not the large improvement still required.

Measured budgets match the parent: heap 41,859,104/50,331,648 bytes, minimum
linear free 1,622,784/10,485,760, VRAM free 4,723,712, sampled main stack
72,984/524,288, process commitment 69,599,232/83,886,080. Host run duration
84.95 seconds is diagnostic elapsed time, not an FPS measurement.

Keep accepted `92fd26aa4763` as the deployment baseline. Candidate `630d6d29d395`
is retained locally with its evidence; it has not been packaged or deployed.
Do not request another hardware run just to resolve this small traversal change.
