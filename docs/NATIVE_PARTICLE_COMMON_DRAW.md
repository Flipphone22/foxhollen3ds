# Common particle draw specialization

The 19 October experiment uses the complete generated `drawGlow` body from
`particle-common-update-v2-console-setup-builds` (source `df5668002cdc`). Two
live per-slot behavior/render pairs select copies of the source expressions:
`20280101/822` and `80201/4040800`. The generic path remains unchanged. No pool
homogeneity is assumed. Slot iteration, rendering-off RNG, material descriptors,
stream commit, resource retirement and the GPU programs remain intact.

This is **not integrated or deployed**. It is a tested experiment, not an FPS fix.

## Evidence

`particle-common-draw-test/host.json` records 30,000 complete producer
comparisons under ASan/UBSan, including disabled rendering and changing masks.
`guards.json` adds 30,000 comparisons with near-match flags and nonzero geometry,
UV and colour fields. Ignored behavior/render guards and duplicate common draws
are rejected. External services are modeled; these are not live GPU tests.

The paired ARM fixture uses the game compiler options, actual draw bodies and
modeled external calls. It completes 384 calls per producer with matching output.
Self instruction encounters total 156,947 -> 150,096 (-4.365%). By fixture group:

| Group | Prior | Candidate |
| --- | ---: | ---: |
| Random flags | 35,043 | 34,677 |
| 20280101/822 | 64,438 | 60,715 |
| 80201/4040800 | 57,466 | 54,704 |

These groups include empty masks and rendering-off cases. They are synthetic,
not captured frequency weights. Counts exclude external callees and instruction
cache effects; they do not establish cycles or whole-frame improvement. The
inherited report mentions a descriptor helper; neither linked producer has a
separately counted descriptor helper here, as its counts dictionary confirms.

Independent compilation of the complete production translation unit grows
`drawGlow` from 2,380 to 4,772 bytes. Candidate TU SHA-256:
`dc26fd64f7039d10adf941aa7ec4db11d899f1e86ddd4430181bf82a9ec2f5bb`.
The compilation report's original inherited scope label says world-position;
its source hashes and named `drawGlow` sizes identify the actual test. The
compiler wrapper has been corrected for subsequent invocations; the original
result is retained rather than silently rewritten.

## Decision

Do not promote this duplicated draw body: the limited synthetic saving does not
justify doubling the function and an additional hardware test. Particle drawing
was about 6% of the eight-update main-core instruction sample, so even a similar
4.4% reduction there would be a small portion of total work; that arithmetic is
not a hardware FPS estimate. The larger guarded particle **update** improvement
in `df5668002cdc` remains separate and is not reverted by this decision.

Pinned Melee `source_snapshot_cache.h` ownership rules were consulted. No cache,
allocation, worker, GPU program, live resource owner or private asset changes
were made. Old `4f58d475c8f7` and New `d478192e76be` uploads remain unchanged.
No full game boot or console probe was run for this unpromoted experiment.
