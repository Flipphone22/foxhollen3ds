# Exact single-pass DSP curve comparison

Local shared build `f6a4029da01f` replaces only the two endpoint-comparison
helpers in `audio_dsp_voice.c`, relative to `3b403841b1e1`. The producer still
validates and queues every changed curve at its original source deadline. The
callback still uses the original gain arithmetic. There is no new DSP command,
audio approximation, worker, allocation, cache operation or queue mutation.

The previous producer compared three overlapping 18-byte arrays to establish
that both curves had constant, identical volume endpoints. The end arrays are
not word aligned. The native code now compares all four unsigned 16-bit
endpoints once per channel, preserving exact signed-volume bit encodings.
The callback's constant-curve check also uses typed endpoint comparisons.
Envelope, rate, interpolation, pause and routing comparisons remain exact.
The pinned Melee audio ownership/cache-publication pattern was consulted;
this change leaves those hardware interfaces unchanged.

## Evidence and limits

The source comparison passes 11,100,000 host ASan/UBSan cases, including each
lane of all four endpoints, signed encodings, envelope changes, NaNs and rate/
pause/route changes. The complete queued NDSP owner passes 23,487 ARM checks,
including 10,000 contiguous retained-control segments; nine protocol mutants
are rejected. The SDK channel implementation is real, with simulated replies
and a direct pre-balanced mix adapter at the application output boundary.
Balanced output/DSP sound quality is not proved by that protocol fixture.
Initial fixture linking exposed the newly added output-mapping dependency; the
test adapter explicitly preserves the original fixture's pre-balanced scope.

`unified-curve-compare-v1-benchmark-old-dsp-profile-v2` completes the actual
shared app's startup benchmark in 107.45 host seconds. Native DSP/LLE, state,
save/settings preservation, log drain and owned-process exit pass. The palace
frame-1601 capture is inspected: textured environment, Krystal, purple
creatures, particles and HUD are retained. This bounded run is not full
progression/endurance or physical acceptance. No new PCM was captured: the
audio arithmetic/routing is byte-unchanged and queue decisions were checked.

An analysis of existing ARM basic-block captures attributes direct calls
without another game run. Attributed memory-call counts exactly equal the
observed callee entries for both binaries. All comparisons originating in
`audio_dsp_voice.c` disappear. Total `memcmp` calls fall from 7,224 to 2,374;
its instruction encounters fall from 765,053 to 237,278. Those totals include
unrelated callers and differing workloads, so they are not a pure patch cost.

Whole-window ARM encounters fall 24,565,041 -> 22,630,009 (-7.88% raw).
However, source-audio jobs fall 28 -> 25 and lightning calls 64 -> 0; particle
phases also differ. This is **not an attributable full-frame speedup or FPS
prediction**. Do not subtract selected functions and claim normalized timing.
DSP LLE/capture host timing remains excluded. World draw coverage is identical:
768 draws, 528 in prepared groups and 240 individual submissions.

Measured Old memory is unchanged: heap 41,734,800/50,331,648 bytes; linear
minimum free 1,688,704/10,485,760; VRAM free 4,723,712; main stack
72,984/524,288; process 69,611,520/83,886,080. These are observed samples.
All 18 GPU programs, game archive, setup recipe and Old80/New124 grants are
unchanged. Isolated build restoration passes. No redundant New boot/conversion
was performed; unchanged setup and renderer evidence is reused explicitly.

## Reproduction and package

Run `test_audio_curve_compare.py`, `build_audio_curve_compare.py console-setup
--revision v1`, and `run_audio_curve_compare.py benchmark --family
unified-curve-compare --revision v1 --old --profile` through the existing WSL
toolchain. Output directories are exclusive; use a new revision to retain
earlier evidence. Use `analyze_arm_blocks.py` on the full run path, then
`compare_audio_curve_compare.py` and `attribute_native_memory_sites.py`.
Inspect the capture and record its review before running
`checkpoint_audio_curve_compare.py`.

The shared package is
`build/console-validation/foxhollow-iso-setup-5ce30b6dc0c27f4e.zip`.
It includes one ordinary-app 3dsx and CIA, no private assets or preferences,
and uses the existing startup benchmark menu. It is **not deployed**.
Old/New payload identity and rejection of a stale CIA pairing pass. Fresh
conversion is not claimed for this binary; the unchanged recipe evidence is
reused. `audio-curve-compare-test/integration.json` binds its hashes and tests.

The last physical measurement remains 13.5157 FPS / 73.988 ms. The 61.19 ms
prediction is unverified. Full rendering parity, target performance and final
gameplay/save/endurance validation remain incomplete.

## Next renderer cost identified without another boot

The parent capture also accounts for every one of 9,232 `GPUCMD_Add` entries:
3,992 from `shaderProgramConfigure`, 1,536 from context publication, 1,008 from
uniforms, 808 from TEV binding, 704 from attributes, and the remainder from
lighting, buffers and other SDK publication. These are call counts, not cost
shares. Existing native program plans only cache false/false configuration;
code-upload switches intentionally remain direct SDK calls. Preparing complete
immutable upload sequences is the next candidate. It must preserve SDK order,
program-setter invalidation, GS/VSH transition safeguards, bounded memory and
command capacity; merely caching partial state would risk previous hardware
hangs. This extension is not implemented or included in this package yet.
