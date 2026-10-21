# Bounded spatial audio control delivery

Candidate: `6d78487cce32`, archived in
`artifacts/research/pica-scene-rewrite/unified-audio-controls-v1-console-setup-builds`.
This is not an established hardware speedup or an accepted general release.

The 17 October Old 3DS CORE 1 palace run measured 19.75 FPS / 50.64 ms per
frame. Its exclusive main audio-update scope was 6.45 ms; that scope includes
lock waits and scheduling, so it is not 6.45 ms of arithmetic that can simply
be removed. CORE 0 measured 15.24 FPS. Earlier CORE 1 hardware already measured
19.57 FPS, so these results are not a 30% improvement over that earlier release.
The comparison is recorded in `old-audio-core-test/hardware-comparison.json`.

## Change and ownership

Only `audio_sfx.o` changes in the 794-object game archive. Spatial sound maths,
voice handles, position tracking and original control values are preserved.
Three value-only commands (pan, surround and volume) are published together in
an eight-byte packet. A volume-only branch uses the same representation.
The ring holds 64 packets (512 bytes), with bounded backpressure and no drops.
It neither retains game-object pointers nor creates a new worker.

The existing Old CORE 1 mixer consumes packets under the existing recursive
synth lock. Other synchronous synth API work drains older packets at outer
lock acquisition; recursive calls do not introduce mid-block updates.
Calls already holding that lock, calls from another producer, CORE 0 and New
3DS use synchronous control delivery. Lock-owner bookkeeping remains present
on those paths and needs a matching runtime regression check.

Shutdown disables new queue publication, joins the worker, then drains retained
commands through the existing owner. The pinned Melee audio ownership pattern
was consulted; this is an adaptation for Foxhollow's MusyX recursive API, not
a copied Melee mixer. Source timing, DSP source clocks and sample synthesis are
unchanged. No shader, draw, geometry buffer or GPU submission change is made.

## Evidence and remaining gates

`audio-controls-test/result.json` records actual queue/owner-wrapper threaded
ASan/UBSan checks: ordering, recursion, wraparound, forced full queue, barriers
and shutdown. 100,226 packets passed; fault mutants reject dropped controls,
swapped values and missing barriers. This does not prove full MusyX fidelity or
console scheduling. `game-object-proof.json` verifies the sole changed game
object and the generated source against the tested adaptation. All 18 GPU
programs, the preparation recipe and Old 80 / New 124 MiB CIA profiles match
the parent. `restored.json` verifies restoration of unrelated workspace inputs.

The first exact-binary palace run (`...core1-profile-v2`) reached gameplay but
exceeded its 240-second host bound. It was cleaned up, and its failure is
retained. Other emulator processes were observed; no timing comparison is
valid. Early queue observations were made before the measured palace window
and were zero. These samples do not establish that the gameplay path is unused.
The corrected observer targets rendered frames 1200 onward, beside the existing
1300..1307 instruction-profile window. A bounded 480-second retry preserves
the original game, input and profiling work. Placement must be checked using
actual executed blocks because this emulator's processor-ID SVC is a stub.

The CORE 1 retry completed game execution, rendering coverage and drained
shutdown in 210.19 host seconds. Eight measured-window observations contained
207..505 queued controls, with consumption keeping up (at most two pending at
those snapshots), and zero observed full-queue backpressure. The earlier zero
counts were loading-window samples, not a queue implementation failure.
Normal heap peaked at 41,364,432 / 49,283,072 bytes, linear free stayed at least
3,591,424 / 12,582,912 bytes, initialized VRAM free at 4,723,712 bytes, and main
stack at 72,984 / 524,288 bytes. Process commitment increased by one 4 KiB page
to 70,647,808 / 83,886,080 bytes. These are sampled bounds, not exhaustive worst
cases. Raw status retains the emulator's processor-ID telemetry failure; the
separate executed-block audit is required before accepting actual placement.
The palace image retains Krystal, world textures and HUD; static bottom-screen
controls are absent in this capture, consistent with the unresolved parent
limitation. No new visual parity or audio-fidelity claim follows from this run.

Deployment requires completed CORE 0 and CORE 1 replay checks, exact binary
identity, unchanged saves, draw coverage, observed use and consumption of the
queue, and explicit retention of known UI limitations. Hardware testing uses
the existing application and startup menu, not another similarly named app.
No assets, saves or graphics preferences need replacement for this change.

Both CORE 0 and CORE 1 checks are now complete (`...core{0,1}-profile-v3`).
CORE 0 finished in 154.96 host seconds; these durations are diagnostic only,
not comparative emulator performance measurements. Both completed status 1,
reported zero attempted missing actor draws, preserved saves/settings and
retired queued GPU work. Independent per-core executed-block audits verify
the requested mixer placement in each case. CORE 0's palace image includes
Krystal, the purple enemies, world textures, HUD and static bottom controls.
The CORE 1 capture has a different camera phase and missing static bottom
controls; these are not pixel-matched visual or performance acceptance runs.

`hardware-test-review.json` binds these checks to the exact CXI/ELF and preserves
the rejection for general release. Package `e484f826a62ae01f` is limited to the
requested Old 3DS palace experiment. Installation is through FBI using the
existing `SD:/3ds/foxhollow/foxhollow.cia`; uploading alone does not replace the
installed HOME icon. Select DSP and CORE 1 (30%) on startup, then run the existing
palace benchmark once. CORE 0 is still the startup default. Compare the resulting
full-frame windows with the retained physical CORE 1 baseline, including workload
counts and audio underruns, before attributing any improvement to the queue.


Old CORE 1 default (17 October)

The shared application now has an isolated default-only derivative builder,
`tools/build_old_core_default.py`. It changes the startup selector to CORE 1;
CORE 0 remains selectable before the audio selection is latched. New hardware
continues to use its existing worker placement. The actual selector host test
passes default, manual selection, invalid selection and latch checks.
The 30% label describes the requested system-core scheduling allowance, not
clock speed or measured utilization. The previous hardware comparison was
15.24 FPS on CORE 0 versus 19.75 FPS on CORE 1 in its short palace window.
This default-only change is not an additional speedup. Deployment of this
derivative must be recorded separately; the previous upload remains 6d78487cce32.


Hardware confirmation and default-only derivative

The physical 6d78487cce32 CORE 1/30% palace run completed normally. Compared
with 094e03b401a2 on the same core, case, save CRC and graphics mask, the
approximately five-second window improved from 19.7485 to 23.0011 FPS and
50.6368 to 43.4762 ms/frame. Main audio update fell from 6.4524 to 0.09936 ms.
P95 observation fell from 65.8699 to 47.8250 ms; no sampled PCM underruns.
Draws/frame were 162.82 versus 161.99, indices/frame 72251 versus 72531:
similar workload, not identical animation state. This is not sustained-play or
full audio-fidelity validation. The game still needs 10.14 ms/frame removed
for a 30 FPS average in this scene. Normal peak 41,364,432 bytes, minimum linear
free 3,591,424 bytes; process 70,647,808 / 83,886,080 bytes.

The c8acfac0d348 derivative changes only the startup old_worker_core initializer.
Actual Old/New menu, selection, latch and mocked grant/failure paths pass
ASan/UBSan; a removed grant guard mutant fails. The linked ELF initializer is
also checked as 1. Game archive, shaders, memory profiles and setup recipe are
unchanged. Parent CORE0/CORE1 runtime records are explicitly inherited, not
reported as new executions of this derivative. The limited hardware package
retains known rendering/UI issues and the general-release rejection.

Existing eight-frame CORE0 instruction encounter data were reused to inspect
particle update and actor command append; these are not physical CPU cycles.
The generated particle source matches DWARF MD5 e220bcc2b995d0ce5e6cc89e0253b919.
Its update cost is distributed among lifetime, physics, color and bounds work;
this does not justify deleting simulation or claiming one small branch will
close the remaining frame-time gap. Actor source line text lacks a DWARF MD5
and was deliberately left unverified by the analyzer. No new rendering
optimization is included in the default-only build.

Deployment completed on Old229: c8acfac0d348, transfer20261017-160303. All six staged payloads byte-verified and published; final manifest verified. Install SD:/3ds/foxhollow/foxhollow.cia through FBI. No additional default-only benchmark required. New223 untouched.
