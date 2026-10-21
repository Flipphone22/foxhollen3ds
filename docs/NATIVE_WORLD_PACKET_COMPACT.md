# Count-sized persistent world packets

This change builds on the direct-particle-stream application. It retains the
same native shader programs, game archive, geometry, material selection, draw
order, whole-group visibility checks, epoch ownership and cache publication.
It does not skip through a partially visible command list: attribute/layout
state inside a packet may depend on preceding draws, so that shortcut is unsafe.

Previously every group embedded metadata for 16 draws and reserved command
space for the worst-case command count. Groups of two or three were excluded.
The new load compiler writes commands and metadata into bounded temporary
storage, then allocates exactly the compiled command extent and the group's
actual number of metadata entries. The packet stores a pointer/capacity for
owner-provided metadata; both preparation and update validate that capacity.
All persistent pointers are relocated out of load-time scratch before the group
is published. The same release fence retires commands and metadata together.

The minimum group size is now two. The global caps remain 128 KiB linear and
24 KiB CPU metadata, with the existing 15% free-linear-memory admission guard.
No allocations or compilation move into the frame loop. Load-time scratch adds
roughly 14 KiB (commands plus metadata); its actual stack high-water must be
checked in gameplay. No geometry, textures, extra shaders or GPU passes are
added. Pinned Melee's explicit streaming/command cache-publication discipline
was consulted; the port's verified submission owners are retained.

New load-time counters distinguish candidate groups, singleton runs, unready
draws, selector rejection and budget rejection. `linear_saved` compares actual
command bytes with the old worst-case reservation. `cpu_saved` measures removed
unused entry slots, not net metadata savings: pointer/capacity and allocation
bookkeeping add a small header cost. Observed allocation peaks determine the
actual memory result.

The ARM11 core test preserves every effective register and material uniform at
1,024 draws against the independent native draw reference (247,499 checks),
including state that belongs to other rendering owners. Five incorrect-state
mutants fail. New null/undersized metadata cases reject preparation; failed
updates preserve both command bytes and the separately allocated metadata.
The actual allocation/update/release owner passes 211 checks, including pair
admission, singleton retention, exact storage sizes, immutable queued epochs,
hidden/fog-incompatible groups, publication retry and global memory ceilings.
Those tests stub GPU execution/fences and do not establish console correctness.

`tools/world_packet_compact_patch.py` applies only to archived sources.
`tools/build_world_packet_compact.py` builds the shared application and restores
unrelated root work. `tools/test_world_packet_compact.py` reproduces the core and
owner checks. The new packet ABI is private to the native renderer; cached
converted assets, save formats and setup recipes do not change.

Gameplay evidence belongs under `world-packet-compact-test/` and the
`unified-world-compact-v1-*` runtime directories. Full-frame performance,
hardware cache correctness and the complete port goal remain unproven.

## Gameplay results and four-draw groups

Compact metadata alone (`6eab5305868c`) increases the matched palace packet
coverage from 216/768 to 232/768 draws, while reducing peak packet allocations
from 81,840 to 55,040 linear bytes and 16,992 to 14,284 CPU bytes. No group hits
the memory cap in this run. This is useful storage reduction, not a large
submission improvement.

The next candidate (`a14385518a6f`) limits each contiguous prepared group to four
draws, retaining the same all-visible/all-compatible requirement. It does not
claim spatial reordering or BSP grouping. The owner passes 169 ARM checks;
unchanged packet compiler tests are reused by source hash. This increases
coverage to **528/768 draws (68.75%)**, with individual submissions reduced to
240 and 144 packet calls in the eight-frame window. All three candidates submit
the same 768 native world draws in that window. Coverage derives from verified
selector/append entry counts with no load-time or failed-group calls.

The selected world-command functions, including preparation of individual
commands, packet updates, selection, calls and submission wrappers, total
565,288 -> 565,088 -> 516,089 ARM encounters across the original, compact-only
and four-draw candidates. The last is 8.70% below the original; it is not the
complete renderer, since shared helpers, scene traversal and GPU costs remain.
The complete windows total 24,018,297 -> 24,189,360 -> 24,282,087. Audio job,
particle and periodic diagnostic phases differ, including a large unchanged
worker stack scan. No complete-frame time or physical FPS gain is established.

Both Old profiling runs pass state/save/drain/owned-exit checks, and their
frame-1600 captures retain world geometry, textures, Krystal, creatures,
particles and HUD. The four-draw New run also passes, with its frame-1601 image
inspected and 113,592 GPU draws audited without a layout fault. These bounded
checks do not establish physical packet-cache correctness or endurance.

Four-draw peak packet storage is 64,688 linear bytes and 16,480 CPU bytes,
below the original fixed-size representation despite higher coverage. Actual
Old heap peak is 41,734,800/50,331,648 (+320 bytes versus original); minimum
linear free is 1,688,704/10,485,760 (+15,488 bytes). New heap peak is
56,612,024/71,303,168, minimum linear free 16,368,768/25,165,824. Both retain
4,723,712 free VRAM and observed 72,984/524,288 stack high-water. No memory cap
was raised; source/asset formats and setup recipe remain unchanged.

The shared 3dsx/CIA package is
`build/console-validation/foxhollow-iso-setup-c80a57c16eae06ed.zip`. It contains
no private assets or settings and has not been deployed. Old/New payload
identity and rejection of mismatched CIA content pass the package verifier.
Fresh conversion was not repeated for this binary; previous unchanged converter
and recipe evidence is reused. The package does not claim fresh conversion or
physical validation. `world-packet-local-test/integration.json` binds both build
archives, gameplay captures, memory, comparisons and package hashes.
