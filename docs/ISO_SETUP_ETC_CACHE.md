# Exact ETC block reuse during setup

The startup CMPR converter now has an image-local cache for the expensive
high-quality ETC1 encoder. Repeated 4×4 RGB blocks reuse their eight encoded
bytes. This does not reduce texture quality, resolution, or alpha coverage.
The original decoder, BOX filtering, ETC quality setting, output ordering,
visible-error calculation, quality rejection, and tile cancellation callbacks
remain in place.

Each of the 256 entries retains all 64 encoder-input bytes, eight output bytes,
and a 32-bit hash. A hash match also requires a full input-byte comparison;
collisions only cause a miss. The cache resets between images. It occupies
19,456 bytes of temporary stack storage, with no heap allocation, worker, GPU
resource, persistent state, or gameplay work. Compiler stack-frame measurements
are recorded by the test; they are not measurements of the complete startup
call chain's high-water mark.

The pinned Melee texture decoder remains the source for CMPR palette semantics.
The cache sits after that decoding and the existing transparent-to-black
composition used by the pinned tex3ds encoder. Alpha coverage is independently
generated, so opaque black and transparent black may share encoded RGB without
sharing their alpha. No GPU publication or live texture owner changes.

`tools/test_install_texture_cache.py` compares the candidate with the exact
deployed source archived in `normal-setup-shadow-share-v1-builds` and the
existing private ETC texture pack. It checks every encoded byte, dimensions,
format, visible error, coverage, output guards, progress and cancellation.
It also deliberately poisons a same-hash entry with different pixel bytes;
removing the full-byte comparison must fail. ASan/UBSan runs and ARM compilation
cover the actual converter. The independent padded-image test retains edge
pixel, transparent pixel and cancellation coverage.

The first wider sanitizer test exceeded its 240-second host bound. Its failed
result remains in `artifacts/research/install-texture-cache-test`. The focused
follow-up uses two representatives per format/reduction/size category through
128 pixels, avoids repeated complete large-image cancellation passes, and
preserves partial test output if another deadline is reached. This is not a
claim that every game texture has been re-encoded with this candidate.

The build recipe hashes the converter source, so texture and movie recipe keys
change. Geometry keys should remain unchanged. Existing journals must not be
relabelled to match the new encoder. The running full ARM conversion deliberately
continues with its original deployed executable and recipe. It tests the full
installer independently of this candidate.

This is a setup optimization, not a gameplay FPS improvement. Host encoder-call
counts and CPU times do not establish ARM conversion time, whole-install speed,
or console renderer performance. No console deployment is implied.

The focused test passed for 19 real textures (full/reduced, opaque/alpha), with
57 cancellations and the poisoned-entry mutant rejected. All encoded bytes and
visible-error totals match both the deployed encoder and prior private pack.
Calls to the expensive encoder fell from 6,648 to 5,831 (12.29%). Sanitized host
process CPU time fell from 55.658198 to 52.607332 seconds (5.48%); this is a modest
sample result, not a large whole-install speedup. The separate padded test
passed 14 cases, 8,832 edge pixels and 42 cancellations.

ARM compilation reports a 19,920-byte converter frame versus 448 bytes before;
the cache itself is 19,456 bytes. The existing main stack reservation is
524,288 bytes. Whole-chain measured stack usage and ARM execution of this
candidate remain unverified. Reports are in `install-texture-cache-v2-test` and
`install-texture-cache-padded-test` under `artifacts/research`.

Shared local build `normal-setup-texture-cache-v1-builds` completed and restored
the isolated worktree. 3DSX `93e1b9dc7059`, CIA `cdb70a7d45b2`. Verification finds
exactly one changed source versus the previous shadow-preparation setup build:
`install_texture.cpp`. Renderer sources, linked shaders and game archive are
unchanged; the CIA retains Old 80 MiB / New 124 MiB profiles. Geometry recipes
match the previous local candidate, while texture/movie recipes change as
expected. `tools/verify_install_texture_cache_build.py` records these checks in
the build's `verification.json`. This executable has not been run in Azahar or
on hardware and is not deployed. Existing physical installs remain unchanged.
