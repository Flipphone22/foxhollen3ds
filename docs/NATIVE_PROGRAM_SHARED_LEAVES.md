# Shared native shader command storage

The accepted renderer stored an allocation containing all large upload variants
for each program. Identical variants shared space within that allocation, but
different programs could not share it. Admission was all-or-nothing for the
program, approaching the 64 KiB linear cap in the palace route.

The replacement interns each immutable upload variant by exact word count and
byte comparison at program publication. A variant has one reference per owner;
variants shared within or across programs use the same allocation. Each large
variant is admitted separately. Small configurations and unadmitted variants
retain the existing native SDK command-copy path. No source GX interpretation
or CPU vertex processing is introduced.

No comparison, allocation, reference scan or compilation occurs at draw time.
Submission dereferences the selected leaf and uses the existing channel-0 call
and channel-1 return owner. Shader instructions, command order, upload content,
draw order, geometry, simulation and audio are unchanged. This reduces storage
duplication; it does not remove the GPU's shader upload work.

The 64 KiB cap still includes unreferenced in-flight allocations and admission
retains the 15% linear reserve. Last-reference release can free only unused or
retired leaves. Pending GPU readers retain their allocation until the existing
successful frame retirement. Reacquiring an in-flight leaf preserves its use
epoch. All ownership remains on the existing graphics owner. Pinned Melee
`command_cache.c` publication discipline was consulted; the established flush,
queue split and retirement boundaries are retained.

`test_program_shared_leaves.py` executes the actual linked ARM libctru command
writer and the real native packet/segment owner with mocked allocator, cache
flush and completed-frame epochs. The retained 18-DVLB corpus supplies 1,613
exact SDK comparisons, including 852 leaf expansions. Sharing, divergent bytes
of equal length, separate owner releases, in-flight reacquisition, partial
admission, short destinations and memory reserve are exercised. Six incorrect
implementations are rejected. 18,413 checks pass. These tests do not establish
physical GPU coherence or timing.

The synthetic maximum-upload comparison is 6,577 SDK instruction encounters
versus 223 for the candidate. The previous resident publisher was already 213
in its fixture: this change is not a claimed faster resident-call kernel.
Application memory/copy counts and correctness must justify it before promotion.

Build from the accepted `unified-aux-chunks-v1-console-setup-builds` snapshot with
`tools/build_program_shared_leaves.py console-setup --revision v1`. Do not build
the unrelated root WIP. Only the program-plan owner changes; shader and game
archive identity, shared Old80/New124 profiles and restoration are checked.
No deployment or physical performance claim is made by the source tests.

## Application result: not promoted

Candidate `041ed5fa844f` completes the Old80 palace replay with native DSP LLE,
state/save/settings checks, drained logs and owned exit (97.81 host seconds).
Capture1601 was inspected: world geometry/textures, Krystal, purple creatures,
particles and HUD remain. No physical or complete visual parity claim follows.

The decisive result is unchanged: resident storage remains 64,624 bytes and
two admissions are still rejected. There are 30 shared acquisitions, but this
does not improve on the parent's within-program deduplication on this route.
In both eight-frame profiles there are 184 program configurations, 136 shader
leaf calls, 280 total chain calls and 48 segment publications. Configuration
work increases slightly from 10,720 to 10,808 ARM instruction encounters.

Whole-window work falls 21,912,036 to 21,130,837, but audio curve calls fall
1,711 to 1,375, audio jobs28 to25 and particle draw calls312 to293. Do not claim
that 3.565% change as an implementation speedup. Whole-run copy-saved counters
also span differing counts of calls and cannot establish a saving per frame.

Observed heap increases408 bytes to41,859,512/50,331,648; linear minimum free
decreases128 bytes to1,622,656/10,485,760 (allocator accounting), while the
shader-leaf byte total stays unchanged. VRAM free4,723,712 and sampled main
stack72,984/524,288 are unchanged. Process commitment69,595,136/83,886,080 is
not a direct measure of the metadata change; executable layout also changes.

Keep the accepted `92fd26aa4763` shared build/package. Do not package, deploy or
request hardware testing for this candidate. Sharing complete packets is not
the useful granularity for the remaining shader storage duplication, and
shader-leaf submission itself is a small fraction of the current CPU profile.
Do not repeat this experiment or pursue its isolated instruction count as the
large full-frame improvement. Evidence and the reproducible implementation
are retained under `program-shared-leaves-test/`.
