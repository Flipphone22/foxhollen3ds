# Morph and fur regressions reported on hardware

The user's New 3DS run, collected read-only in
`artifacts/console-visual/20261010-135222-223`, ends cleanly at retrace 9016.
It uses original geometry and original-resolution textures, and records 2338
morph updates and 3795 fur draws. This establishes startup recovery for that
run, **not visual correctness**. The user reports widespread model distortion
and flickering that disappears with MORPHS off, plus fur flicker and eye issues.

Three defects are corrected locally:

* The original unchanged-vertex morph copy uses a `u32` access at a six-byte
  stride. Every odd record is only halfword aligned. The ARM adapter now uses
  halfword copies, preserving the exact six bytes. The exact linked binaries
  were executed under Unicorn with a memory-access alignment trace: 32
  unaligned accesses before, zero after across two source/destination offsets.
  This proves removal of the invalid alignment assumption; Unicorn's tolerant
  output does not prove that this was the console's corruption mechanism.
* When the game selects base vertices because no blend channel produced a
  stream, the native owner now selects resident base geometry too. Previously
  it kept drawing the last published expression. Resetting this selection
  neither overwrites pending GPU readers nor allocates/copies another buffer.
* First-visible fur now requests ordinary world-space pose preparation through
  a native-only pass value 16. Pass 4 selected the source's fur preparation,
  which can leave streamed joints in model space until a later original render
  section that the native backend correctly no longer executes. Native shell
  expansion remains on the GPU. Original animation and hit-volume work remains.

A separate load-compiler correction applies raster lighting after composition
of two pre-light texture layers when a secondary light texture exists without
the primary environment texture. The source inventory has five such material
declarations; this is not proof of all eye-material coverage. Layered secondary
specular remains unsupported. There is no new rendering pass or shader code.

Pinned Melee's `matrix_vertex_pointer`, explicit cache publication and retained
material-state invalidation were reviewed. The existing padded morph fetch
layout, fenced position slots and SDK command ownership are retained. The
hardware register layout documentation also confirms the eight-byte padding
component encoding ([3dbrew](https://3dbrew.org/wiki/GPU/Internal_Registers)).
No CPU skinning or retired rendering interpreter is reintroduced.

Validation:

* Actual producer versus fixed-point source: 144000 coordinate checks, plus
  byte comparisons of original and corrected unchanged-vertex copies.
* Actual morph owner: delayed GPU readers, slot reuse, channel reset/resume,
  toggle continuity and owner retirement under ASan/UBSan. Missing-retirement
  mutations are rejected. Static owner metadata grows by 256 bytes; position
  buffers and their existing limits are unchanged.
* First-visible fur parent/world ownership and the generated dispatch boundary:
  1000 first-pass and 1000 already-prepared cases pass. This is not a complete
  source animation integration oracle.
* Material compiler: 1920 RGB cases covering environment and raster-only
  deferred lighting; both old-order mutations rejected; alpha preserved.
* Original-asset cockpit replay with morphs, fur and specular enabled together:
  New baseline/candidate both complete state checks, preserve saves, drain
  logs and exit. Old candidate also passes the custom vertex-layout audit;
  its instrumented timing is excluded from performance comparisons.

New Azahar baseline/candidate scene preparation is 0.77240/0.76958 ms and native
submission 1.94114/1.93807 ms. Both are paced at about 59.831 FPS; animation
phases differ slightly. This establishes no meaningful timing regression in
this short scene and no physical-console speedup. Heap peak 47,854,184 of
71,303,168 bytes; linear minimum free 18,625,280 of 25,165,824. Old heap peak
40,514,152 of 50,331,648; linear minimum free 3,945,216 of 10,485,760. Both retain
4,846,592 bytes free VRAM and use a measured 72,844 of 524,288 stack bytes.

Images at retrace 900 were inspected for all three valid runs. They retain
Fox, the cockpit, eyes and menu. They do not reproduce or disprove the reported
hardware-wide flicker, and residual face lighting remains visibly imperfect.
Do not call morph/fur/eye fidelity accepted without console confirmation.

Evidence is under `morph-fur-repair-test`, `morph-original-audit`,
`actor-layer-lighting-test`, and the corresponding `artifacts/native` runs.
The initial cockpit test recipe placed profile directives after input; parsing
correctly rejected it. That owned run was interrupted and cleaned up, its
failure retained, and the corrected v2 recipe supplied the valid comparisons.

Deployment: New223 manifest `20261010-140802-223` is complete; normal `70b95ca077de`/CIA `aa4544a21f75`, replay `b6a6e70f48b5`/CIA `2c9bbd2e689c`. All changed payloads byte-readback verified. Saves and preferences preserved; Old229 unchanged. Hardware visual confirmation remains pending.
