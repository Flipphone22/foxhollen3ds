# Reusing particle pool admission

The native particle update already uses live pool views when no impact callback
can change the current or following pool. Callback-bearing updates retain the
original two-buffer snapshots, prefetch order and full writeback. Drawing also
already reads live pools while retaining the old scratch address for rotation
phase. Neither direct path is newly introduced here.

The new change removes a second callback scan for a view admitted as direct.
Such admission proves both the preceding pool and this pool callback-free. A
staged view is still scanned against the live mask: a preceding callback may
have activated more slots after its snapshot was taken. Retaining that rescan
is essential for matching the original update semantics.

`expgfx_view_scan_patch.py` modifies only the archived pool-view generator.
The per-slot simulation, RNG, callback, appearance and bounds statements are
byte-identical. No renderer program, vertex, allocation, thread, GPU fence or
publication mechanism changes. Pinned Melee command-cache publication was
reviewed; existing GPU resource owners remain intact. This optimization belongs
on the CPU because it governs mutable game-particle snapshot isolation.

The existing original-snapshot oracle exercises 16,384 updates and 60,009
callbacks, including cross-pool spawning and changes to active masks and
snapshot padding. Final bytes, RNG, event observations, direct/staged decisions
and copied-byte counts match. Test-only inspected-slot counts fall from
2,132,886 to1,275,781; neither count is a frame-time measurement. Mutants that
ignore staged callbacks or invert the view check fail. ASan/UBSan pass.

The complete generated parent translation unit matches the profiled DWARF MD5
40f170224d180650a6448ebcae584a62. The candidate differs only by the helper and
its call site. Its linked MD5 is e220bcc2b995d0ce5e6cc89e0253b919. Only expgfx.o
changes in the game archive; all18 GPU programs and installation recipes match.

Shared source5f4cfbf0d3d8 (3dsx061ff3eba462/CIA3030de9c239e) completed the Old80
palace replay in104.74 host seconds, with state/save/settings/drain/owned-exit
checks passing. Capture1601 was inspected: textured world, Krystal, purple
creatures/particles and HUD are present. Native DSP/LLE was verified. The
profile is not an elapsed-time benchmark. Prior New610d4b GPU/owner evidence is
explicitly reused; there is no fresh New candidate runtime.

The eight-frame update-function count falls1539569->1424377 with8 calls each.
DWARF attribution for the unchanged callback predicate falls82928->41072
encounters; mask extraction falls41464->20536. Active particle counts also
differ, so the entire update-function reduction is not attributable to the
patch. Raw whole-window work rises21929498->22215624 (+1.30%); audio jobs25->27,
particle draw calls298->275. This is neither an established full-frame speedup
nor an established regression. Do not subtract variable work to manufacture a
frame-time estimate.

Observed memory matches the parent: normal41859104/50331648, linear free1622784/
10485760, VRAM free4723712, stack72984/524288, process69595136/83886080. No added
allocation. All sampled limits have headroom, but this is not a full-game peak.
Old80/New124 CIA profiles, root/archive restoration and package checks pass.
The ordinary shared package is foxhollow-iso-setup-9039620c96034535.zip, not a new
probe app. It is NOT DEPLOYED. No FTP retries or console changes this turn.
Physical FPS, full rendering parity and progression/endurance remain unverified.
Evidence: `artifacts/research/pica-scene-rewrite/expgfx-view-scan-test/`.
Build: `build_expgfx_view_scan.py console-setup --revision v1`.
Runtime: `run_expgfx_view_scan.py benchmark --family unified-view-scan --revision v1 --old --profile`.
