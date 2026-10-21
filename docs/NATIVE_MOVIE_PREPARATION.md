# Native startup movie preparation

These local modules implement the ISO-to-native movie stage without a desktop
encoder dependency. They are linked into the shared normal setup build and
connected through its four-stage installation task. That build has only passed
menu and source-fingerprint cancellation checks; it has not completed conversion
or activated a whole-game installation. Original ISO, assets and saves stay intact.

The **complete native conversion has now run on the PC** against the original
ISO, with no seeded frames. The output contains all 1,797 native frames and
unchanged audio (1,787 distinct source video chunks). It is 37,014,432 bytes,
SHA256 `656ad2a7f8b300be3c1d49996da3bf5cde832e8a545783675fe505bc3836e298`.
`install-movie-full-build/audit-checkpoint-00003603.json` verifies every source
identity, video/audio payload, assembled cyclic frame link and padding byte.
The source ISO stayed unchanged throughout bounded, resumable conversion runs.

`install-movie-full-images/result.json` additionally checks all framing/audio
and decodes five frames against the original images. The lowest sampled PSNR
is 35.36 dB; side-by-side frames 45, 89 and 898 were inspected. These are baked
movie images, not evidence that the live renderer reproduces those effects.
The native output remains private and unactivated. This result does not measure
console conversion time or establish the complete app installation/handoff.

- `install_movie_source.c` validates the supplied PAL revision 1 movie's SHA,
  metadata and all 1,797 frame links. Its bounded index and one frame buffer
  replace whole-movie buffering. All original video and audio chunks match an
  independent source parser (`install-movie-source-test`).
- `install_movie_decode.c` restores THP JPEG entropy stuffing and validates the
  final EOI marker/alignment. Twelve original frames decode byte-exactly against
  Pillow. Invalid/truncated streams, allocation failures and cancellation are
  tested with ASan/UBSan. The SDK JPEG translation unit uses `-fshort-enums` for
  its private ABI; public interfaces contain no JPEG structures.
- `install_movie_resize.c` performs bounded 512x384-to-256x192 Lanczos resizing.
  Four real frames match Pillow exactly, with overlap/bounds/cancellation checks.
- `install_movie_frame.cpp` produces existing tiled ETC1 payloads and optional
  LZ4/CRC compression. Four half-resolution frames and one original-resolution
  frame match the prior tex3ds cache byte-for-byte. Six cancellation points,
  source immutability and runtime payload validation pass.

The decoder/frame tests include actual ARM SDK compilation and complete
link-only ELF checks. These ELFs were never launched, packaged or deployed.
Test reports live under `artifacts/research/install-movie-{decode,frame,source}-test`.

`install_movie_pack.c` adds a checkpointed per-frame spool and assembly into the
existing native THP container. Duplicate original video can reuse its encoded
payload while each frame retains its own untouched audio. Assembly preserves
source timing and all frames. Progress callbacks distinguish hashing, indexing,
decoding, resizing, encoding, compression, writing and assembly. No gameplay
worker, rendering path or audio playback policy is changed.

Initial host integration gracefully stopped after two durable frames and
resumed through frame 21 in a bounded 45-second run. There was no hard timeout
or source mutation. Evidence: `install-movie-full-build/result.json` and its
immutable attempt folders. The longer continuations have independently checked
1,064 actual native-converted frames, including each encoded video and original
audio byte against the existing prepared reference. This is incomplete
conversion, not a console time estimate. `audit_native_movie_spool.py` reads only
the checksummed durable prefix, even when later frames are being appended.
The latest 300.069-second continuation stopped gracefully at frame1064, retaining
21,997,954 checkpointed bytes; no hard kill or source modification occurred.
`audit-checkpoint-00001067.json` passes all1064 unique source videos. Earlier
272-, 503-, 613-, 739-, 862- and 958-frame audits remain preserved. This continuation
overlapped original-image padded-codec checks and their fixture preparation.
Elapsed host runtime is evidence of test cost only; console preparation duration
remains unmeasured.

The complete writer now passes ASan/UBSan and actual ARM SDK compilation/linking
(`install-movie-pack-test/result.json`). Its focused fixture seeds 1,795 frames
with previously encoded video so recovery/assembly can be tested quickly. The
last two original frames correctly reuse earlier video; their own audio remains
unchanged. Two assembly interruptions and two unfinished file tails recover.
All 1,797 assembled video/audio chunks match the reference. The output is
37,014,432 bytes, SHA256
`656ad2a7f8b300be3c1d49996da3bf5cde832e8a545783675fe505bc3836e298`.
This verifies the writer, **not** a full new encoding of all source frames.
The temporary fixture/output are not installed or activated. Full encoder
completion and whole-stage memory/fault validation remain outstanding.

The writer's link-only ELF is 476,244 bytes text, 7,520 bytes data and 47,504
bytes BSS, including the SDK scaffold. This is neither the final app-size delta
nor a runtime heap measurement. It was never run or packaged. The driver rejects
changed source recipes and never activates this stage as a complete game install.

The ISO-only user flow, progress display and graphics presets now run in the
complete shared application in isolated Azahar checks. Integrated diagnostic
registration, complete asset coverage, useful fidelity-safe reductions,
menu-to-game handoff and measured temporary SD/RAM budgets remain outstanding.
See [STARTUP_MENU_OWNER.md](STARTUP_MENU_OWNER.md) for exact runtime scope.
