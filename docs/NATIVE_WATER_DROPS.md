10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native splash droplets

Follow-up: `tools/test_pica_point_shader.py` executes 30,720 live droplet and
30,720 ordinary star vertices using the exact linked Azahar float24 shaders.
All 16 original output lanes match the earlier point expansion bit-for-bit.
Rasterization/occlusion acceptance remains open. The subsequent local water-fog
integration is documented in [NATIVE_WATER_FOG.md](NATIVE_WATER_FOG.md).

Local replay `203bd9e04e3e` and normal `555f545cd4d8`; not deployed. The original water module's droplet
simulation, spawning, random calls, gravity, damping and impact/ripple creation
are unchanged. A new native render adapter reads its at-most-30 live centres
in original pool order. Ripples and wakes still need native render replacements.

One draw renders the entire live droplet pool. A 640-byte immutable allocation
contains 120 point/corner identifiers and 180 triangle indices; only centre
uniforms change during play. The existing star-point vertex program performs
view/projection and clip-space expansion. It now has an explicit live-centre
mode; ordinary stars explicitly turn that mode off. The eight-entry shared
program grows from 500 to 503 instructions, within the 512-instruction limit.
There is no geometry shader, CPU quad construction, per-frame vertex upload,
texture, render target, extra worker or GX interpretation.

The point size is the source GX value 0x12, or three source-viewport pixels,
scaled with the viewport onto the native screen. Colour is the original
quarter ambient plus 128, with alpha 128. The pass depth-tests without depth
writes and uses source-alpha blending. Its placement after splash rings follows
the original render order. Complete fog interaction and physical rasterization
remain unvalidated; original reflected ripples/wakes are separate effects.

The generated adapter copies at most 360 bytes of live centres into bounded
stack scratch and publishes at most 30 centre uniforms. This is live effect
state, not expanded/rebuilt mesh geometry. Resource preparation and freeing
follow the pinned Melee immutable-owner/retirement rule: flush the prepared
allocation once, retire GPU readers before release, and invalidate the actual
pipeline handoff. It borrows the existing shared shader entry; no new SDK queue
or OS service is introduced. New core 2 remains owned by the audio worker.

WATER DROPS is on graphics page 9 and defaults OFF on both profiles. It is
independent of SPLASHES and obeys the master RENDERING switch. The replay mask
bit is 524288; prior masks retain their meaning. Existing two-slot settings
persist it on clean exit, and recording/replay locks apply normally.

`tools/test_pica_drops.py` compares the actual original droplet selection loop
with the generated native adapter: 25,009 centre publications and 1,000 colour
cases match, with no pool mutation. Five original simulation/spawn functions
are checked byte-for-byte. Native load/release code passes 100 repeated owner
cycles under ASan/UBSan; removing retirement fails. Graphics option and setting
tests pass, including the new bit, reversible/locked toggles and persistence.
These are CPU/source tests, not physical GPU proof.

Three serial exact-console-binary Azahar route checks pass state assertions,
save preservation, log drain and owned-process exit:

| Profile / droplets | FPS | Scene preparation ms | Native submission ms | Droplet batches / points |
|---|---:|---:|---:|---:|
| New ON | 56.4091 | 1.796531 | 3.629156 | 79 / 499 |
| New OFF | 55.4563 | 1.799664 | 3.613916 | 0 / 0 |
| Old ON | 48.8103 | 1.792645 | 3.605070 | 132 / 706 |

The same movement recipe and fixture are used. Animation/audio work and splash
counts differ between these ordinary time-based input runs, so ON being faster
is not a speedup claim. Per-active-batch submission averages 0.03078 ms on both
emulated profiles, below the provisional 0.1 ms CPU budget. No physical GPU or
CPU performance gate is established. The Old emulator rate is not a prediction
of Old console performance, which has been substantially slower.

Peak normal heaps: New 48,771,880/71,303,168 bytes, Old
41,431,848/50,331,648. Minimum free linear: 18,328,064/25,165,824 and
3,650,048/10,485,760. Both initialized VRAM-free values are 4,858,880; measured
main stack is 72,908 of 524,288 reserved. Process commitment remains
103,149,568/130,023,424 and 67,497,984/83,886,080. Headroom is retained.

World, Fox, vegetation and HUD captures were inspected on both profiles. The
first interval captures did not isolate droplets clearly enough for visual
acceptance. A fourth, explicitly diagnostic run uses
`tools/capture_native_drops.py` to read stable live centres/camera and capture
only the owned Xvfb display. It changes no guest state or input. Eleven captures
at retraces 1741..1823 locate live droplets beside Fox's feet; 1741, 1796 and
1814 were inspected. Pool sampling and display capture span 5..7 retraces, and
terrain/actor occlusion is not determined by projected coordinates alone.
Therefore full droplet visibility, clipping and original-image fidelity remain
OPEN; counters and world screenshots are not treated as visual proof.
This diagnostic run is excluded from performance comparison.

Reproducible build: `artifacts/research/pica-scene-rewrite/build-water-drops-v1.py`.
Evidence: `water-drops-test/{result,route-validation}.json`, runtime folders
`artifacts/native/water-drops-v1-*`, and the separate
`water-drops-visual-on-new/drop-observations.json` with untouched screenshots.
No remote apps, assets, settings or saves changed.

Matching ordinary build `555f545cd4d8` uses the same checked game archive and
shared renderer. Its isolated empty-save menu check passes in 25.13 host seconds:
ON/OFF/ON input, clean Select, drained logs and owned exit, with no replay hooks.
Capture 540 shows the new page without overlap; saved FHGRAPH4 mask 540673 and
checksum were verified. This is menu/persistence evidence, not droplet gameplay
acceptance. Build scripts isolate the unrelated native-image VRAM candidate and
restore the working files/archive in `finally`.
