# Exact sharing of native diffuse light maps

11 October 2026. Local work, not deployed. The native renderer's optional
LIGHT MAPS path recreates the original game's diffuse environment images.
Its previously measured incremental CPU cost exceeded the provisional 0.20 ms
New 3DS gate. This change removes redundant map work without changing lighting
values, texture coordinates, original scene state or image filtering.

A bounded read-only observation of the existing palace replay captured 30
resident source-state triples at retraces1203–1492. Slots0 and2 were byte-identical
in every sample; slot1 differed. No guest state, inputs or clock were modified.
This diagnostic run is excluded from timing comparisons because observation
may perturb it. The full source bytes are retained in
`artifacts/native/diffuse-slot-identity-v1-ordered-on-new/diffuse-slot-identity.json`.

Three logical source slots now refer to a pool of three resident 32×32 images.
An assignment first reserves every existing image still needed by any logical
slot. It then allocates the remaining target indices for changed states,
sharing a pending draw when its complete source state is byte-identical.
Partial updates preserve untouched slots. A permutation can reuse existing
results without redrawing. There is no float tolerance, quantization, shader
change, atlas coordinate remapping or approximation of moving lights.

The existing ordered GPU owner still binds the native map shader, draws the
same immutable hemisphere mesh, splits to publish target writes, and restores
the scene before consumers bind samplers. Mapping changes are published after
that boundary. Prior consumers and new target writes share the same queue;
the existing shutdown fence precedes image release. The pinned Melee retained
binding/ownership rules and pinned Citro3D texture/framebuffer/queue publication
were consulted. No raw register shortcut or new worker is introduced.

Resident geometry remains8852 linear bytes and the three images remain12288
VRAM bytes. This saves draws, not image allocation. Source-state ownership is
848 CPU bytes versus832 for the former snapshots/valid flag, plus a4-byte
sharing counter. A32-byte temporary assignment and three temporary pointers
are bounded stack state. Remaining CPU work compares at most three small
source states per slot and chooses ownership; it does not generate vertices.

`test_pica_diffuse_slots.py` executes the actual native owner under ASan/UBSan.
It verifies301,290 logical-slot shader-input comparisons,100 randomized
lifecycles, duplicate slots, permutations, partial overwrites, unchanged-state
reuse, all published sampler results, and fenced retirement. The test rejects
both a missing publication boundary and recycling an image still needed by
another slot. Replaying the30 captured triples through that same owner produces
39 GPU map draws instead of66, with identical inputs for each logical result.
The host sink checks shader inputs and ownership; it does not render PICA pixels
or measure console speed. The actual map shader and 32×32 target/filter remain
unchanged, with previous shader/image checks retained for those components.

Serial replay9462cc49cea0 checks complete with status1, unchanged normal saves,
drained logs and owned-process exit. New and Old frame1410 captures were
inspected: palace geometry/textures, Krystal, purple glows/creatures and HUD
remain present. These images do not establish full original-image parity.

| Azahar profile | FPS | Scene CPU ms | Submission CPU ms | Map draws |
| --- | ---: | ---: | ---: | ---: |
| New, LIGHT MAPS ON | 59.87616 | 1.916961 | 4.323711 | 791 |
| New, OFF, same binary | 59.82397 | 1.828964 | 4.101160 | 0 |
| Old, ON | 57.48930 | 1.889762 | 4.334933 | 820 |

The New ON-minus-OFF CPU cost is0.310547 ms, still above the0.20 ms gate.
There is no matched Old OFF comparison. Workloads/animation phases differ;
neither slightly higher ON cadence nor the source-state diagnostic run is
used as speedup evidence. The latter drew1295 maps, but its timings were
perturbed by read-only inspection. Captured-state replay provides the stronger
66-to-39 workload comparison because its inputs match exactly.

New ON heap peak48,473,528/71,303,168 B; minimum linear free18,368,384/
25,165,824 B. Old ON heap40,936,016/50,331,648 B; linear free3,716,352/
10,485,760 B. Both retain4,822,016 B free VRAM; measured stack72,796 B against
524,288 B reserved. Process commitments103,206,912/130,023,424 B New and
67,555,328/83,886,080 B Old. The small static state change did not increase
the committed page count. These are palace measurements, not whole-game bounds.
New/Old record670/651 logical alias observations; these include unchanged
updates and must not be reported as an equal number of avoided draws.
Both record zero unready map samplers. No physical performance claim is made.

Evidence: `artifacts/research/pica-scene-rewrite/diffuse-slots-test/` and
`diffuse-slots-runtime-test/`. A reduction in these small map draws
alone is not evidence that the full frame target has been reached. LIGHT MAPS
remains opt-in; FS stalls, intermittent physical visibility defects, fur wind,
remaining lighting families and shadow fidelity are still open.
