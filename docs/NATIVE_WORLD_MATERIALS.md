# Ordinary world material publication

The local native renderer now has a shorter binder for ordinary world materials.
It consumes the existing load-compiled TEV program, resident samplers and live
tint/UV/frame parameters directly. Actor lighting, generated maps, texture
swizzles, cloud overrides, post-material overrides, UI, moon and movie materials
continue through their corresponding native handling. This does not reconnect
the retired GX interpreter or reconstruct geometry.

The eligibility check uses current material fields, so a live callback changing
the material cannot leave a stale classification. It adds no allocations,
geometry, texture data, shader instructions, worker or draw. A single four-byte
counter records use and is logged once at shutdown. Existing native SDK state
ownership, invalidation, texture routing and lifetime checks remain intact.

The pinned Melee `renderer.c` retained raster state/invalidation and
`citro3d_fix.c` texture binding were consulted. Foxhollow continues using its
verified SDK adapter. In particular, the third sampler is still explicitly
disabled and its coordinate owner restored; this optimization does not remove
publication merely because the new material does not sample that unit.

`test_pica_world_material.py` compares the actual previous C binder and the
candidate at the SDK boundary: 32,768 cases, 2,140,852 identical argument words,
327,680 special-family exclusion checks. Cases cover live tint and scrolling,
animated samplers, packed/float vertex formats, blending, alpha/depth state,
texture toggles, hidden materials, and poisoned prior program/colour state.
ASan/UBSan pass. Deliberately deleting the third-sampler reset fails the test.
This is equivalence to the existing renderer, not proof of original-game parity.

The accepted `prepared-buffers-v1` snapshot is the comparison baseline. The
candidate changes exactly two source files: the material owner and new helper.
The game archive, assets, shaders and SDK remain unchanged. The isolated builder
shelves later experimental source files outside that snapshot, restores all
source/archive bytes in `finally`, and verifies the resulting source difference.
Do not build the unrelated VRAM/finite-projector/shared-shader WIP directly.

| Cave replay measurement | New profile before → after | Old profile before → after |
| --- | --- | --- |
| Scene preparation | 1.850266 → 1.860724 ms | 1.858508 → 1.847909 ms |
| Native submission | 5.235879 → 5.110383 ms | 5.250162 → 5.130779 ms |
| Combined difference | −0.115038 ms | −0.129983 ms |
| Azahar cadence | 59.831225 → 59.830961 FPS | 57.887532 → 58.399721 FPS |

These are short emulator comparisons, **not console performance**. Particle and
animation phases differ. New world draw/index counts match exactly: 117,405
draws, 30,887,925 indices. Their accumulated submission timer drops about 6%.
Old totals differ with simulation/render phase; 114,303 candidate world draws
use the specialization. Both runs complete replay/state/save/drain/owned-exit
checks, and captures 1410 were inspected for world, Fox, HUD, textures and mist.

Memory is unchanged in the sampled windows: New heap 48,404,624/71,303,168,
linear free 18,441,856/25,165,824, process 103,301,120/130,023,424 bytes; Old heap
40,867,984/50,331,648, linear free 3,761,792/10,485,760, process
67,649,536/83,886,080. Both retain 4,723,712 bytes free VRAM and measure
73,008/524,288 bytes of main stack.

Shared normal `1a3c2eb10639` / CIA `b931b0c7d0d8` and replay `5154b4b36924` /
CIA `6e917011b3d9` are local only. Their 445 source files, game archive and active
shader match; Old 80/New 124 MiB CIA profiles are retained. Evidence is under
`artifacts/research/pica-scene-rewrite/world-material-test`, with immutable build
snapshots in `world-material-normal-v1-builds` and `world-material-v1-builds`.
The exact normal build also completes the New-profile startup/touch test in
33.07 host seconds: HUD/movie mask transitions 24833 → 24577 → 16385, 241 resumed
movie publications and 1,440 movie draws, clean Select exit and drained logs.
Captures 540 and 1080 were inspected: video/HUD hidden, then restored. Old normal
startup was not repeated; the shared implementation and Old replay were checked.

This is a modest CPU saving. It does not close the frame-rate target, remaining
rendering gaps or physical-console acceptance. Both requested deployments still
need the separate skybox repair; neither console has been changed.
