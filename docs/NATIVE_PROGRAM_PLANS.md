# Prepared native shader configuration

16 October 2026. Local candidate; hardware performance is not established.

DSP audio remains the default on both models. This change reduces separate CPU
render-submission work: immutable shader configuration is prepared when a native
program is admitted, then copied into the existing command buffer on reuse.
It does not interpret GX, change geometry, or omit rendering features.

The preceding palace ARM capture contains 9,264 GPUCMD_Add entries and 184 SDK
shaderProgramConfigure entries. Direct branch attribution identifies repeated
configuration as one command-construction cost. Counts are instruction
encounters, not CPU cycles or an estimate of physical FPS.

`platform_program_plan.c` retains 64 direct-mapped configuration records, each
with a 64-word command capacity. Records consume 16,896 bytes of static normal
RAM, plus 12 bytes of counters. No linear RAM, VRAM, heap allocation, GPU
instruction, pass or worker is added. Collisions cause safe recompilation.

Preparation calls the installed SDK with both code-upload flags false into a
private CPU buffer, restoring the command pointer, capacity and offset afterward.
The cached packet is exactly those SDK bytes. Calls that upload vertex or geometry
code retain the original SDK ordering, including geometry-engine transitions.
The private record is never submitted as a GPU buffer; existing destination
command-buffer publication and retirement remain responsible for coherence.

Program initialization, free, shader setters and input-permutation setters
invalidate the record. Successful setters prepare its replacement. DVLE metadata
must remain immutable between setters. Native source inventory found no direct
named-field mutations or shaderInstanceFree calls; this is not a general pointer
alias proof. The existing single graphics owner remains required.

Pinned Melee `citro3d_fix.c` and `uniform_upload.h` informed packet bounds,
ordering and ownership. This adaptation caches only immutable program settings;
uniforms, raster state, code uploads and fences retain their existing owners.

Validation:

- Actual linked ARM libctru produces identical complete command buffers and
  globals in 1,312 cases / 4,835 checks, including all upload flag combinations,
  permutations, program reuse, eviction and teardown.
- Largest observed configuration is 46 words, below the 64-word capacity.
  One-word-short destination capacity is rejected before any write/cursor change.
- A stale-permutation mutation fails the same oracle.
- One synthetic no-code configuration uses 254 ARM instructions versus 1,291
  for the SDK. This is not a workload-weighted or whole-frame speedup.
- Linked C3Di_UpdateContext calls the wrapper. All used lifecycle wrappers are
  present; the unused permutation setter and its wrapper are both discarded.
  The standalone test still exercises that setter.
- Shared Old 80/New 124 MiB profiles, unchanged game archive, all 18 GPU programs,
  conversion recipe and isolated-build restoration are verified.

Reproduce with `test_program_plan.py`, `build_program_plan.py console-setup
--revision v2`, and `verify_program_plan.py`. Parent is archived
`unified-audio-raster-v1-console-setup-builds`; never build unrelated root WIP.
V1 failed a shutdown-marker assertion before modifying the root/archive. V2
preserves the existing shadow-image capture during shutdown. The initial linked
verifier incorrectly required an unused, discarded setter; it now proves both
the original and wrapper are absent instead of claiming they were linked.

Runtime evidence and deployment status are recorded separately. Full visual
parity, sustained target FPS, progression and endurance remain incomplete.

V2 source `c1640051d32a` passed the Old palace benchmark with DSP audio enabled:
307 rows, status1, logs drained, owned exit, saves/settings unchanged. The
captured palace shows Krystal, textured world, floating enemies and HUD.
No new complete visual or PCM parity claim. Source/callback audio faults and
curve shortages were zero. The 146.49-second host runtime includes DSP LLE and
profiling and must not be converted into console FPS.

Across the run, plans compiled 33 times and served 4,047 cache hits; 11,439
code-upload configurations retained the SDK path. In the eight-frame sample,
SDK Configure entries fell 184 to 136, with 184 wrapper entries. Whole sampled
ARM encounters fell 24,676,208 to 24,587,424 (-0.36%). Audio producer counts were
28 in each run, actor draws 280 each, but particle draws were 324 versus 300 and
RNG phases differed. This does not establish a causal whole-frame speedup.
The combined configuration/GPUCMD/all-memcpy bucket fell 2.62%; it includes
unrelated copies and is not an isolated configuration timing.

Observed Old heap peak 41,717,672/50,331,648 bytes, linear minimum free
1,755,776/10,485,760, VRAM free 4,723,712 and stack 72,984/524,288 match the
parent. Process commitment increased 16,384 bytes to 69,599,232/83,886,080.
These are sampled palace budgets, not moving-route or endurance validation.
Evidence: `program-plan-test/runtime.json` and the exact binary-bound run
`unified-audio-program-v2-benchmark-old-dsp-profile-v2`. No hardware deployment
or performance improvement has been established.
