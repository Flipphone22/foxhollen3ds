# Prepared native vertex buffers

12 October 2026. Implemented and checked locally in shared normal
`d0955e721177` / CIA `29a2a6c6b8ef` and replay `104b76ce39eb` /
CIA `822f36835f95`. **Not deployed; physical acceptance and the sky repair
remain pending.** This supersedes the scanning experiment in
`NATIVE_BUFFER_PUBLICATION.md`.

## Architecture

World and actor draws now select one of ten immutable fetch layouts, then pass
live stream addresses to the common SDK buffer owner. Formats cover compact
and float world/actor storage, morph position/attribute streams, and optional
fur groom streams. They are separate from the already prepared vertex
attribute layouts. No geometry, animation or material data is rebuilt here.

The old draw loop called `C3D_GetBufInfo`, cleared the 152-byte descriptor, and
rebuilt its streams with `BufInfo_Add` each time. The new path translates the
one to three live addresses and updates offsets. It copies formats only on a
layout change, and clears retired stream descriptors. It does not scan all
36 register words as the rejected prototype did.

The SDK publisher retains the immutable layout identity. A repeated layout
publishes changed offsets only; a changed or unknown owner publishes the
complete descriptor. Ordinary SDK `GetBufInfo`, `SetBufInfo`, and direct
`Init`/`Add` mutation of the context invalidate CPU provenance. This includes
ordinary mutation after a prepared bind but before its draw. Unrelated local
descriptor construction does not invalidate the context. Initial context,
framebuffer invalidation and APT restore invalidate both CPU and GPU knowledge.

This is an extension of the common SDK owner, not direct application register
writes or another command queue. Pinned Melee binding snapshots and checked
draw-packet discipline were consulted. Index draws, shader/geometry boundaries,
cache publication, GPU retirement and resource lifetimes are unchanged. The
layout arrays have immutable program lifetime; stack or recycled layout
objects are explicitly unsupported.

Remaining CPU work belongs here: translate live addresses, reject invalid
storage, select the layout, update active offsets, and publish a changed owner.
The geometry/morph/groom owners still control their existing storage and fences.
There is no new worker, allocation, vertex copy, pass or shader instruction.

Linked storage is 280 bytes of read-only layouts plus 60 bytes of SDK BSS
(two pointers, three offsets and five counters). The previous scanning
prototype's 184-byte cache is removed. Game archive and exact 511-instruction
scene shader are unchanged. The new shared build has 444 captured source files;
SDK source hashes and commands, including the buffer owner, match between normal
and replay. Old 80 / New 124 MiB CIA profiles are verified.

## Evidence

`tools/test_prepared_buffers.py` compiles the actual previous world/actor draw
stanzas, actual morph-layout helper, new owner, and pinned SDK for ARM11. All
360,693 checks pass over mixed layouts, repeated binds, ordinary 12-stream
owners, address changes, invalid input, context loss and geometry-stage
handoffs. GPU register state and side-effect counts match the previous path.
Five deliberately broken variants fail: lost context invalidation, lost writer
invalidation, lost tail clears, lost offset publication and incorrect formats.
An initial harness compilation error (external SDK include and a shadowed local
name) is preserved in `initial-compile.log`; it was not a game failure.

Serial isolated cave checks completed with status 1, saves unchanged, logs
drained and owned emulator exit. Both frame-1410 captures were inspected:
world geometry/textures, Fox, ladder, mushrooms, torch, particles and HUD remain
present; the Old touch stick is visible. Particle haze differs by animation
phase, as in prior checks. New whole-run world/actor geometry totals match the
previous build; Old totals differ slightly with frame count.

| Azahar profile | Previous scene + submission | Prepared buffers | Change |
| --- | ---: | ---: | ---: |
| New | 7.365252 ms | 7.086145 ms | −0.279107 ms |
| Old | 7.366142 ms | 7.108670 ms | −0.257472 ms |

Native submission alone changes New 5.499612 → 5.235879 ms and Old 5.513532 →
5.250162 ms. New guest cadence remains 59.831 FPS; Old changes 57.140 → 57.888
FPS. These are short emulator checks with matching recipes/assets/settings,
not fixed-state A/B or console performance forecasts. This is a modest CPU
saving, not achievement of the physical frame-rate targets.

New recorded 210,296 prepared binds / 4,212 CPU layout changes; Old recorded
203,422 / 3,944. The remaining publications include full restores and ordinary
SDK consumers. These counts must not be treated as a universal timing ratio.

Memory peaks are unchanged: New heap 48,404,624 / 71,303,168 bytes, linear free
18,441,856 / 25,165,824; Old heap 40,867,984 / 50,331,648, linear free
3,761,792 / 10,485,760. Both VRAM free 4,723,712 and main stack 73,008 / 524,288.
Process commitment remains New 103,301,120 / 130,023,424 and Old 67,649,536 /
83,886,080. No console timing, GPU-utilization or physical acceptance claim.

The exact normal binary also passed startup, real touchscreen HUD/movie
OFF→ON changes, resumed movie publication and Select exit in 33.15 host
seconds. Captures 540 and 1080 were inspected: hidden movie surface first,
then resumed movie and copyright HUD. State masks were 24833 → 24577 → 16385;
241 movie commits / 1,440 draws were logged. This uses isolated preferences,
not either console's settings.

Reproduce with `build_prepared_buffers_v1.py`, `test_prepared_buffers.py`,
`check_prepared_buffers_v1.py old|new`, `check_prepared_buffers_normal_v1.py new`,
and `verify_prepared_buffers_v1.py`. Builders isolate the verified prior
snapshot and restore the 440 prior files and game archive in `finally`; new
source inputs are captured in the output snapshots. Do not blindly build the
unaccepted VRAM/finite-shadow worktree. Evidence is in `arm-prepared-buffer-tests`,
`prepared-buffer-runtime-test`, and the three `prepared-buffers-v1-*` native runs.

## Remaining work

Physical frame-rate and rendering-parity requirements remain open. This change
does not repair the sky, fur fidelity, finite shadow receivers, water reflections
or other incomplete material/effect families. Normal deployments to both
consoles remain conditional on the requested sky repair. Latest read-only
collection `results-20261011-214444-223` found no sky-probe result; no remote
files, saves or preferences changed.
