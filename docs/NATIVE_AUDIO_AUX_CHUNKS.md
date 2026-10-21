# Contiguous native audio return mixing

The accepted DSP audio backend retains its effects, source clocks and original
signed PCM. Its shared AUX consumer previously recalculated ring slot and sample
indices for every sample and used a 64-bit sum to detect 32-bit overflow.

The replacement copies each already-validated source span in contiguous chunks,
splitting at 160-sample ring boundaries. Slot ownership remains with the input
queue until the existing application callback commits the tail. It adds no
buffer, allocation, thread, DSP command or new publication boundary. Overflow
uses checked 32-bit addition with the exact same reject condition and output.
All admission checks, effect identities, lookahead and source/device phase
conversion remain intact. Pinned Melee audio publication rules were consulted;
this changes only CPU work on private scratch, not DSP cache coherence.

The archived original and candidate execute the existing independent rational
clock and PCM/ring checks under ASan/UBSan: 4,280,036 checks and3,157 effect
applications each. The scenarios include stereo/surround, delayed input,
lookahead, changing effects, failed admission and ring wrap. Missing wrap,
wrong offsets and missing block advancement are rejected. Another million
checked-add cases include signed32 extrema. Strict ARM11 compilation passes;
reported shared-function stack use is400 bytes, not a measurement of the
complete audio thread's stack.

The patch applies to accepted source20c5e0b542b5. Root contains unrelated WIP;
the isolated builder verifies the tested owner and all linked test dependencies,
keeps the game archive/shaders/Old80-New124 profiles and restores the workspace.
The performance gate must count memcpy work introduced by contiguous copies,
not just the reduction inside the shared consumer. No hardware FPS claim is
supported by these helper checks.

## Shared application result

Source `92fd26aa4763` passed the Old80 palace run with preserved saves/settings,
state assertions, DSP LLE, drained logs and owned exit. Capture1600 was inspected;
palace textures/geometry, Krystal, purple creatures, particles and HUD remain.
Diagnostic host duration was99.49 seconds, not a console frame-time measurement.

Both profiles contain27 calls to the shared consumer. Its sampled instructions
fall364803->162624, while all memcpy encounters rise891511->917814. The new
copy work is therefore included in the comparison; that global memcpy count
also contains other owners and is not precise per-caller attribution. Application
and admission helper counts remain1782 and81. No new render work is introduced.

Whole-window encounters rise21640568->21912036(+1.25%), with voice-curve calls
1549->1711 and particle drawGlow calls284->312. This is neither an attributable
full-frame regression nor an FPS gain. The independent correctness comparison
and smaller consumer workload support retaining the change; physical and
full-frame performance acceptance remain open.

Heap41859104/50331648, minimum linearfree1622784/10485760, initializedVRAMfree
4723712, sampledmainstack72984/524288 and process69599232/83886080 match the
parent. Strict ARM compilation reports400-byte shared-function stack use for
both versions; this does not measure total callback-stack depth.

One source file changes; the game archive,18 GPU programs, Old80/New124 profile
and setup recipe are unchanged. Shared payload/stale-CIA checks pass. Package
`foxhollow-iso-setup-a741d18d0583a347.zip` is ready locally, not deployed. It keeps
the previous sparse stop-tail change and excludes the separate early-particle
experiment. Prior setup/New renderer evidence is reused explicitly; no fresh
conversion boot or New hardware/runtime claim is made.
