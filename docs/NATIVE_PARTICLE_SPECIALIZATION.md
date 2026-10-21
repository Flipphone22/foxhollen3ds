# Particle specialization from live behavior evidence

Shipping Old4f58's retained eight-update ARM profile attributes 16.16% of main
instruction encounters to particle update and drawing. This is not a cycle or
wall-time share. The existing native templates, active traversal and batches
are already present; removing them again is not a new optimization.

`capture_particle_behaviors.py` reads active pool ownership and slot bytes from
the unchanged shipping executable. The reader verifies the ELF's DWARF layout,
process identity and heap mapping using the established observer, then requires
pool pointers, masks, all sampled slot bytes and the retrace to remain stable
across double reads. No pause, guest writes or input changes are used. Decoder
checks cover all 25 slot positions, empty/full masks and four malformed cases.

The headless Old palace run `particle-behaviors-v1-old` completed normally,
preserved saves/settings and drained logs. Its capture was inspected: Krystal,
textured palace, purple enemies/particles and HUD remain. Four coherent samples
at retraces 1100..1192 contain 2,484 resident active-slot observations. All 151
sampled pools have homogeneous behavior/render flag combinations. This does not
prove an invariant across callbacks, spawning, other areas or future frames.

1,998 observations (80.43%) satisfy `(behavior & 0x20005) == 1`: source-world
translation is zero, attached rotation is disabled, and copy-source translation
is disabled. These are residence observations, not counted update visits. The
run's 113.99 host seconds and displayed FPS are not performance evidence because
of observation and competing workloads. No physical FPS result was obtained.

`particle_direct_position.py` implements an inline specialization for this
condition. It retains table lookup, scratch outputs, original `0 + position`
arithmetic, published position and bounds checks. Copying position directly
would change signed-zero behavior. The original block handles other cases.
No allocation, thread, GPU change, simulation omission or changed RNG call is
introduced. The previous outlined-function candidate remains rejected/separate.

The complete original translation unit MD5 is
`98dcf788c855090b2249d0c087c4a999`. The host oracle passes 8,192 slot, bounds,
scratch and external-call comparisons. Its helper is a test wrapper only;
production specialization remains inline. Actual ARM compilation against archived
native headers succeeds: update code grows 9,492 to 9,592 bytes. That is not a
speedup measurement. Candidate SHA256 is
`812ae6789a0214a82ebcc07aa3984316742692841f84693c507610b9429e3aa8`.
It is not integrated, built as an app or deployed. ARM runtime cost remains open.

An initial compiler-helper path replacement missed its destination and wrote
the isolated older outline experiment's compile artifacts. The destination was
corrected, the original outline compiler rerun to reconstruct its original
candidate (`b594e50b8e1d88e3a0012f177da7d0eb370eafe88a414dfd3d0f7cd56da72be4`),
then the new specialization compiled into its own directory. Neither working
game sources nor game archive were changed. The mistaken compile report is kept
as `particle-direct-position-test/initial-misdirected-compile.json`.

Next gates: measure actual complete-function ARM cost including the added branch
and uncommon-path cost. Investigate pool-level dispatch only after establishing
the producer/mutation contract; observed homogeneous pools alone do not permit
assuming immutable flags. Preserve the source snapshot/callback semantics.

## Complete-function fixture and isolated application

`test_particle_direct_position_arm.py` links the actual compiled update objects
and executes the entire update function on ARM11 in 27 controlled cases: nine
behavior/render combinations at occupancies 1, 8 and 25. The compiled function's
own instructions are counted; external dependencies are explicitly modeled,
not silently accepted. Slot bytes, runtime pools, phase globals, RNG state and
external event order match. A missing `vecRotateZXY` model initially halted the
test; the added deterministic model preserves call effects and is not claimed
to implement original trigonometry. The host oracle separately covers source
objects, attachment transforms and scratch state. Copy/rotation guard mutants
are rejected by `test_particle_direct_position_mutants.py`.

For full pools of the four common eligible observed types, update-function
counts fall 9,366 to 8,640; 8,466 to 7,665; 8,626 to 7,850; and 9,566 to 8,815
(7.75–9.46%). The sampled noneligible type rises 8,541 to 8,715 (+2.04%).
Other fixtures cover guard failures and small pools. These are synthetic initial
states using observed flag combinations, not captured complete game states,
inclusive external-call timings, a frame workload or a hardware FPS estimate.

Shared local normal build **0dc0f5fac88a** integrates this guarded path against
shipping 4f58. Exactly `tools/arm_overrides.py` and the generated `expgfx.o`
payload change; all other game object payloads and GPU programs are unchanged.
Old80/New124 CIA profiles, unchanged installation recipe, exact generated source
and selected workspace/archive restoration pass. 3dsx SHA256:
`76bcdeb24458fbe567e88305ddbf19bbaf162b3c4ee71795dc779c3942b677cf`.
The build adds no buffer or static cache. Full-game execution and memory peaks
have not been remeasured for it; do not claim the earlier observer run tested it.
No deployment or hardware request was made.

Build with `build_particle_direct_position.py console-setup --revision v1`.
Evidence is in `particle-direct-position-test/{arm-update,mutants}.json` and
`particle-direct-position-v1-console-setup-builds/{scope,verification,restored}.json`.
Batch its game validation with the next meaningful implementation step rather
than repeatedly booting solely to resolve a small, variable frame-count delta.
