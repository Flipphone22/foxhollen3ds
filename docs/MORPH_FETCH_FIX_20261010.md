# New 3DS startup GPU hang: morph fetch layout

The reported black top screen with working bottom-screen UI is a real GPU
completion failure in deployed normal `0b2c0f722073`. The read-only collection
`artifacts/console-visual/20261010-130530-223` records a timeout at retrace 216,
after cockpit resources load. This does not prove any cockpit image was shown.
The once-per-failure command and 24 MiB CPU input snapshots completed before
recovery. Empty buffered frame/audio/SD CSVs do not mean the game never ran.

The submitted list contains 91 draws. Eleven violate the existing hardware
fetch-layout gate: the morph resident-attribute stream starts eight bytes into
each 40-byte vertex but describes only 32 bytes of attributes. All captured
index/vertex CPU reads are bounded and finite; texture layouts pass. The 11
bad layouts are independent evidence of a contract violation, not physical
proof of which individual draw stalled.

`fh_pica_morph_layout.h` now binds the resident stream at the vertex base and
uses an explicit leading eight-byte padding component (`0xD`), followed by
attributes 1 through 5. The separate eight-byte position stream is unchanged.
The pinned Melee `matrix_vertex_pointer` in `renderer.c` uses this same PICA
padding mechanism to skip a coordinate supplied by a separate stream.

The shared binding covers ordinary actors and their shadow path, plus the
hologram consumer. It neither disables morphs nor restores CPU skinning.
There are no extra allocations, draws, shader instructions, vertex copies or
per-draw SDK calls. Leading padding also avoids a trailing fetch beyond the
last resident vertex.

`tools/test_pica_morph_layout.py` executes the pinned Citro3D buffer compiler
with the actual shared binding, then checks the captured layouts. All 91
layouts pass after substitution; all 66 affected attribute address/stride/size
tuples stay exactly equal. Removing the padding fails the gate. ASan/UBSan,
10,000 address variations and SDK failure handling pass. This catches the
invalid hardware contract on PC even though earlier Azahar render tests
accepted it.

The normal-app runner now accepts `--graphics-settings-dir` to copy and hash
the actual saved settings into an isolated test profile. This hardware launch
used original geometry and original-resolution textures. Its saved FHGRAPH3
mask migrates with ground shadows disabled; new opt-in specular/fur were not
enabled. Testing only reduced default assets is insufficient coverage.

Physical recovery remains unverified until the corrected build runs on the
console. No claim that every GPU hang is fixed follows from this repair.

Candidate normal `4fe1d96ad2a1` and replay `2192fb6abc45` share the same game
archive, 66 renderer sources and unchanged interpreter-tested 511-instruction
shader. Both CIA headers retain Old 80 MiB / New 124 MiB profiles. Build
verification is in `morph-layout-test/build-verification.json`.

The exact normal `.3dsx` passed an isolated New-profile startup with copied
console graphics slots, original geometry and original-resolution textures:
721 retraces, 503 morph updates, 4,499 group bindings, Select exit and drained
logs in 25.38 host seconds. Image 540 was inspected: cockpit, movie and bottom
panel are present. The shared replay CIA content passed the Old-profile palace
case: all state checks and save isolation passed, 42 morph updates / 12,920
group bindings, drained logs and owned exit in 52.47 host seconds. Image 1410
was inspected for world, Krystal, glow and HUD. These are correctness checks,
not hardware performance or recovery claims.

Measured normal heap peaks were 45,973,840 / 71,303,168 bytes (New startup)
and 40,864,800 / 50,331,648 (Old replay). Linear minimum free was 18,625,280 /
25,165,824 and 4,265,088 / 10,485,760. Initialized VRAM minimum free was
4,846,592 on both; measured stack peaks were 72,844 / 72,892 of 524,288 bytes.
Process commitments were 102,637,568 / 130,023,424 and 67,534,848 / 83,886,080.
Startup rows before renderer initialization report zero VRAM; they are not a
measurement of exhausted VRAM. `morph-layout-test/integration.json` records
these checks. The six unrelated VRAM work-in-progress files were restored by
the isolated builders; the build did not include them.

The pending build also includes bounded slow-I/O attribution from work preceding
this report: 9,768 bytes of normal static RAM, last 64 completed calls of at
least 50 ms, file hash/basename and per-launch thread token. Producer queue/drain
spans are separate from worker filesystem calls. Details write only at exit;
an incomplete marker replaces stale evidence at startup after replay archival.
This instrumentation does not itself fix queue backpressure or predict console
SD latency. `sd-slow.csv` and `log-io-summary.json` are collected and archived.

Deployment completed to New223: `console-settings-update/20261010-131416-223/manifest.json`. Normal and replay `.3dsx`/CIA plus metadata were staged and byte-readback verified, then renamed/final sizes checked. Saves match their pre-install hashes; graphics settings preserved. Original assets reused existing verified payloads. Old229 was not updated. The `.3dsx` is ready in Homebrew Launcher; CIA needs FBI installation. Physical startup recovery is pending.
