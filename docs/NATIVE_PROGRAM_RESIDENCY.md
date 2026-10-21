# Resident PICA shader commands

The complete CPU upload cache still copied every shader upload into the SDK
arena on each program change. This candidate places the larger immutable SDK
upload sequences in linear memory once and submits a channel-0 call to them.
The leaf returns through channel 1. The existing world-packet segment owner
seals return addresses before cache publication. Calls are not nested.

Small configuration packets remain direct copies. Identical upload-flag
variants share one leaf. Total leaf storage, including deferred frees, is
capped at 64 KiB; admission retains the existing 15% linear-memory reserve.
The ordinary CPU cache remains available when admission or call capacity fails.
This is the same native SDK command sequence, not the retired GX renderer.

Program setters invalidate the CPU plan and detach the GPU allocation. A leaf
that has been used in the current frame is retained on a deferred list; only
the existing successful FrameBegin retirement can release it. A never-used or
previously retired leaf can be freed immediately. A pending, unsubmitted call
prevents epoch advancement. There are no new GPU waits, workers or per-draw
allocations. The same owner handles SDK command-list splits and world calls.
The existing GS-to-VSH split remains unchanged.

Pinned Melee command_cache.c was consulted for explicit dirty-range publication
and the kernel-cache-flush/service fallback. Existing Foxhollow flush/fence
owners are reused. Shader instructions, upload contents, simulation, audio and
assets are unchanged. This reduces CPU copy and command-arena writes; the GPU
still fetches and executes the shader upload commands on each required change.
It does not eliminate shader upload hardware work.

`tools/program_resident_patch.py` applies only to the archived ee121967dec3
parent, through the isolated builder. Root renderer WIP is not a build input.

## Initial verification

`program-resident-test/result.json`: 16,253 actual ARM checks, 1,613 SDK command
comparisons, including the 18 embedded DVLBs, and 852 executed leaf comparisons.
The real leaf/segment relocation functions run with mocked allocator, physical
addresses, cache publication and retirement completion. Tests cover capacity,
deduplication through command equivalence, deferred lifetime, table/cache
pressure and the linear reserve. Premature free, wrong return channel, omitted
flush, ignored reserve and wrong upload selection mutants are rejected.

Synthetic maximum-upload instruction encounters are 6,577 SDK versus 213 for
the resident call; this is not a captured-workload cycle or frame-time estimate.
Hardware cache coherence, GPU behaviour and console performance are not proven
by these CPU tests.

## Exact application checks

Shared candidate source `610d4b418e6d` is archived in
`unified-program-resident-v1-console-setup-builds`. The only changed sources are
the program-plan owner and command-packet owner. All 18 GPU programs, the game
archive, Old80/New124 profiles and conversion recipe are unchanged. Isolated
build restoration passes. No fresh conversion claim or redundant conversion
boot is made.

Old palace profile completed in 115.91 host seconds; New layout-audited palace
completed in 93.07. Both preserve saves/settings/fixture, drain logs and exit.
Captures 1601/1600 were inspected: palace textures/geometry, Krystal, purple
creatures, particles and HUD are retained. The final New GPU report covers
112,474 draws with zero layout faults. This is limited emulator evidence, not
physical command-chain acceptance or full visual parity.

Old/New resident calls: 11,719/11,683; avoided command-copy bytes:
28,482,784/28,328,568 over the complete respective runs. Both allocate 64,624
linear bytes; two load admissions exceed the cap and retain CPU command copies.
No direct SDK generation or CPU-plan rejection occurs. No deferred frees remain
at the pre-shutdown report. These counters include startup and are not just the
eight-frame profile.

Old sampled heap is 41,859,104/50,331,648; minimum linear free
1,622,784/10,485,760 (15.48%, above the unchanged 15% reserve); process commitment
69,595,136/83,886,080. New heap is 56,736,328/71,303,168; linear free
16,302,848/25,165,824; process 104,738,816/130,023,424. Both retain 4,723,712
VRAM bytes and measured main-stack usage 72,984/524,288. These are palace
samples, not worst-case full-game headroom proof.

Compared with the preceding CPU-cache build, the eight-frame profile has
1,196,745 -> 884,815 memcpy instruction encounters. Additional chain/seal work
is explicitly included. World coverage remains 768 draws: 528 grouped,
240 individual, 144 calls. Raw whole-window encounters **increase** from
22,601,366 to 23,224,461 (+2.76%): the candidate window includes 120 lightning
calls absent from the parent, different particles, 25 vs 27 audio jobs, and a
periodic stack scan. No attributable whole-frame FPS gain or regression is
established; do not subtract those differences to manufacture an estimate.
Timing remains excluded for profiling/DSP LLE/captures.

Package `foxhollow-iso-setup-ae34179b013ac082.zip` is READY, NOT DEPLOYED.
Old/New identical payload and stale-CIA rejection checks pass. It is the single
normal app with its existing startup benchmark menu, not a new probe app.
Physical baseline remains 13.5157 FPS / 73.988 ms; 61.19 ms is still a hypothesis.
The full performance, fidelity, progression and endurance goals remain open.
