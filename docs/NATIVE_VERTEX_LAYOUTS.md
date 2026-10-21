# Prepared native vertex layouts

World and actor draw submission previously reconstructed `C3D_AttrInfo` whenever
their input layout was published. This repeated `AttrInfo_Init`, several
`AttrInfo_AddLoader` calls and, for some models, `AttrInfo_AddFixed`. The formats
are immutable: only compact/expanded storage, world texturing and fur direction
streams select among them.

Ten layouts are now compiled at renderer initialization using the pinned SDK's
constructors. World/actor submission selects and publishes the prepared record
through `C3D_SetAttrInfo`. Fixed attribute values 5 and 6 are still restored at
the same binding boundary; preparing their descriptors does not preserve their
live register values across another pipeline.

This removes repeated layout construction, not geometry or simulation work.
It adds 240 bytes of fixed descriptor storage and eight bytes of counters, with
no heap/linear/VRAM allocation, worker, shader changes, resource copy or extra
draw. It does not yet replace per-draw vertex-buffer descriptor construction.

The pinned Melee Citro3D fix and the SDK's attribute publication were consulted.
Publication stays inside the SDK dirty-state path. Existing program, framebuffer
and APT invalidation still controls GPU state reuse; the application does not
keep a second cache of the current GPU layout or write registers directly.

`test_pica_vertex_layouts.py` extracts the previous world/actor setup statements
from the tested particle-fog snapshot and uses the actual pinned SDK constructor
and publication bodies. It compares all ten layouts over 100,000 bindings with
poisoned intervening descriptor/fixed-attribute state under ASan/UBSan. Struct
bytes, dirty flags and every fixed value match. The test performs 62 constructor
calls at initialization instead of 620,000 across those synthetic bindings;
that is not a gameplay speedup percentage. A mutant that omits restoration of
fixed attribute 6 fails.

The isolated builder uses `glow-fog-normal-v2-builds` and restores all worktree
files/game archive afterward. Only `platform_pica_scene.c` and the new
`platform_pica_vertex_layouts.h` differ. The game archive and all linked shader
bytes must remain identical. Runtime measurements and build identities are
recorded separately. No physical performance or skybox-fix claim is made.

Serial Old/New cave replays of `c47c685d5c2e` completed with status 1, unchanged
saves, drained logs and owned-process exit. Both 1410 captures were inspected:
textured cave, Fox, ladder, mushrooms, torch, HUD and Old touch stick remain.
Particle haze still differs with phase, as established separately.

| Azahar guest measurement | New | Old |
| --- | ---: | ---: |
| Native submission before / after, ms | 5.560080 / 5.499612 | 5.578334 / 5.513532 |
| Scene preparation after, ms | 1.865639 | 1.852610 |
| Change in scene + submission, ms | -0.070562 | -0.063100 |
| Guest measured FPS | 59.831224 | 57.140176 |
| Prepared world / actor layout bindings | 7,405 / 39,366 | 7,073 / 37,630 |

These are short comparisons with matching recipe/assets/options, not exact
animation-state pairs. New world and actor geometry totals match the preceding
particle-fog run; particle counts differ. Old geometry counts also differ slightly.
The result supports retaining the prepared layouts, not a large FPS improvement
or a prediction of console speed. Host playback FPS is distinct from these guest
measurements.

Measured New/Old heap peaks are 48,404,624/40,867,984 bytes against reservations
71,303,168/50,331,648. Minimum linear free is 18,441,856/3,761,792 bytes against
25,165,824/10,485,760 reservations. Both retain 4,723,712 bytes free VRAM and a
73,008-byte measured main stack inside 524,288 bytes. Process commitment remains
103,301,120/67,649,536 bytes within 130,023,424/83,886,080. New's observed heap
peak returns to the earlier sampler-run value; this change introduces no heap
allocation and that variation is not claimed as memory saved by this change.

Reproduction: `build_vertex_layouts_v1.py`, `check_vertex_layouts_v1.py`,
`summarize_vertex_layouts_v1.py`; results in `vertex-layout-runtime-test/`.
Completed ELF/map/game-archive evidence was NTFS-compressed with before/after
hash verification to retain testing space. Paths and bytes are preserved.
