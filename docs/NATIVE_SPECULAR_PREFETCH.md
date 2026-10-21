# Source-boundary half-map preparation, 14 October

The revised native scheduler is implemented and available in shared normal/setup
and replay candidates. It is **not deployed**. Correctness/workload checks pass;
its timing gate and physical performance remain open because unrelated host
compilation overlapped both emulator runs. This is not a shadow-flicker fix.

The earlier `NATIVE_SPECULAR_DEMAND.md` experiment deferred full-map generation
until actor submission. That reduced half-map copies but increased measured New
rendering CPU time by 0.054 ms/frame. This revision keeps full-map production at
the original source lighting boundary. Each material set has a load-compiled
half-map slot mask. Actual half-map use predicts which copies should accompany
the next full-map update. Demand expires independently for each updated slot.
A newly visible material gets an immediate copy before that actor's lighting,
camera, palette and material uniforms are uploaded. No frame is skipped or drawn
with a deliberately stale map, and getters never initiate GPU work.

Prediction may produce one unused copy when demand disappears. It avoids the
repeated late target switches of the prior design while ceasing speculative
copies at subsequent unused boundaries. Full-map changes invalidate stale half
maps. Group updates cannot clear another slot's pending demand. Controls OFF
perform no map work. Full-map equations, original source snapshots, two-channel
interpolation, clamp-before-halving, GPU ordering and shutdown retirement remain.

The existing main queue and `FrameSplit(GX_CMDLIST_FLUSH)` publication are reused.
Pinned Melee `gpu_efb_copy`, `citro3d_fix.c` and `early_queue.c` inform target/cache
ownership and complete retirement. No new GPU register bypass, queue, worker,
shader, texture, CPU vertices, readback, per-frame allocation or geometry pass is
introduced. Four bytes per material and small owner counters/masks are added.
Both map target sets remain resident; there is no VRAM or linear allocation win.

Actual C/SDK-stub tests under ASan/UBSan cover 100 lifecycles and 100,000 predicted
updates, initial/changed demand, expiry, group isolation, immediate misses,
controls, ordered writes and retirement. Missing publication, stale-half,
no-expiry, group-loss and missing-repair mutants fail. The actual material
compiler passes 36 aggregate-mask/preflight cases. Existing half-owner lifecycle
tests and 1,671,168 original-source RGB equations also pass. The fixture's
post-light SDK enum/header was updated to match the existing compiler. No
production equation changed to satisfy a test.

Serial palace replay results:

| Work across complete run | New mode | Old mode |
|---|---:|---:|
| Full-map updates | 806 | 792 |
| Half-map copies | 553 | 542 |
| Half-map request hits / late repairs | 3,086 / 4 | 3,029 / 4 |
| Specular consumer draws / unready | 24,017 / 0 | 23,158 / 0 |

The former eager rule would copy once for every full update in these runs. The
revision therefore avoids 253/250 copies in this auxiliary stage. **That is not
a comparable reduction in total scene work or FPS.** Animation and startup phase
counts differ between runs. Captures at 1410 were inspected in both modes:
Krystal, the palace, purple enemies, effects and HUD remain present. State
assertions, saves, log drain and owned exit pass; full visual parity is not proven.

Both runs observed unrelated SMG compiler activity and explicitly exclude timing
attribution. Linked ARM disassembly confirms the demand-hit path has no GPU, FS,
or allocation call; it is static path evidence, not physical cycle counts. No
additional hardware probe or unchanged emulator boot was requested for this gate.

Sparse memory measurements, bytes:

| | New | Old |
|---|---:|---:|
| Heap used / reserved | 48,598,456 / 71,303,168 | 41,061,792 / 50,331,648 |
| Linear free / reserved | 18,036,224 / 25,165,824 | 3,356,160 / 10,485,760 |
| VRAM free | 4,723,712 | 4,723,712 |
| Measured main stack / reserved | 72,972 / 524,288 | 72,984 / 524,288 |
| Process commit / grant | 103,354,368 / 130,023,424 | 67,702,784 / 83,886,080 |

Builds under `artifacts/research/pica-scene-rewrite/`:

- `specular-prefetch-v1-console-replay-builds`: source `b9cf14dd9c44`, 3dsx
  `410d9f67bd6c`, CIA `98d05310a10e`.
- `specular-prefetch-v1-console-setup-builds`: source `5a16f2fa68f0`, 3dsx
  `5575157b6eda`, CIA `2bd4b6f67991`.

All 596 source inventory entries, game archive `9d521d51f66e`, and 17 linked
shaders match between variants. The six changed native files are isolated from
the heat-v3 stable snapshot; worktree/archive restoration is verified. Shared
Old80/New124 CIA profiles and installation recipes are unchanged. No fresh
normal-app boot or conversion was run for these candidates; replay coverage and
unchanged startup evidence remain distinct.

Reproduce with `build_specular_prefetch.py`, `run_specular_prefetch.py new|old on`,
`test_pica_specular_prefetch{,_material}.py` and `analyze_specular_prefetch.py`.
Evidence: `specular-prefetch-test/{owner,material,integration}.json` and linked
ARM disassembly. The working tree carries this revision, but do not blindly build
it: unrelated VRAM work is still present. Use isolated builders. The prior heat
snapshots remain preserved as the performance reference until cost is established.

Read-only FTP collection `console-visual/20261013-175106-223` completed; every
collected file matches `20261013-163615-223`. Installed New223 stays f2c4fafba819,
and the saved gameplay log still predates that installation. Saves/preferences/
assets were untouched. Physical rapid whole-shadow dropout remains unconfirmed.
