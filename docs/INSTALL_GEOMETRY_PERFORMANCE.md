# Native startup geometry: performance acceptance remains open

The full native converter currently removes only **1,682 of 1,383,658 source
triangles (0.12%)**, changing 80 lists on 79 model variants. Both original and
performance packs contain all 2,102 renderable source packets; eight additional
source records are non-rendering. These are unique-source counts, not frame
workload or a predicted FPS change.

Do not replace the deployed performance assets on the assumption that this
new converter supplies equivalent reductions. The prepared installation remains
local and unactivated. Shape, texture, animation and gameplay acceptance still
apply, alongside runtime budgets.

Evidence: `artifacts/research/install-performance-full-build/audit.json`.
Original geometry occupies 50,971,512 bytes and the candidate 50,926,092 bytes.
These are collection sizes, not resident memory.

## Why refinement loses savings

`tools/audit_install_reduce_attempts.py` instruments an isolated copy of the
actual C reducer. Its 40-model, 85-list fixture, candidate bytes and final result
rows match the earlier production test exactly. Production sources, acceptance
limits and existing conversion journals are unchanged.

Of 44 actor lists, 22 produce no reduction on their first attempt. Of 41 world
lists, 17 initially produce no reduction. Initial candidates remove 1,453 actor
and 1,012 world triangles, but most fail the unchanged quality checks. Recorded
rejected attempts include:

| Rejection | Actor attempts | World attempts |
|---|---:|---:|
| No nearest surface with a compatible geometric normal | 21 | 8 |
| Surface distance | 2 | 30 |
| UV or colour interpolation | 1 | 10 |

Repeated attempts can reject the same list; these are not distinct defect counts.
The final `NO_GAIN` label can follow earlier quality failures. Zero recorded UV
error does not mean the candidate was sound: no compatible normal can prevent
an attribute comparison entirely.

The baseline passes all checks with 15,139 → 14,879 triangles in this fixture.
All final checks include each source material context, protected bone/morph
faces, original storage bounds and an independent host surface comparison.

## Experiments kept out of production

| Isolated candidate | Remaining triangles | Attempts | Decision |
|---|---:|---:|---|
| Existing global refinement | 14,879 | 157 | Baseline |
| Lock vertices of offending faces, then retry | 14,885 | 809 | Reject: worse savings and more conversion work |
| Refine the offending connected section separately | 14,879 | 172 | Reject: no additional savings |
| Reject local collapses rotating a face over 20 degrees | 14,879 | 151 | No additional savings; keep out of production |

Both experiments pass ASan/UBSan, ARM compilation and the independent final
surface checks. Passing correctness does not justify their additional startup
work. Neither implementation is in the application. Reproduction scripts are
`prototype_install_reduce_repair.py` and
`prototype_install_reduce_components.py`; their evidence directories preserve
the exact copied implementations and outputs.

The next reduction design needs to produce candidates that respect local surface
orientation and attribute interpolation during simplification, rather than
spending more retries on candidates the final guard will reject. Do not weaken
the safeguards to improve triangle counts.

`prototype_install_reduce_orientation.py` changes only a private copy of the
pinned simplifier's local face-rotation check, from approximately 75 to 20
degrees. The same 85-list fixture passes ASan/UBSan and independent final surface
checks, with fewer rejected attempts (63 versus 72) but identical final triangle
count. It does not resolve the ineffective fast preset. The modified kernel was
tested on the host; the harness's ARM compilation covers the unchanged reduction
and deformation-guard owners, not this private kernel. It was not adopted or
deployed. Repeated local collapses can accumulate rotation, so this check cannot
replace the final bidirectional comparison.

## Complete geometry caches

`tools/prepare_native_install_caches.py` runs the actual cache owner and
transaction on both complete native geometry outputs under ASan/UBSan. Both
cache files exactly match the independent host encoder; source files remain
unchanged. Each conversion peaks at 262,144 bytes of tracked explicit scratch
and releases it before return. This excludes library internals and stack.

The original cache is 27,560,614 bytes; the performance cache is 27,526,515 bytes.
Both are private integration fixtures, not adopted application-stage journals.
Their research keys must not be relabelled as application recipe keys. This
checks cache production, not complete startup-to-game handoff, live residency,
console conversion time or gameplay performance.

Small-component admission experiment (`prototype_install_reduce_small_components.py`):
lowering the component threshold from 72 to 6 indices, retaining all fidelity
and disappearance guards, again gives 15,139 -> 14,879 triangles on the 85-list
fixture. Attempts rise to 162 and rejections to 77. ASan/UBSan, final independent
checks and ARM owner compile pass, but there is no gain; it is not adopted.

## Changed-region acceptance experiment

The new isolated candidate retains valid changed regions while restoring failed
regions before all unchanged whole-list checks. It removes six extra triangles
on the established fixture and thirty extra on a new 28,474-triangle gameplay-
area fixture. The latter contains one two-triangle regression. It is not promoted;
see `NATIVE_REDUCTION_REGIONS.md` for the architecture, exact scope and evidence.
These results do not support an FPS gain or effective fast-preset claim.
