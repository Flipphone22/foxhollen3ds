Frame-688 particle GPU isolation probe (9 October 2026)

V3 HARDWARE RESULT: Old3DS collection20261009-121426 confirms correct binary
d86c19b64d59 and COMPLETE. Single split GS->HUD stage0.925062ms; eight repeated
switches2.346066ms (isolated GPU scope, not gameplay FPS). Initial particle
readback is byte-identical to the earlier hardware capture. This validates the
boundary for this reproduction; the complete game replay still needs checking.

Current probe: v3 split GS-to-HUD, binary d86c19b64d59. The version is visible
on the bottom-screen menu and written to the log. Press A once.

Old3DS v2 (d4d9fd4b78a7) passed isolated particles, the original stream offset,
and textured particles, then timed out at the particle-to-HUD switch within
one command list. Evidence: artifacts/console-particle-results/20261009-115333,
confirmed unchanged by collection20261009-115807. This reproduces the hang
without game logic, saves or the private Citro3D modifications.

V3 queues a finalized command list with C3D_FrameSplit(GX_CMDLIST_FLUSH)
immediately before disabling GS. It adds no CPU completion wait or resource
retirement. Repeated HUD-to-GS transitions remain inside the next list, so
the test also exercises the reverse direction. This candidate passed both
short headless modes, including eight repeated switches, with unchanged
initial particle capture. Hardware success and its performance cost are
not established. No game/replay fix has been deployed.

This is a diagnostic, not a game fix or FPS benchmark. It runs on Old or New 3DS
and uses no game assets or saves. The small 3DSX uses 8 MiB normal/4 MiB linear;
it does not require the game CIA extended-memory profile.

Launch through Homebrew Launcher:
SD:/3ds/foxhollow-particle-probe/foxhollow-particle-probe.3dsx

Press A once for the normal shared-code test. It draws the eight captured
particles with the exact deployed shader, then exits automatically if successful.
If GPU completion times out after three seconds, it displays "resources retained"
and deliberately stays there. It will not recover by waiting: reboot and reopen
ftpd. This prevents unsafe destruction of resources still being read by the GPU.

X selects the explicit geometry-code upload comparison. Run that only when
requested after the first result. Each launch currently overwrites probe.log;
let the first result be collected before another launch.

Logs/capture: SD:/3ds/foxhollow-particle-probe/{probe.log,color.rgba}

Both modes passed silent headless Old3DS checks in under four host seconds and
produced identical images (189 white particle pixels on a dark background).
Emulator success does not establish hardware safety. Shader/input identity:
capture 3863146450a9, deployed 442-instruction shared shader, 8 x 36-byte records.
The probe first exercises a VSH-only -> geometry-stage switch, then the captured
draw. The updated probe then adds the original vertex offset1024, captured32x32
texture, particle->HUD switch in one command list, and eight repeated switches.
Every stage logs BEGIN/PASS. Two unchanged HUD view rows are inferred identity
rows from the native caller. Earlier frame draws, private SDK adaptations,
game workers and resource eviction remain outside this isolated test.
A failure is a specific shader/binding hardware question, not a save-load test.

First hardware run (probe b95901357cbe8): Old3DS completed; collected113755.
Hardware image196whitepixels differs substantially in placement from emulator
189pixels. Do not interpret emulator image equality as hardware correctness.
The v2 probe d4d9fd4b78a7 and its failing hardware evidence remain preserved.
