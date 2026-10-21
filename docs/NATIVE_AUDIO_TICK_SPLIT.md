# Exact native sequencer tick splitting

The current Old palace profile attributes4,320 fmodf and4,320 floorf calls to
MusyX SetTickDelta in eight rendered frames. These are not renderer trig calls.
The exact ELF, callsite coverage and linked source checksum are recorded in
unified-view-scan-v1-benchmark-old-dsp-profile-v2/{math-sites,arm-source-seqHandle}.json.

The candidate preserves the original BPM/delta/speed float32 calculation and
all sequencer/event code. It replaces only conversion of the resulting float
increment to low16-fraction/high-integer ticks. For0<=x<2^31, integer conversion
gives floor(x). Subtracting that integer is exact: below1 it subtracts zero;
above1 Sterbenz's condition holds, and above2^24 every representable value is
already integral. Multiplication of the remainder by2^16 is exact. The result
therefore equals the original fmodf(x*65536,65536) conversion. The old operations
remain unchanged outside the admitted range. This is not an audio-quality or
tempo approximation, nor a new persistent tempo cache.

There is no allocation, extra buffer, thread, GPU work or new DSP publication.
Pinned Melee audio ownership/publication was reviewed; those boundaries remain
unchanged. The calculation stays on CPU because it schedules original music
events; only redundant generic math-library work is removed.

Host ASan/UBSan checks pass17,149,472 float splits and changing tempo/speed/index
steps, including every mantissa encoding from0.5 up to2 and exact accumulated
clock carries. Wrong fractional scale and discarded high-word mutants fail.
Actual ARM11 checks pass2,048 cases, with all four VFP rounding modes and flush
to zero varied. Output guard bytes, stack pointer and FP control bits are
preserved. The first128 cases count11,401 versus3,072 instruction encounters;
these are not cycles or full-frame FPS.

The first ARM harness incorrectly included call-clobbered NZCV condition flags
in its preserved-control mask. It was corrected to check actual control bits;
production code was unchanged. Cumulative floating exception flags are outside
the equivalence claim; the original FP control settings are preserved.

Use `test_audio_tick_split.py`, `build_audio_tick_split.py console-setup --revision v1`
and `run_audio_tick_split.py benchmark --family unified-tick-split --revision v1 --old --profile`.
The builder uses the archived5f4cfbf0d3d8 source, not unrelated root WIP, checks
the tested source identity and compares the complete generated seq.c against
the pinned original with only the conversion replacement.

Shared source4e9ce0e40ad2 (3dsxc61b603532a1/CIA004c8b58411d) is built and packaged
as foxhollow-iso-setup-3992cc5acc2393d9.zip, NOT DEPLOYED. The Old80 palace run
completed in103.92 host seconds with native DSP/LLE, state/save/settings/drain/
owned-exit checks passing. Capture1600 was inspected with the textured palace,
Krystal, purple creatures, particles and HUD retained. Audio source/callback
faults are0. No fresh New runtime or ordinary opening/conversion boot: prior
GPU/owner and unchanged setup evidence are explicitly reused.

Actual callsite coverage verifies zero sequencer fmodf/floorf calls in the new
window, versus4,320 each in the parent. Remaining2,214 fmodf and2,112 floorf calls
are fully attributed to other owners. The sequencer ran140 times versus135;
its own body grows297419->336985 encounters because the conversion is now
inline. Library work falls, but the raw complete window rises22215624->22527356
(1.40%) with audio jobs27->28, particles275->292 and different voice activity.
Neither a full-frame speedup nor a regression is established by this window.
Do not convert the synthetic instruction saving into physical FPS.

All18 GPU programs, the game archive, Old80/New124 CIA profiles and the setup
recipe are unchanged. The generated seq.c equals the entire pinned original
with only the tested conversion replacement. Build/source restoration and
shared package checks pass. The checkpoint initially compared CRLF raw source
against newline-normalized generated text; matching the builder's explicit
newline handling corrected that verification failure without changing code.

Observed memory is unchanged: normal41859104/50331648, linear free1622784/
10485760, VRAM free4723712, stack72984/524288, process69595136/83886080. No added
allocation. These samples do not establish full-game peaks or physical limits.
Full parity, sustained performance and final progression/endurance remain open.
Evidence: artifacts/research/pica-scene-rewrite/audio-tick-split-test/
{result,profile-comparison,integration}.json and
artifacts/native/unified-tick-split-v1-benchmark-old-dsp-profile-v2/.
