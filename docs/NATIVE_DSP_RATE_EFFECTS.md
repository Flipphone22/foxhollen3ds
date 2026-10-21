# Native DSP-rate audio effects

16 October 2026. Work in progress; not deployed or physically benchmarked.

The accepted DSP-default application still resamples each DSP effects block to
32 kHz, combines software-generated sends, runs the source reverb, then resamples
back to the device clock. This candidate removes that round trip. Dry voice
playback, source simulation, music and dialogue scheduling keep their clocks.
The existing two-device-sample wet/dry alignment remains in place.

The replacement uses the actual returned PCM provenance to split a device
block at effect boundaries. Integer boundary calculations are equivalent to
evaluating the rational source clock for each of the 160 device samples, without
per-sample division. Source lookahead must be tagged. Missing or mismatched
epochs, skipped windows, changed bus modes and nonzero unmapped sends fail;
silent unmapped sends hold state. No producer-current-route substitution occurs.

Software-generated raw sends remain part of the common effect. The SDK consumer
validates each retained ring span, copies its samples while the producer cannot
reuse them, holds source samples onto the DSP grid and checks sums for overflow.
Only after complete effect admission does it combine the samples and release
the consumed ring prefix. Lookahead is read but remains queued. Unknown source
intervals consume no software samples. There is no new callback allocation,
filesystem operation, wait, worker, linear RAM or VRAM buffer.

This changes effect interpolation and permits a close audible approximation,
as authorized by the user. It does not change game or voice timing. The original
32 kHz delay lengths and decay coefficients cannot simply be used at the higher
rate: a test signal exposed a 22.7% decay difference through shifted resonance,
despite a clock difference of only 2.28%. The v1 candidate is retained but rejected
for integration testing. V2 retunes delay lengths to 1830/2045/443/152 samples
and computes decay/predelay using the actual 16756991/512 Hz device rate.
The arena-size calculator changes with the constructor; the reverb loop remains
the archived source-derived kernel.

Tests so far:

- 4,280,036 ASan/UBSan checks cover independent rational-clock sample routing,
  both buses, stereo/surround folding, raw-ring wrap, known/unknown prefixes,
  effect boundaries, held gaps, lookahead and rejection before modification.
- Five incorrect boundary/phase/epoch/mode/lookahead mutations are rejected.
- ARM11 compilation passes. V2 shared function had a 2,320-byte static stack
  frame. V3 moves 1,920 bytes into fixed SDK-callback-owned scratch and reduces
  that frame to 400 bytes. The actual SDK has a 4 KiB stack; these static
  frame sizes are not a measured whole-thread watermark. The callback is
  serialized, and no scratch pointer escapes.
- Actual archived effect kernels, three burst/impulse signals and three reverb
  settings: tail energy differs 0.107–0.124 dB; integrated -30 dB decay duration
  differs -0.004–3.041%. These checks do not establish full gameplay-audio parity
  or a listening review. WAV files are retained for review.
- 2,050 retuned constructor cases check requested arena size against actual
  allocations across two/three channels and predelay 0–0.1 seconds.

Pinned Melee audio buffer ownership/publication rules were reviewed. This
adaptation operates in CPU-only raw AUX storage and preserves the SDK's DSP
buffer publication, callback ownership and joined shutdown. It does not add
new cache-maintenance calls or reuse DSP resources early.

Use `test_audio_aux_native.py`, `test_audio_native_reverb_wave.py`, and the
isolated `build_audio_aux_native.py console-setup --revision v3`. Parent is
`unified-audio-program-v2-console-setup-builds`, not unrelated root WIP.
`verify_audio_aux_native.py` checks shared Old80/New124 profiles, exact source
composition, unchanged game archive/shaders/recipe and absence of the old AUX
rate converters from the linked application. Runtime and hardware evidence
remain separate gates. No FPS gain is established by the waveform tests.

V2 exact build `f9fac5c1b527` completed the Old palace replay: status1,307 rows,
source/callback audio faults0, curve shortages0, preserved saves/settings and
owned shutdown. Capture1601 was inspected with Krystal/world/enemies/HUD intact.
Whole sampled ARM encounters fell24,587,424 to24,267,579(-1.30%) versus program-v2,
and12.24% versus the last physical build's matching profile. Source producer
calls28/28 and actor commands280/280 match; particles300/303 and scheduler edges
still differ. These are not cycles, FPS, or causal whole-frame attribution.

Old sampled heap41,716,656/50,331,648,linearfree1,755,776/10,485,760,
VRAMfree4,723,712,mainstack72,984/524,288,process69,599,232/83,886,080.
Main-stack telemetry does not measure the SDK thread. V2 remains unpromoted:
v3 additionally removes the second arithmetic/overflow mixing pass by expanding
backwards in private scratch, then publishing a checked copy. Source index never
exceeds destination index, so reverse expansion cannot overwrite unread input.
All public PCM/ring state remains unchanged on preflight failure.

V1 waveform rejection and V2 evidence are preserved separately. No console files
or assets were changed. Hardware performance, long mixed-source transitions and
listening acceptance remain open; this is not a completed port.

V3 source0d6f83a3f5d5 also passed the Old DSP palace run:144.997hostsec,
307rows/status1/logs drained/owned exit/saves and settings unchanged. Image1600
inspected; Krystal, textured world, enemies and HUD retained. Audio source and
callback faults0,curve shortages0. No listening, long transition or hardware
acceptance. Whole ARM24,587,424->24,143,808(-1.80%); last deployed hardware
build profile27,651,093 gives-12.68%. Audio producer28->25,particles300->293,
actor280->280; phase differences prevent causal frame-time/FPS attribution.
The old converter and raw-add consumers have zero entries and are absent from
ELF. New shared consumer364,803 encounters; reverb436,386->445,218. Callback
counts27 and reverb54 match. These are instruction encounters, not CPU cycles.

V3 sampled memory equalsV2:heap41,716,656/50,331,648,linearfree1,755,776/
10,485,760,VRAM4,723,712,mainstack72,984/524,288,process69,599,232/83,886,080.
The audio owner_bytes diagnostic excludes the private1,920-byte raw scratch;
include it separately in static budgeting. Process measurements include it.
Source/game archive/18GPUprograms/Old80New124/recipe/restoration checks pass.
No unchanged conversion, normal Play, New hardware or endurance test repeated.
