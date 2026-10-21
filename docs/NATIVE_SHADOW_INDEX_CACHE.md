# Native shadow-mask vertex reuse

13 October runtime correction: the first V1 build is **visually rejected**.
Its cave replays completed and counted all receivers, but the inspected New/Old
captures lacked Fox's silhouette. The builder selected a worktree compact-draw
header that omitted the existing `shadow_active==2` capture branch. It applied
planar receiver stencil/depth/blending while drawing to the mask target. Ordered
triangle equivalence and draw counters could not detect that publication error.

V2 restores the exact previously working capture branch: no stencil/depth test,
opaque-white replacement, six clean TEV stages, fog disabled. It retains the
capture-only index ordering and compact resource caps. No extra pass, geometry,
worker or allocation. Pinned Melee cache/segment publication remains unchanged.
The actual state owner passes1,024 capture/planar publications under ASan/UBSan;
removing the branch fails. Build composition and packaging now reject that same
mutant, including when capture dispatch was already patched. Historical V1 build
verification was incomplete; the updated verifier rejects it.

Repaired shared replay `40fd2bdeb32e` / CIA `9b32a2f6e4b7` lives in
`shadow-index-cache-v2-builds`. Normal setup `4e24995bc96d`, 3DSX `9f66f0f47b9c`
/ CIA `c8f0f910f4ea`, lives in `normal-setup-shadow-capture-v2-builds`. Each exact
archived source comparison changes only the capture header; game archive and
linked shaders match its preceding build. Both retain Old80/New124 profiles.
The normal converter and recipe are unchanged; no new full conversion was run.

Serial New/Old V2 cave replays pass state/receiver coverage, saves, log drain and
owned exit. Both1410 images were inspected: Fox, cave, HUD and the shaped shadow
are present; fog/animation phases differ, so they are not pixel-equality proof.
New scene/submission1.941244/5.370548ms; Old1.926553/5.405284ms. Against historical
visible-shadow sharing runs, combined CPU changes +0.006665/+0.018728ms. These
include prepared-load/index changes, not an isolated or physical speedup claim.
Azahar cadence59.83149/57.36858FPS is not console performance. Full shadow budget,
fidelity and actual hardware cache benefit remain open.

New heap48,510,064/71,303,168B, linear free18,583,552/25,165,824B;
Old heap40,973,424/50,331,648B, linear free3,903,488/10,485,760B.
Both VRAM free4,723,712B and measured stack73,008/524,288B. Process commitments
103,329,792/130,023,424B and67,678,208/83,886,080B. Thirty compact plans loaded
without declines; peak compact linear324,688/524,288B. No resident-memory gain
over the sharing baseline. New715casters/5,005receiver draws/9,493,770points;
Old684/4,788/9,082,152. Eleven receiver plans, zero missing/declined on both.

Evidence: `shadow-capture-state-test/{result,runtime}.json`, isolated build
`scope.json`/`restored.json`, and `shadow-index-cache-v2-cave-on-{new,old}`.
ISO-only package `0996053d1fec9b0f` is local and selector/identity tests pass.
Earlier package `191072361db3f64b` is withdrawn; the transfer tool rejects its
local withdrawal marker. No console files changed. The sections below describe
the implementation and pre-runtime evidence; V1's pending acceptance is now
superseded by the failure and repair above.

The compact shadow compiler now orders whole triangles for vertex reuse during
asset admission. It adapts the pinned meshoptimizer FIFO algorithm to C,
16-bit indices and caller-owned, bounded workspace. No vertex, bone binding,
weight, bind offset, winding or triangle multiplicity changes. There is no
per-frame sorting, allocation, geometry rebuilding or additional draw.

This stream is restricted to native shadow-image capture (`shadow_active == 2`).
That pass writes constant white with replace blending, no depth/stencil test,
no culling and no fog. Its result is the union of triangle coverage. Ordinary
materials, transparent surfaces, HUD, fur and fogged planar draws continue to
consume their original native geometry. Reordering those passes indiscriminately
would not be correct. The capture dispatcher and compact draw owner both enforce
the mask restriction.

The linked prior game binary already discards `fh_pica_scene_shadow` (the old
direct planar entry); `fh_pica_scene_shadow_capture` remains linked. The new
explicit restriction also keeps any future direct planar consumer from using
the reordered compact stream. It does not remove geometry from that consumer.

The first local candidate retained two index streams. The final candidate
instead reuses the compact mask stream and its dead admission hash workspace;
the original parent asset remains untouched. Thus resident GPU bytes and draw
descriptors match the previous compact plan. The existing 512 KiB global
compact-plan linear limit, 64 KiB CPU-metadata limit, normal-memory reserve,
cache flush and reader-retirement fence remain in force. No GPU cache register
or command-submission protocol changes. Pinned Melee `command_cache.c` and
citro3d `drawElements.c` were consulted; cache publication stays after all
preparation, and each draw retains the SDK's post-vertex-cache reset.

Temporary workspace increases: the actual 835-model collection peaks at
428,778 bytes, versus 157,666 bytes for the prior host preparation. A synthetic
32,768-vertex/32,760-index batch uses 797,416 bytes. This is normal-memory
admission scratch, released before GPU publication, not persistent linear RAM.
The reserve check charges it before allocation. The reorderer itself has a
112-byte ARM stack frame; complete loading-stack depth remains unmeasured.

The independent reordered-index test matches the pinned upstream FIFO output
exactly across all 1,040 compact draws. It preserves 984,036 indices / 328,012
oriented triangles. Modeled shader invocations, compared with the **already
deduplicated** compact plan (excluding the earlier sharing win), are:

| Modeled FIFO entries | Prior | Reordered | Reduction |
| --- | ---: | ---: | ---: |
| 4 | 517,670 | 489,909 | 5.36% |
| 8 | 489,459 | 321,766 | 34.26% |
| 16 | 458,649 | 245,539 | 46.46% |
| 32 | 421,198 | 233,805 | 44.49% |
| 64 | 374,215 | 223,602 | 40.25% |

These are collection-wide FIFO models, not a measured PICA cache policy,
scene-frequency-weighted GPU work, frame time or FPS. Some individual groups
can regress under a particular cache model. Real gains depend on the hardware
and how much shadow capture costs in the active scene. This does not reduce
visible-world shading or improve rendering when shaped shadows are disabled.

Validation completed locally:

- The native helper matches upstream on 120 additional randomized/max-capacity
  cases under ASan/UBSan, including degenerate triangles and disconnected
  components. It rejects 724 invalid-range/alignment/overlap/capacity cases.
- Prior/current compilation across all 835 models preserves all vertex bytes,
  palettes, descriptors, oriented triangles and 6,959,312 collection GPU bytes.
  Repeated emission proves reused workspace does not destroy the retained map;
  1,670 output-capacity failures leave destinations unchanged.
- The original-source compiler oracle passes 2,505 malformed/capacity cases.
  Maximum-batch skin/weight/tail tests and the position-only-weld mutant pass.
- The actual owner passes 100 delayed-reader load/retire cycles, allocation
  pressure and retirement checks; removing the retirement fence is rejected.
- Native helper and integrated compiler compile for ARM with strict warnings.

Evidence is under `artifacts/research/pica-scene-rewrite/`:
`index-cache-test`, `index-cache-prepared-v2-test`,
`index-cache-compiler-v2-test`, `index-cache-share-v2-test`, and
`index-cache-lifetime-v2-test`. The earlier dual-stream candidate's separate
reports remain for comparison; it is not the final implementation.

The isolated shared builders include the new dependency and apply the capture
restriction to the checked archived renderer, restoring unrelated work after
building. Whole-game runtime, screenshots, load-time cost, actual scene memory
admissions and physical performance remain pending. The ongoing full ARM ISO
conversion is not restarted or replaced to run this candidate. No console files
have changed, and the overall rendering/playability/performance goal is unmet.

Shared local builds are complete: replay `9ca29321c11f` / CIA `35b68750da43`
in `shadow-index-cache-v1-builds`, and normal setup `9d3230ca03e8` / CIA
`8be8cf4a4a42` in `normal-setup-shadow-index-cache-v1-builds`.
`tools/verify_shadow_index_cache_builds.py` passes: matching renderer sources,
unchanged game archive and linked shaders, capture-only admission, discarded
unused direct-planar entry, restored worktree, and Old 80 MiB / New 124 MiB CIA
profiles. Normal setup recipes are unchanged from the preceding local ETC-cache
build. Neither new build has been run in Azahar or deployed.

After the owned conversion run finishes, the existing cave runner accepts
`new --build-dir /work/artifacts/research/pica-scene-rewrite/shadow-index-cache-v1-builds/console-replay
--label shadow-index-cache-v1-cave-on-new` (and an equivalent separately labelled
Old run). It retains receiver coverage, compact-plan consumption, save/drain and
owned-exit checks. Compare with the archived prior geometry, inspect captures,
and account for admission cost and memory before requesting hardware evidence.
