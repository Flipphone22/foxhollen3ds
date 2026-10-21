16 October local candidate 9dc28d861d24: DSP audio is the default on Old/New,
with CPU comparison selectable in the existing startup menu. Includes retained
collision queries and fused raster-cache publication. Shared Old80/New124 CIA
and 3dsx; conversion recipe unchanged. Old palace runtime passes, memory bounds
unchanged; no new physical FPS gain established. Not deployed: both FTP checks
timed out before login. Particle batching remains a separate local experiment.

Build 5f50dd9e7bc7 retains DSP-default audio and the startup CPU option.
It also reuses filtered collision triangle lists instead of rescanning all
triangle masks on repeated queries. Geometry, live collision checks and output
order remain unchanged. Added static normal RAM: 16,592 bytes; no added linear
RAM, VRAM, worker or GPU work. Old palace replay passed; collision ARM work
fell 63.9%, total sampled ARM work 4.23% versus the DSP baseline. These are
instruction encounters, not hardware frame times or an FPS prediction.
Deployment is recorded separately. No asset conversion or save changes needed.

DSP audio default (build ee35300a85c5, 16 October)

The unified startup app defaults to Audio: DSP for ordinary gameplay and
benchmarks on both models. Select the Audio row to switch to CPU for this
launch. Benchmark Start also toggles audio; cancelling keeps the choice visible
on the startup screen. Selection locks when audio starts; relaunch defaults
to DSP. No graphics, save, asset or persistent settings changes are required.

For comparison, run Benchmarks / playback -> 00-resident-palace once per
audio mode with identical graphics. Start toggles DSP/CPU, A launches, and the
run exits automatically. Reopen ftpd afterward. Install the CIA through FBI
to update the HOME icon; copying it to SD does not install it.

Prior measured ARM work reduction was 13.1% in one sampled palace window,
not a hardware FPS prediction. Small audible differences are acceptable;
report dropouts, gross level errors or broken timing. New deployment status
is recorded separately. Historical CPU-default instructions below apply only
to the older build named there.

Experimental CPU/DSP audio comparison (build047a9491e50b, 16 October)

Use the ordinary Foxhollow startup app, not a second replay installation.
Install SD:/3ds/foxhollow/foxhollow.cia through FBI to update the HOME icon.
Choose Benchmarks / playback, then 00-resident-palace. Keep visuals PICA ACTORS
+ HUD, renderer MAIN, texture VRAM OFF and GPU profiling OFF for both runs.
Run once with Start: audio CPU (reference). Relaunch, choose the same case,
press Start once to select DSP (experimental), then A to run. Both runs exit
automatically and keep normal saves separate. Reopen ftpd after both runs.
Normal Play and a cancelled benchmark default to CPU; audio cannot hot-switch.

Please assess obvious dropouts, gross level differences or broken sound timing;
small resampling/reverb differences are acceptable. The sampled ARM work falls
13.1% in one eight-frame palace comparison, but this is NOT a physical FPS
prediction. Voice/RNG phases and producer block counts differ slightly. The
reference mode retains native sample storage, so its memory is not identical
to the older CPU-only installation. Neither mode establishes complete audio or
rendering parity, all-area stability, or the required hardware frame rate.

# Old 3DS Fast effects

Build 30ea42e7a564 changes the existing o3ds-fast preset from mask
33570817 to 16391. Select that preset again in the startup menu: previously
saved choices are preserved and are now shown as Custom.

- Dynamic vertex lighting and environment lighting are disabled. Existing
  source adapters skip object-light selection and environment snapshots at
  their producer boundaries, before uniform or GPU publication.
- Weather is disabled. Its CPU quad trigonometry and camera/matrix preparation
  now stop when WEATHER or RENDERING is off, for any preset/hardware.
- Weather angles, flake motion, RNG, lightning state and simulation continue.
  Turning weather back on regenerates display geometry from live state.
- Shadows, fur, diffuse/specular maps and optional post effects remain off.
- World/actor textures, HUD, sky, movie, morphs, skeletal animation and combat
  particles remain available. Morph computation shared with collision is not
  claimed to disappear; this change does not remove gameplay work.

The other presets are unchanged. There is still one shared 3DSX and CIA,
with Old80/New124 memory profiles. Converter recipes and prepared assets are
unchanged; no reconversion is needed. Install the CIA with FBI to update the
HOME Menu application.

Follow-up build fa18b1a85a42 additionally stops per-particle opacity, scale,
camera-facing aim and parameter preparation when PARTICLES or RENDERING is
off. The original pool/slot eligibility checks, shared random-number calls
and pending-resource cleanup stay in order. Default Old Fast still enables
combat particles; this fixes the existing manual OFF option.

The actual native draw function passes 30,000 host ASan/UBSan comparisons:
same slots, RNG sequence, visible parameter packets and cleanup. Mutants
omitting RNG or skipping cleanup are rejected. Other generated source bytes
are unchanged; the tested function matches the generated ARM function.
Evidence: glow-off-work-v5-test, including compiled-source-proof.json.
The two added counters total eight static bytes. No shader, buffer, allocation
or worker was added. Physical speedup is not established.

The exact follow-up Old80 CIA completed a particle-OFF intro/title/Select run
in 59.69 host seconds: 101,073 skipped preparations, 8,375 retained RNG calls,
zero particle draws, normal actor/material rendering and clean log drain.
The title capture was inspected. Observed RAM bounds match the preceding
check. This is no claim of full progression or a paired performance result.
See glow-off-work-v1-console-setup-builds/acceptance.json.

The Old Fast/weather candidate changes two source files from 22e57bb425dd;
the particle follow-up changes two more. Both are isolated from unrelated
installer/renderer work. All 17 GPU programs are unchanged.
Pinned Melee early_queue.c was consulted: command/resource retirement stays
with existing owners; a disabled visual does not permit early resource reuse.
No new buffer, worker, allocation or GPU program was introduced.

Validation:

- old-fast-effects-v2-test: actual source shape functions, 6,000 visible/
  hidden transitions, exact live angle state and visible geometry, no hidden
  camera/trigonometry calls. The test keeps hidden geometry stale to exercise
  restoration. Flake-motion code is unchanged.
- compiled-source-proof.json: tested weather functions match actual generated
  ARM game functions. ARM game compile passed.
- old-fast-effects-v1-console-setup-builds/acceptance.json: exact Old80 CIA
  content selects the new preset through HID, plays the intro/title, and
  exits with Select with logs drained. Captures inspected. Environment
  updates, bindings and publications are all zero.
- Observed normal heap 39,051,680/50,331,648 bytes; linear free minimum
  3,833,216/10,485,760; VRAM free 4,723,712; stack upper bound 72,936/524,288;
  process 68,886,528/83,886,080. This short title route is not a whole-game
  memory bound.

The initial runtime harness incorrectly expected a startup option message in
the gameplay log. The app completed normally. Preserved evidence was checked
using the actual checksummed settings written by the menu; no second boot was
needed. The failed harness record is retained.

No matching physical FPS comparison or weather-scene runtime comparison has
been established. Other compilation was running on the host, so this run is
correctness evidence, not an attributable speedup. Deployment is recorded
separately; a completed local build is not a console installation.
# 15 October palace memory follow-up

Current user policy: **Lighting, Environment and Specular ON in all four named
presets**. Normal e61d43cdbad0 implements this over the palace repair. Previous
Old Fast lighting exclusions below are historical. Weather and ground shadows
remain off in Old Fast; other effect/geometry/texture choices are unchanged.
Saved Custom choices are preserved; reselect a named preset to adopt the policy.

Shared normal build e3052ddc285f retains the Fast/particle changes and adds a
grant-checked Old80 48/12 MiB heap profile. It addresses the captured palace
texture allocation rejection while preserving process and linear headroom.
See [palace allocation evidence](OLD_PALACE_LINEAR_PRESSURE.md). No settings or
prepared assets change, and physical hallway completion is not yet established.
