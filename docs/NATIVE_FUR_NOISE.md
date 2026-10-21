10 October deployment update: New 3DS normal `0b55ba943be8` and replay `bddd446fde84` uploaded and readback-verified at user request. Saves/settings preserved; Old 3DS unchanged. CIA copies require FBI installation. Earlier local-only wording below describes the pre-deployment checks. Physical startup recovery and fur acceptance remain pending.

# Native fur colour restoration

The original `objFuzzRenderCb` uses the RGB channels of its noise texture to
perturb the lit base colour, as well as its alpha channel for shell coverage.
The native path previously used only alpha. The local candidate restores the
source RGB equation with two precompiled PICA combiner stages:

`clamp(B + 0.5 - mix(noise, 0.5, B))`

`B` is the already-lit base colour. Stage 1 saves it in the delayed RGB combiner
buffer. Stage 2 interpolates noise towards half using `B`; stage 3 combines the
saved base with the complement using signed addition. PICA's 8-bit half constant
is 128/255. Across all 65,536 byte-valued base/noise pairs, the arithmetic differs
from the source real-valued equation by at most 0.00196075439, less than one
8-bit level. This is an arithmetic check, not a claim of exact GX integer
rounding or original-image equivalence.

There is no extra geometry pass, texture, vertex calculation, allocation or
shader instruction. Existing noise sampling and four material stages are reused.
The only additional CPU publication on a material change selects the RGB
buffer-update mask; alpha buffer updates are explicitly cleared. Cached program
reuse avoids repeating this setup. Native colour fade and fog stages stay in
their existing positions.

Pinned Melee's persistent raster/texture binding invalidation and the pinned
citro3d texenv publication/buffer-update implementation were consulted. This
uses SDK descriptors and the existing native material owner, with no direct
register writes or original GX interpreter. The delayed buffer must be restored
at material handoff; hostile-state and missing-buffer/complement/reset mutants
are tested.

## Evidence

`tools/test_pica_fur_material.py` runs the actual material owner with ASan/UBSan:
262,144 alpha combinations, 65,536 RGB noise pairs, 1,024 lighting/UV cases and
1,000 forced material handoffs. It checks the original source TEV selection.
All three mutation cases fail as expected.

Serial before/after New-profile cockpit runs use the same game archive, recipe,
assets, options and input. Both render 1,450 shells / 4,350 fur draws /
4,867,650 indices, with no unsupported fur materials in this scene.

| Azahar measurement | Before | After |
|---|---:|---:|
| Rendered FPS | 59.8314206 | 59.8314205 |
| Scene CPU, ms/frame | 0.7630475 | 0.7630436 |
| Native submission CPU, ms/frame | 1.8722256 | 1.8744012 |

The combined measured difference is +0.00217 ms/frame, within the 0.5 ms New
fur CPU budget; it is not a physical GPU cost estimate or a speedup claim.
Animation poses still differ in captures despite matching workload counters.
Both 900 images were inspected. Fox, cockpit and HUD remain visible; the
existing lighting/tint concerns and full original fur appearance remain open.

The Old-profile CIA content also completes using the custom PICA layout audit:
69,632 reported checked draws, zero layout violations; the counter is a prefix
with up to 4095 further checked draws unreported. Its host timings are excluded
from comparisons. Old 900 capture was inspected. Every run preserves saves,
passes replay assertions, drains logs and exits the owned emulator.

| Resource | New | Old audit |
|---|---:|---:|
| Peak normal heap / reservation, B | 47,856,072 / 71,303,168 | 40,516,040 / 50,331,648 |
| Minimum linear free / reservation, B | 18,950,272 / 25,165,824 | 4,270,208 / 10,485,760 |
| Initialized free VRAM, B | 4,846,592 | 4,846,592 |
| Process commitment / limit, B | 103,186,432 / 130,023,424 | 67,534,848 / 83,886,080 |
| Measured main stack / reservation, B | 72,844 / 524,288 | 72,844 / 524,288 |

New before/after memory observations are identical. Runtime evidence:
`artifacts/native/fur-noise-v1-{before-on-new,after-on-new,audit-old-on-old}`.
Machine-readable comparisons: `artifacts/research/pica-scene-rewrite/fur-noise-test`.

## Visibility policy and remaining work

FUR remains opt-in and defaults OFF on both models. ON allows the original
eligible-object and distance-selected fur passes; it does not force fur on all
characters or at all distances. `lightmap.c` queues eligible visible models and
`playerRenderFuzz` retains player-state checks. `objRenderFuzz` has a much
stronger distance reduction for ordinary objects than its close-up/special
cases; it returns when the computed shell count is zero. There is no general
cutscene-only condition in these paths. The native port preserves these
conditions and limits the admitted result to first/last shells, at most two.

Indirect wind/length masks, special shell/shadow callbacks, masked/layered
materials and full source-image/physical acceptance remain incomplete. This
change does not repair all face, eye or lighting differences. No console files
or saves were changed; the installed startup-fix candidate remains available.
