# Independent changed-region reduction experiment

This changes the preparation algorithm in an isolated candidate, not the live
renderer, source assets, simulation, collision or installed geometry.

The original reducer discards a whole candidate when any region fails source
fidelity. The experiment computes the oriented-face multiset difference between
the original and proposed triangles. Exact unchanged faces remain unchanged.
Changed faces sharing any source vertex form a region; added faces connect their
source regions as necessary. Each region passes the original distance, UV, RGBA
and geometric-normal tests in every material context, or all its original faces
are restored. Protected morph/bone faces remain protected. The assembled list
still passes the original protected-face, storage and whole-list fidelity gates.

There are no new vertex values, approximate bone bindings, changed tolerances,
GPU resources or per-frame work. The pinned Melee resource-publication policy
remains untouched. The work happens before native geometry publication, so it
does not introduce new cache-coherence or retirement rules.

The first candidate retained fewer savings than the baseline. It stopped
refinement after restoring a rejected region, and changed face order even when
no region needed repair. Version 2 preserves an entirely accepted candidate's
original index order and retains tighter-error retries after a full restoration.
Both versions and their source snapshots remain under `artifacts/research`.

## Results

| Fixture | Source triangles | Existing reducer | Region candidate v2 |
|---|---:|---:|---:|
| Established 40-model/85-list fixture | 15,139 | 14,879 | 14,873 |
| Gameplay-area fixture | 28,474 | 28,424 | 28,394 |

All accepted candidates pass ASan/UBSan, runtime scene-packet compilation and
independent host surface comparisons. The region reducer also compiles for ARM.
The gameplay-area checks reuse the exact previously compiled sanitizer-enabled
executables and record their hashes. Native processing for that fixture took
0.944 and 0.879 host seconds respectively. These single host runs do not establish
console conversion speed, frame cost or physical FPS.

The earlier fixture's world geometry all comes from `animtest`. The added fixture
contains the largest eligible lists from two blocks per `shipbattle`, `swaphol`,
`warlock`, `icemountain`, `shop` and `magiccave` directory, plus original models
1, 46 and 465. It is not a frequency-weighted captured frame or whole collection.
The initial fixture script incorrectly assumed all three actors were in root
`MODELS.bin`; it stopped before executing a reducer. The corrected script searches
indexed model banks and records each actual bank and CRC. No game assets changed.

The candidate gains thirty triangles overall in the added fixture, with one list
retaining two more triangles than the existing reducer. It is **not promoted**
to the normal application or converter recipe. The improvement is too small to
address the performance target and the new scratch allocation has not undergone
a complete Old 3DS installation budget test. The existing shader/game/archive
and latest local normal build `17d50ce53c4c` are unchanged.

Reproduction:

- `tools/prototype_install_reduce_regions.py` writes an isolated copy including
  `tools/reduce_delta_regions.inc` and runs the established independent checks.
- `tools/check_reduce_play_areas.py` compares the preserved binaries on the
  explicitly selected gameplay assets without recompiling or editing them.
- `install-reduce-regions-prototype`, `install-reduce-regions-v2` and
  `install-reduce-play-areas-v2` preserve output bytes, source identities, failure
  history and comparison reports.

This evidence limits the usefulness of further variants of the present reduction
strategy under its current fidelity constraints. A separate, previously recorded
index-order audit in `NATIVE_VERTEX_CACHE_AUDIT.md` predicts a much larger change
in vertex invocation count without changing any triangle or vertex. Its cache
model is not hardware timing. Any native integration must explicitly handle live
blending, equal-depth overlaps, shared index ranges and morph ownership; universal
triangle reordering is not safe. No such integration or physical gain is claimed
by this experiment.
