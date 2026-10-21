10 October: physical probe v3 completed, rejecting the padded morph loader in all12 comparisons; packed attributes matched all12. New223 normal913f01dfaa2a/replayd5f2e028e258 deployed/readback-verified with packed static lane; gameplay visual confirmation remains pending. See [MORPH_PACKED_FIX_20261010.md](MORPH_PACKED_FIX_20261010.md). Earlier padding acceptance statements below are superseded.

10 October: startup GPU capture exposed an invalid split-stream fetch layout. Shared actor/shadow/hologram binding repaired and New223 updated to normal4fe1d96ad2a1/replay2192fb6abc45. Physical recovery pending. See [MORPH_FETCH_FIX_20261010.md](MORPH_FETCH_FIX_20261010.md); earlier CPU and emulator tests did not establish this hardware contract.

10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native morph positions (local candidate)

The original fixed-point morph producer already updates facial/model positions.
The native renderer previously kept drawing the unchanged resident base positions.
The candidate publishes that result before the original CPU hit-volume skinning
can reuse its source buffer. Skeletal transformation remains on PICA.

At model load, a bounded union of affected source vertices compiles to sparse
destination writes. Only affected draw groups get a separate eight-byte position
lane. All other attributes, material plans, indices and UVs stay in their existing
resident asset. Three lanes per instance protect outstanding GPU readers; no
per-update allocation, topology rebuild, GX interpretation or new worker is used.
Ordinary actors, planar silhouettes, and hologram submission consume the same
instance-owned position lane. Hologram morph visual acceptance remains open.

This adapts the pinned Melee renderer's write-site cache publication and resource
retirement rules through the existing Foxhollow owner/fence helpers. Slots are
flushed before publication and retired before overwrite/free, including repeated
updates within a frame. Shutdown frees already-retired storage without reopening
a frame. Existing game animation, CPU collision/hit-volume producers and RNG are
unchanged. CPU sparse gathering is necessary to preserve the exact signed-short
result, including source integer interpolation/narrowing; it does not skin meshes.

The pool permits 64 instances, at most 1 MiB linear and 256 KiB metadata, additionally
subject to existing heap headroom and Old/New linear reserves. The prepared asset
audit covers 36 morph-bearing models and verifies every prepared coordinate against
its original source reference. Largest observed instance: Krystal, 60,672 linear
bytes for all three slots. Fox uses 34,176 bytes. Original-geometry profile and
whole-game peak residency still need runtime acceptance.

Page 8 adds MORPHS, ON by default on both models. OFF selects base positions;
the game continues animation and sparse publication so enabling it again consumes
current state. It is a visual diagnostic switch, not a promise to remove all morph
CPU work. No restart is required. Existing preference bits remain unchanged.

## Evidence and limits

Evidence directory: `artifacts/research/pica-scene-rewrite/morph-native-test`.
The actual source interpolation versus native sparse consumer passes 144,000 exact
coordinate checks, including negative weights, distinct instances, padding and
invalid admission. Actual owner tests pass 192 retirements / 3,120 updates, delayed
readers, both pose buffers, slot wrap, toggle continuity and zero update allocations;
missing-retirement mutants fail. ASan/UBSan are clean. These mock GPU ownership,
not physical PICA sampling.

The first integrated candidate only hooked the shadow producer: its zero-update
run was not accepted as working animation. Both ordinary and shadow producer
hooks are now checked separately before their CPU skinning calls.

Shared replay 4f52308db047 passed isolated New/Old cave checks, save preservation,
assertions, log drain and owned exit. Captures at retrace 1410 were inspected:
Fox, world, mist and HUD remain present. Whole-session morph updates: New 75,
Old 42; sampled camera is behind Fox, so this is not facial-expression acceptance.

| Azahar check | FPS | Native submit ms | Scene ms |
|---|---:|---:|---:|
| New cave, preceding splash baseline | 59.831 | 5.071 | 1.822 |
| New cave, morph candidate | 59.831 | 5.208 | 1.985 |
| Old cave, morph candidate | 57.172 | 5.232 | 1.957 |
| New cockpit, morph candidate | 59.846 | 1.615 | 0.719 |

Cockpit captures 750/990 show Fox's face/eyes, with 236 actual morph updates.
No matched desktop expression capture yet: all blink/speech states and reported
lighting errors remain unaccepted. Native position storage peaks at 93,312 linear
bytes plus 5,012 metadata bytes in these checks. New cave heap 48,114,488/71,303,168,
linear free 19,358,464/25,165,824; Old heap 40,774,456/50,331,648, linear free
4,678,400/10,485,760. Both initialized VRAM free 4,858,880, measured main stack
72,836/reserved 524,288 bytes. Full process grants remain within Old80/New124 MiB.

Timings are emulator-only, measured windows differ from whole-session morph
counters, and Old/New update counts vary. No physical performance or visual-parity
claim. New cave added scene/submission cost stays below the provisional 0.5 ms
feature gate, but dialogue-heavy scenes and hardware still need measurement.

Replay 3818c7cecff8 extends this to hologram submission and strengthens duplicate
owner detection. It is local only; shared linked shader remains 500/512 instructions.
Old cockpit check passes (35.00 host seconds, 59.832 Azahar FPS, 126 live morph
updates); capture 750 inspected. New cave UI route passes (50.58 host seconds,
148 live updates); captures 1200/1500 inspected, but zero hologram draws means
this does not validate the hologram change. Menu touch/lock/no-redraw tests and
preference persistence tests cover MORPHS and SPLASHES. The previous test's
rejection of bit 131072 was stale after adding SPLASHES; it now accepts both new
bits and rejects the next undefined bit, without changing replay defaults.

Normal candidate 6e0aafb67db6 built against the same game archive. Its ordinary
app check passed, but image 540 exposed hint text overlapping the MORPHS control.
Corrected normal 055b1c1a8727 moves the hint into the header. A focused ordinary
app check passes (24.99 host seconds); image 540 is inspected and readable, and
the real saved FHGRAPH4 preference has the expected mask 278529 and valid CRC
after toggling off/on/off and exiting with Select. No native test hooks are
present in this build. Host UI tests still pass, including idle no-redraw checks.
Console saves and installed files remain unchanged.

Final local replay d0ccc11839a0 is the successful rebuild with the same header
fix; renderer/game code is unchanged from tested 3818c7cecff8. Existing replay
evidence is reused rather than repeating unchanged scenes. All owned build and
emulator processes are terminal. This remains a parity milestone, not completion
of the renderer or acceptance of physical frame targets.
