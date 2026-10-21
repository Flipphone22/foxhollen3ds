# Native normal-demand experiment

Local research candidate, **not promoted or deployed**. Continue stable work
from `actor-live-v2-builds` and `actor-live-normal-v1-builds`. Neither the prior
directional-light experiment nor this experiment has established a net scene
GPU or physical FPS improvement. Hardware is unavailable; no probe is requested.

The shader previously transformed actor normals whenever vertex lighting was
enabled, including ambient-only objects. The candidate publishes a separate
normal-demand bit after native material publication. Active lights with enabled
vertex lighting, environment mapping, or specular UV generation require normals.
An empty light list alone is insufficient to skip them.

All sixteen boolean registers were occupied. Investigation found that UV1
selection is still significant for layered and special materials, despite the
compact loader duplicating UV0 when a secondary source is absent. That flag is
preserved. Instead, the sign of the existing colour decode scale carries the
vertex-versus-constant RGB material choice. A negative scale selects constant
RGB; the shader restores positive vertex alpha and retains separate alpha-light
semantics. Cloud rendering keeps its unchanged program and original material
boolean. UI and dedicated shadow programs are unchanged.

The original lower light-accumulator clamp is redundant in the admitted domain:
ambient is nonnegative, diffuse factors are clamped to zero, and each source
light RGBA channel is clamped to the byte range before native publication.
The upper RGBA clamp remains. This is not permission to admit signed light
colours without redesigning the shader. Main shader storage falls from192to191
instructions; the actual linked actor/HUD bank falls from511to510. All96float
and16boolean registers remain occupied. There is no new shader bank, draw,
geometry preparation, GPU storage, worker or game-archive change. Four counters
add24bytes of static state; observed process commitment rises by one4KiB page.

V1 decoded signed colours on every actor vertex. V2 moves its three corrective
instructions inside the lighting branch. Exact linked Azahar float24 tests
compare65,536vertices for each of ordinary, layered and GPU-fur programs,
covering ambient-only, active lights, environment, specular, unlit, skin blends,
material selection, alpha and both UVs. All28output lanes are exact against the
retained shader for the sampled inputs. Wrong-sign and missing-map-demand
mutants fail. This is sampled shader equivalence, not whole-game visual parity.

For the sampled ordinary shader traces, aggregate instruction counts are:

| Input class | Retained shader | V1 | V2 |
|---|---:|---:|---:|
| Ambient only | 7,854 | 6,990 | 6,990 |
| Active lights | 11,196 | 11,432 | 11,432 |
| Environment map | 9,592 | 9,828 | 9,828 |
| Specular UV | 7,830 | 8,067 | 8,067 |
| Unlit | 5,164 | 5,544 | 5,259 |

These are totals over different sampled classes, not per-frame totals, weighted
scene costs or milliseconds. Ambient-only traces fall about11%; other paths
are longer. V2 removes three V1 instructions per unlit actor vertex, but still
has an extra branch compared with the retained shader. No net GPU saving is
claimed without representative path weights and cache behavior.

The actual native pre-draw publication block and pinned citro3d boolean setter
pass786,432mask/material/pipeline handoffs under ASan/UBSan. The missing
publication mutant fails. Cloud restores its old slot3 semantics; UI and shadow
paths leave it alone. V2's native source/header are byte-identical to V1, so
that evidence is reused. Pinned Melee shader-output verification practices and
the SDK pending-uniform owner were consulted; no retired CPU renderer is used.

V1 replay425951271596/CIA741dda77cf4e and V2 replaydbcc25d52a1e/
CIA5cee88bdc599 are built locally. Both retain the same game archive and shared
Old80/New124CIA profiles. Only ordinary/layered/fur GPU programs differ from
actor-live; the other ten are identical. All458snapshot source hashes are
verified. Builders restore replaced/absent/shelved files and archive in finally.

Serial unattended tests complete with state assertions, unchanged saves, log
drain and owned-process exit:

| Route | Retained Azahar FPS | Candidate Azahar FPS | Disjoint render CPU change |
|---|---:|---:|---:|
| V1 New Thorntail movement | 53.58905 | 53.40962 | +0.048221 ms |
| V1 Old cave, shadows on | 57.43225 | 57.38154 | +0.019510 ms |
| V2 Old cave, shadows on | 57.43225 | 57.34439 | +0.035361 ms |

The CPU sum is scene preparation + native submission + object rendering.
Differences are small, with differing animation phases/work counts. They do
not demonstrate a speedup or predict console performance. V2 has no New runtime
measurement; V1 New is explicitly separate. Captures V1 New1650and Old1410,
V2 Old1410 were inspected: world, player, HUD and shadow remain visible, with
the Arwing in New and touch item pad in Old. These are regression observations.

V1 New normals are requested for24,684draws/13,256,426declared vertices and
skipped for34,227draws/3,474,788declared vertices. V2 Old requests18,217draws/
5,405,795vertices and skips70,320draws/6,614,068vertices. **Skip counts include
draws that were already unlit.** They are not newly avoided work or actual GPU
vertex invocations. This telemetry does not establish the weighted GPU benefit.

V1 New heap56,566,280/71,303,168bytes, linearfree16,033,280/25,165,824,
VRAMfree4,723,712, measured main stack73,068/524,288,
process103,325,696/130,023,424. V2 Old heap40,973,432/50,331,648,
linearfree3,759,104/10,485,760, same VRAMfree, stack73,008/524,288,
process67,674,112/83,886,080. No new resident GPU allocation is introduced.

Evidence: `normal-demand-test/{assembly,shader,publication,build-verification,
runtime}.json` and `normal-demand-v2-test/{shader,build-verification,runtime}.json`
under `artifacts/research/pica-scene-rewrite`. The first VM compile failed on an
Azahar BitField assignment; using its underlying configuration word fixed it.
The first V1 New run stopped at the disk guard before emulator launch. Completed
old ELF files were losslessly compressed, with byte hashes retained. Nothing
was deleted to bypass the guard; compression/builds did not overlap timing.

No matching normal build or deployment is justified yet. Do not run more long
boots merely to average these small differences. Resolve the mixed-workload
tradeoff offline before promotion; the current whole-run counters cannot do so.
The existing native draw-capture tool deduplicates and caps by material/program,
so its output is not a representative frame histogram without extending it.
Broader visibility/submission work and remaining visual defects stay open.
