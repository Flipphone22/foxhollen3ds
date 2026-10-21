# CPU-visible skinning integration

This candidate changes the remaining game-side vertex consumer, not PICA actor
rendering. It preserves the original transforms, reader joins and conditional
cache publication. The measured instruction reduction in the isolated routine
is not a whole-frame speedup; the earlier main-core profile attributed only
2.15% of encounters to CPU skinning.

The first shipping-app integration (`cpu-skin-game-v1`) completed the Old palace
replay but admitted no plans: zero prepared, zero hits, 595 misses. The original
CPU consumer remained active. This is an integration failure, not optimization
acceptance. Its parent also predates the Arwing stop-timestamp repair.

A second run with `model_resources.txt` present also admitted no plans. Direct
reads showed directory mode zero and zero registrations. The configuration
reader is inside `FH_EMULATOR_TEST`; shipping binaries do not execute it. The
inspected palace capture retained Krystal, creatures, world textures and HUD,
but cannot validate the unexecuted candidate path. Both runs are excluded from
timing comparisons because of observation and competing host activity.

The v2 isolated candidate explicitly initializes the resource directory and
inherits `audio-stop-boundary-v1`. It adds load-only rejection counters without
removing any containment, relocation, lifetime or metadata checks. The observer
must see at least one prepared plan and 100 uses in the actual shipping binary
before activation passes. Gameplay correctness, source-equivalence and total
frame cost remain separate gates. Directory maintenance must be included in
any cost comparison; it is not free infrastructure.

Pinned Melee command-cache publication was consulted; this change retains the
existing source-specific flush and reader-join rules, rather than introducing
an unconditional flush or GPU ownership change. The candidate is not deployed.

Evidence: `artifacts/native/cpu-skin-game-v1-palace-old/skin-activation.json`,
`artifacts/native/cpu-skin-game-v1-directory-palace-old/skin-activation.json`.
The directory experiment's retained configuration file is not effective in a
shipping binary and must not be prescribed as a console setting.

V2 (`5993c69bbe5e`) built with unchanged GPU programs, Old80/New124 profiles,
and verified root/archive restoration. Its generated mixer matches the tested
stop-boundary repair. The Old palace run completed, preserved saves/settings,
drained logs and exited; the inspected image retained the scene and actors.
Observed heap peak 41,366,136/49,283,072 bytes, linear free minimum
3,667,840/12,582,912, VRAM free 4,723,712, stack upper bound 73,008/524,288,
and process peak 70,758,400/83,886,080.

However, the bounded observer saw 66 resource registrations, five prepared
plans, four compilation declines, and zero uses across 470 fallback calls.
It explicitly failed activation. There were no file/buffer containment rejects;
116 loads had no registered instance binding (including possible unanimated
instances). These counts do not identify which prepared actors reached the
consumer, or whether their tokens were retired first. The next task is to trace
retirement versus caller mismatch, retaining all lifetime checks. Do not repeat
the disabled-directory experiment or call this candidate a speedup. Runtime
timings remain excluded, and the candidate remains undeployed.

The subsequent read-only v2 lifecycle capture resolves that ambiguity: a plan
became invalid between 25 and 30 registrations while both allocation owners
remained live and the escaped-owner counter was still zero. Later plans also
disappeared with live owners. The callback-escape hypothesis does not explain
that transition, so no callback exception was added.

The relocation hook called `fh_cpu_skin_metadata_write(NULL, 0)` for an
unskinned instance. The store intentionally treats a null pointer as an explicit
invalidate-all request. A first relocation with a null previous descriptor
pointer could likewise invalidate unrelated plans. V3 notifies only an actual
nonempty offset write or replacement of a nonnull previous descriptor range.
The store's explicit revoke-all behavior and all ownership checks are retained.

`test_skin_relocation_notifications.py` extracts the exact archived and current
hook bodies and executes them against the real plan store under ASan/UBSan.
Across 1,000 iterations the archived hook loses the unrelated plan; the fixed
hook retains it through unskinned and first-time unrelated loads, but still
rejects descriptor/offset mutations and explicit unknown writes. The complete
fixture reports 80,030 checks. This is a publication regression, not full-game
transform or performance acceptance. V3 must still pass observed activation.

V3 source `a0faf2cb1925` now passes observed activation: five prepared plans,
101 direct calls and zero fallbacks in the bounded observer window. Four other
loads declined compilation; this is not a claim that every model is admitted.
The actual shipping-app Old palace replay completed with saves/settings
unchanged, logs drained and owned exit. The palace capture was inspected:
Krystal, purple creatures, textured world and HUD remain visible without an
obvious new distortion. This is not full source-image or animation equivalence.

The shared 3dsx/CIA retains Old80/New124, unchanged shaders and the audio fix;
isolated root/archive restoration passed. Sampled heap 41,366,136/49,283,072,
linear free minimum 3,642,112/12,582,912, VRAM free 4,723,712, stack upper bound
73,008/524,288, process 70,758,400/83,886,080 bytes. Full-frame timing is excluded
because the read-only observer and foreign host activity perturbed execution.
No hardware request or deployment accompanies this milestone. The next gate is
inclusive whole-frame cost, followed by broader consumer/animation correctness;
the isolated routine's instruction saving is not sufficient acceptance.

The first inclusive eight-frame comparison rejects V3 as a performance win.
Main-core ARM encounters rose from 15,243,960 (uploaded audio-fix baseline) to
16,227,661 (+6.45%). Both windows contain eight object updates, 160 native actor
draw calls, 168 object renders, 24 scene-geometry calls and 72 collision-block
builds. Particle phases and diagnostic work differ; present entry counts are
17 versus 16, so no exact command-stream identity or FPS prediction is implied.
Both complete runs passed and their captures were inspected. Timing is excluded.

The actionable new cost is `fh_model_resources_escape`: 9,216 to 694,224
encounters, dominated by repeated linear owner searches. The new index candidate
uses a 256-entry, 512-byte table rebuilt on owner publication/removal. Escape,
forget and handle acquisition use bounded indexed lookup; all reader fences,
escape revocation and generation checks remain. There is no new heap allocation,
GPU work or per-frame table rebuild. The actual registry passes 5,120,512
indexed-versus-linear comparisons through deliberately colliding addresses,
replacement, escape, removal, disabled-mode changes and generation exhaustion.
The existing plan-retirement fixture also passes 70,039 checks under ASan/UBSan;
ARM compilation passes. Inclusive game performance remains the acceptance gate.

Evidence: `skin-frame-work-test/result.json`, `model-owner-index-test/result.json`,
and the two `cpu-skin-cost-v1-*-old-profile` runs. V3's build directory carries
`PERFORMANCE_NOT_ACCEPTED.json`. Do not deploy it based on activation alone.

V4 source `9ee56d3f7d2e` integrates the indexed directory. The exact tested owner
source hash is enforced by the builder. Its game archive is identical to V3,
shaders and Old80/New124 profiles are unchanged, and restoration passed.
The combined Old palace correctness/profile run completed with saves/settings
preserved, drained logs and owned exit; the inspected capture retains the scene.
Its sampled heap/VRAM/stack/process budgets match V3; linear minimum free is
3,642,112 bytes. No console update was made.

Indexed escape work fell to 49,008 encounters, but total main work is 15,179,409
versus baseline 15,243,960 (-0.42%). The unchanged particle functions account for
76,621 fewer encounters, exceeding the total 64,551 reduction. Therefore the
comparison does not establish a causal whole-frame gain. Scene/update/transform
call counts match, but phases and other work vary; this is neither an FPS
measurement nor proof the extra infrastructure pays for itself. V4 remains
unpromoted with `PERFORMANCE_UNPROVEN.json`. Use uploaded audio-stop-boundary-v1
as the parent for unrelated production work until there is stronger evidence.

The fresh baseline profile identifies particle update/draw as about 16.2% of
main-core instruction encounters. Do not repeat older sparse-slot/descriptor
optimizations: the actual archived adapter already includes them. The current
local generated source does not match the linked DWARF MD5
`98dcf788c855090b2249d0c087c4a999`; source-line attribution deliberately withholds
text. Reconstruct and verify that exact source before choosing the next change.

## Exact particle source recovered, 19 October

The archived generator now reconstructs the exact linked source above. See
`tools/reconstruct_current_particle_source.py` and
`tools/partition_verified_particle_work.py`; the source mismatch is resolved.
The partition reuses the existing eight-frame capture, not another game run.
Alpha lifetime is 0.471% and static UV publication 0.163% of main instruction
encounters; world position/bounds is 2.743%. These are neither cycle fractions
nor safely removable work. Do not repeat the rejected direct-member packet or
position outlining candidates. No game code changed or FPS gain established.
