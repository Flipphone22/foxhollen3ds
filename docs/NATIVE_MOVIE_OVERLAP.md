# Movie read/decode overlap

Previously `DecodeNextMovieFramePC` issued its existing next-frame read-ahead
hint only after video decode, integrity validation, audio decode and publication.
The native adaptation now issues that hint immediately after the current
`DVDRead` succeeds. Its offset and size are calculated locally; the source's
frame index, texture index, audio state, loop flag and publication still advance
in their original order. The old late hint is removed so a completed speculative
read is not accidentally issued twice.

The current frame has already been copied to the game's read buffer. The worker
reads into its separately owned existing buffer. No additional allocation,
worker, GPU command, cache operation or resource ownership is introduced.
Pinned Melee's sleeping file worker and explicit completion/ownership handoff
were consulted; the existing native movie reader keeps those responsibilities.

`test_movie_overlap.py` compiles and executes the original and patched source
control flow under ASan/UBSan against identical synthetic inputs. All 3,264
cases retain game state, audio/decode calls and return values, including loop
wrap, texture slots, read failure and decode failure. DVD/audio/codec functions
are mocks in that test, so it does not establish real overlap or visual parity.

`test_movie_overlap_owner.py` separately executes the actual archived movie
worker with pthread-backed SDK events. It confirms the current caller buffer
remains unchanged during a delayed next read, after completion, and while the
next frame is consumed. Existing delayed-read and close/join cases also pass.
Its first invocation used an incorrect wrapper replacement and failed to
compile. That touched only generated test files, not app sources. The original
wait fixture was restored and rerun; its prior result is retained as
`movie-wait-test/result-before-owner-harness.json`. The corrected owner test has
its own output directory. Both later runs passed with 301 polls for 16 delayed
reads; counts are host scheduling observations, not timing guarantees.

A fatal first-frame video decode can now leave one speculative read pending
where the old path issued none. That read owns only the worker buffer. Existing
close joins before release, and a retry at a different offset must go through
the existing offset/length checks; it cannot consume mismatched future bytes.
There is no new cancellation, decoder bypass, or progression shortcut.

The isolated builder starts from `movie-crc-aligned-v1`, checks the tested source
hashes, and rebuilds only the movie game unit. Physical throughput, long playback,
full rendering parity, the deferred-surface submission gate, and the dinosaur
rescue crash remain independent requirements.

## Local build and runtime evidence

Normal source `dfa1cccb5c31` /3dsx `9a07358cf0ef` and replay source
`e18c031fffa2` /3dsx `0650dd364e30` are built, not deployed. Their 801 source
records, 21 linked GPU programs and game archive match exactly. Both retain
Old80/New124 profiles. The game archive differs from the parent only in
`dll_3e.o` and the symbol index; root/archive restoration passes. Replay has
source/build verification only, not a new replay runtime acceptance.

The normal exact-binary cockpit route passes in 74.12 host seconds with clean
exit/drain, unchanged private saves/assets, zero movie decode errors and an
inspected final capture. Fox/fur, cockpit textures and the transparent Game
Select panel remain visible. This is not a source-image parity comparison.
Sampled emulated presentation intervals average 16.7110 ms versus the aligned
CRC parent's 19.5199 ms; worst sampled interval improves 36.234 to 28.028 ms.
These are phase-varying emulator-clock observations, not host playback FPS or
a console-performance forecast. The run's host duration changes much less
than the emulated presentation intervals. No physical 60 FPS claim is made.

Observed heap 54,180,888/71,303,168, linear free 17,125,120/25,165,824, VRAM
free 4,690,944, stack 72,936/524,288 and process 104,800,256/130,023,424 bytes
are unchanged. Deferred-surface maximum inclusive submission is .50965 ms,
still above its .5 ms gate; that gate is not relaxed. The full goal, sustained
performance and rescue-crash resolution remain incomplete. Evidence:
`movie-overlap-test/{result,archive,runtime}.json`, `movie-overlap-owner-test/`,
and `movie-overlap-v1-paired-builds.json`.
