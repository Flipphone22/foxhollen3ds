# Native material shader bank, 11 October

The layered actor entry (197 instructions) and original diffuse-map entry
(73 instructions) now share a single 270-instruction DVLP allocation. Their
separate DVLE entries retain their existing uniforms and output contracts.
This is a shader ownership change, with no change to the material equations,
game state, geometry, textures, lighting approximation or target fencing.

Pinned Citro3D `source/base.c::C3D_BindProgram` compares DVLP pointers when
deciding to upload shader code. Parsing identical data twice would not provide
this benefit. The new owner parses once and gives both entries to the SDK;
uniform publication still occurs on entry switches. The pinned Melee renderer
binding/cache discipline was consulted. No private command submission or raw
register writes are introduced. After parent GPU retirement, shutdown binds
the longer-lived main program before releasing either bank entry, preventing
the SDK context from retaining a freed program pointer.

Actual SDK bind-function tests cover100 lifecycles/100,000 switches, retained
uniforms, a distinct-DVLP negative case, reference ownership and retirement,
under ASan/UBSan. The exact linked Azahar float24 program preserves655,360
sampled actor output lanes across32,768 vertices;332,928 map vertices pass the
source oracle. The actual ARM GPU-owner probe passes nine map image comparisons
(maximum0.502 colour levels), three same-queue consumer handoffs and allocation
reclamation. It does not establish physical GPU execution.

Serial palace replays with the exact shared build pass state assertions,
save preservation, log drain and owned exit. New/Old captures1410 were inspected:
world, Krystal, creatures, glow and HUD are present. New LIGHT MAPS ON/OFF adds
0.276392ms combined scene/submission CPU time, still above the provisional
0.20ms budget. Compared with the prior ON candidate, the reduction is only
0.025482ms; different animation phases prevent treating this as an established
speedup. New ON/OFF cadence59.8308/59.7841FPS and Old ON58.0027FPS are emulator
results, not console forecasts. No Old OFF pair was repeated.

New peak heap48,473,496/71,303,168B, minimum free linear18,368,384/25,165,824B;
Old heap40,935,984/50,331,648B and free linear3,716,352/10,485,760B. Both retain
4,822,016B free VRAM and72,796B measured main stack of524,288B. Process grants
remain Old80/New124MiB. These are scene-specific measurements.

Evidence: `material-shader-test`, `material-shader-owner-test`,
`material-bank-runtime-test`, and `artifacts/diagnostic/material-bank-map-v1`.
Normalc0e326ffc1da/replay6fb445712ba7 share332 native source files, the game
archive and checked shader programs. They were built locally, not deployed.
The lighting feature remains opt-in OFF; the cost gate and original-image
acceptance are still open. White Arwing, disappearing trees, fur, shadows and
physical filesystem stalls are not claimed repaired by this change.
