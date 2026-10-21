# Deferred world streams

18 October: local load-time compiler, not integrated or deployed.

The main-only native plan excludes the two secondary source streams. These
streams must **not** be restored as ordinary main-material draws. In original
`tex_dolphin.c`, `renderMapBlock` sets `doSetup=0` for both secondary streams;
`mapBlockRender_callList(passSelect=0)` tests visibility and queues effects.
Hidden flag 2 rejects a list. Flag 0x80000000 selects queue family 5 (water),
taking precedence over 0x40000000 or 0x2000 selecting family 4 (transparent
surface). Other flags alone enqueue nothing. `lightmap_draw.c` later sorts and
dispatches those families to distinct effect consumers.

`pica_world_deferred.c` compiles immutable records with group, section, authored
list, material, exact first index/count and source queue family. Geometry is
borrowed, never copied. It deliberately retains per-list identity instead of
merging lists that may have different bounds, selectors or sorting keys.
The preceding stream compiler validates stream boundaries at load time.
Neither compiler adds frame-time work or changes the currently built apps.

Validation: `tools/test_pica_world_deferred.py` extracts the original branch
conditions into a compiled oracle and checks 256 flag combinations. Across
1,267 prepared world packets it compares every emitted record with the actual
section metadata: 380 family-4 records, 240 family-5 records, and 57 deliberately
unqueued lists. Short output capacities reject. Strict ARM11 hard-float object
compilation also passes. These are compiler checks, not rendered-image evidence.

Remaining integration: resolve live authored bounds/selectors at load, preserve
visibility and queue ordering, implement each native effect's source equations,
and share the existing fenced resource lifetime. Do not invoke the retired GX
interpreter, draw both families with ordinary materials, or merge transparent
lists across depth keys. Consulted pinned Melee `raster_state_invalidate`:
native effect transitions must invalidate displaced shared state explicitly;
the new compiler itself owns no GPU state and needs no invalidation.

Storage is 28 bytes per admitted record before any future owner metadata;
620 records across this private inventory would total 17,360 bytes if all were
resident. This is not a live allocation or a peak-memory measurement. No FPS,
visual parity, or dinosaur-rescue crash fix is established by this work.

The load-time source-list decoder now retains the original signed 16-bit bounds
and selector in a 16-byte record. It rejects truncated tables, invalid list
indices and inverted bounds without publishing partial output. Its input is
the big-endian asset list table before game relocation, not a live pointer table.

The live sort-key helper consumes that record, block translation, coordinate
scale and the camera depth row. It preserves the source's translate-then-average
order and integer depth quantization. Clamping precedes integer conversion to
define overflow safely; within the original defined range, keys agree. Only
the depth row is evaluated, since the original transformed X/Y are discarded.
This CPU work remains necessary for source transparency ordering; it neither
transforms rendered vertices nor interprets source render instructions.

`test_pica_world_sort.py` executes the extracted original queue-key function
against 65,536 generated native cases with ASan/UBSan. The original matrix helper
is represented by scalar float multiply/add order, so this is not a claim about
hardware floating-point precision. Malformed-input checks retain output state.
Strict ARM11 compilation passes: decoder stack 32 bytes, live key helper 40,
record compiler 144. No heap allocation or GPU submission is added. Runtime
owner integration, complete queue ordering and timing remain unverified.

The isolated `world-deferred-owner-v2` normal build now attaches those records
at actual scene registration, before the source block's endian/pointer
relocation. Admission retains the existing normal-memory reserve and a separate
64 KiB aggregate plan cap. Publication occurs only after every source list and
material reference passes validation. Release follows the existing scene
retirement path; pinned Melee `early_queue.c` confirms that resource reuse must
wait for queued consumers. No new fence or GPU-visible allocation is introduced.

Each plan uses 12 bytes plus 44 bytes per entry; the scene directory adds one
pointer per slot (1,024 static bytes on ARM). Load compilation uses a bounded
256-descriptor scratch array (7,168 bytes before other stack locals). It is not
part of frame submission. Live draw dispatch still does not consume the plans.
Host ownership tests pass 1,000 lifetimes and seven rejection/cleanup paths with
ASan/UBSan; their allocation and admission helpers are controlled mocks.

The first isolated build failed after importing a stale root header. It was
retained as `world-deferred-owner-v1`; the corrected v2 builder patches only the
required declaration into the accepted archived header. V2 changes six source
files, retains the game archive and all linked shader programs, and verifies
Old 80/New 124 MiB CIA profiles and workspace restoration. It is not deployed.

## Ordinary alpha surface integration

The next local candidate routes admitted family-4 surfaces into the existing
game transparency queue. It retains the original queue sorter and relative
ordering with actors/particles. Queue entries reference the resident native
record; the family-4 consumer invokes a native slice publisher instead of the
original GX setup or render-instruction interpreter. Visibility still uses the
source bounds test. Disabled world rendering returns before asset lookup.

The initial admitted material flags are `0x40000000` plus ordinary flags in
`0x000c001d`; the alpha-surface bit must remain present in live state. Other
families are counted as unsupported, not silently drawn with ordinary materials.
Projected falloff, water caustics and more complex overlays remain missing.
This is a restoration stage, not full deferred-rendering parity.

Each resident entry now also stores its prepared GPU vertex/index addresses,
index count, format, culling and material identity. ARM entry size rises from
44 to 76 bytes; the existing 64 KiB cap remains. The GPU submission does not
reconstruct geometry or decode source metadata per frame. It uses the source
transparent viewport depth range (-0.01 to 1), the existing native material
compiler and explicit shader-state invalidation on departure.

The material loader now includes admitted deferred readers, including blocks
with an empty main stream. V3 reached the actual draw path but failed on an
uninitialized material, proving the old main-only admission was insufficient.
V4 fixed admission, then failed on an unconditional inactive height-fog bind.
The next revision follows the ordinary world height-fog prepare/bind conditions.
Neither failed runtime is accepted or deployed. V1 was an early CRLF patch-anchor
failure; V2 failed a missing queue-flush declaration before application build.

Focused checks execute the actual adapter across 2,501 queue publications,
capacity flushes, live flag changes, visibility rejection and disabled work.
The actual slice publisher passes 3,000 fog/no-fog transitions, exact index-range
checks, hidden-material and rendering-off cases under ASan/UBSan with mocked SDK
and engine services. These tests do not substitute for GPU pixels or real game
runtime validation. Game archive comparison for V4 shows exactly two changed
objects (`tex_dolphin.o`, `lightmap_draw.o`) among 794, with member order and all
other payloads unchanged; linked shader programs are unchanged.

V5 normal build `e03c057637ac` / 3dsx `7bb3631d50cc` passes the New-model HLE
save-load route: 748 queued/dispatched/emitted GPU surface draws, 960 unsupported
family encounters, zero material errors, ordinary exit and completed GPU drain.
Both cockpit and loaded-cave captures were inspected. Saves remained unchanged.
This establishes actual dispatch, not source pixel parity. No console deployment.

Observed memory: heap peak 54,180,888/71,303,168 bytes; minimum free linear
17,125,120/25,165,824; free VRAM 4,690,944; sampled main stack 72,972/524,288;
process 104,792,064/130,023,424. Three plans built, none rejected; live/peak plan
allocation at the pre-release log was 328 bytes. Material loads increased from
468 to 470 on this route. Other effects remain incomplete.

Late-cave emulated presentation intervals were 16.6973 ms for the owner-only
baseline and 16.7047 ms for V5. These are capped, phase-varying observations and
do not isolate restored cockpit surfaces; **performance acceptance is pending**.
Before adding another family, instrument the slice's submission cost (currently
included in full frame work but absent from the existing native-submit bucket),
compare a matching surface-heavy window, and obtain stronger image evidence.
Old-model runtime and the matching replay build remain to be validated. The
dinosaur-rescue crash remains a separate unresolved issue.

## Submission and full-frame cost gate

`world-deferred-timing-v2` (normal `3b777c0adf19`, 3dsx `133993d2c5de`)
adds native-submit phase accounting and cumulative/per-frame slice timing.
It retains the V5 game archive, shaders and render behavior. No deployment.
Counters use system ticks with an explicit frequency and write only at shutdown.
Recorded cost includes reserve waits and publication; it is not pure CPU cycles.

Sequential baseline/candidate New-model HLE runs held the Game Select panel in
Fox's cockpit with identical input and save fixtures. Both exited normally;
captures retain Fox, fur, the panel and room. The restored run emitted 1,131
deferred draws across 1,131 active frames. Submission averaged 0.100026 ms per
active frame and peaked at 0.565093 ms on frame 1171. This **misses** the
provisional 0.5 ms New-model submission budget. Queue preparation is outside
that subcomponent; do not label 0.100026 ms as the complete feature cost.

Full presentation intervals in sampled windows 1000–1600 averaged 22.0786 ms
baseline versus 22.4924 ms candidate. The windows contain 641 versus 629 frames,
and animation phases differ; this is a raw comparison, not a causal slowdown
estimate or physical-console prediction. Both remain above the 16.67 ms target.
Reported emulator GPU times were 0.72791 versus 0.72314 ms, not console GPU
measurements. Performance acceptance remains open; investigate submission spikes
and the larger full-frame CPU cost before adding another visual family.
