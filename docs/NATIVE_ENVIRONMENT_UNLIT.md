# No-light environment material correction

11 October 2026. Local normal `f75b075a8585` and replay `7295f9362325`;
not deployed. The New 3DS still has normal `d6068a861f8f` and replay
`e6178b4e5cd0`, including the previously verified fur projection repair.

The original `rcp_dolphin.c` sets a white register material for its mode-0
environment maps. `modelLightChannel_configure(0, 0, 0)` and
`modelLightChannels_applyGXControls()` disable lighting when no lights were
selected. GX then passes that material through; it does not apply ambient.
The native map generator already handled this case, but the default analytic
environment path incorrectly returned ambient colour. This could darken or
tint this particular material contribution when the selected-light count
became zero. It is not evidence that every reported day/night, eye, Arwing or
visibility defect has the same cause.

The native environment owner now publishes white ambient plus its existing
dark dummy light for a zero-light source slot. This supplies the required
constant result through the existing PICA fragment-lighting path. The separate
fog-only owner remains black; ordinary nonzero-light snapshots are unchanged.
Live source snapshots, light selection, simulation and diffuse-map generation
are unchanged. The extra condition runs only when the cached environment state
is published. No extra GPU calls, geometry, render passes, textures, worker,
resident allocation or shader instructions were introduced.

The pinned Melee persistent-state invalidation in
`native-3ds/port/3ds/{renderer,citro3d_fix}.c` and pinned Citro3D
`source/{lightenv,light}.c` were consulted. The existing SDK owner, dirty-state
publication, ambient packing, dark dummy-light handling and resource lifetime
remain in use. There are no raw register writes or per-draw texture flushes
added by this correction.

`tools/test_pica_environment_unlit.py` compiles the actual original channel
configuration/control functions and the actual native environment owner under
ASan/UBSan. It checks 300 ordinary slot changes and 300 layered/fog sequences,
including repeated cached binds, live updates, disable/re-enable, eight-light
admission, the fog-only owner and ordinary restoration. It rejects both the
old ambient behaviour and a mutation that also brightens the fog-only owner.
This is source/SDK-boundary evidence, not a physical GPU image comparison.

Build verification confirms 334 matching native sources between normal/replay,
the unchanged game archive, unchanged exact 511/214/270-instruction programs,
and shared Old80/New124 CIA profiles. The environment owner is the only native
source changed from the deployed fur-projection build. Six unrelated VRAM WIP
files were isolated and restored by both builders. The matching normal UI has
not been booted again: its unchanged startup/menu evidence is reused, not
represented as a new runtime check.

Runtime measurements and completion assertions are recorded in
`artifacts/research/pica-scene-rewrite/environment-unlit-runtime-test/`.
The ordinary-input route approaches the Arwing from the ThornTail fixture,
using original geometry/textures and graphics mask3866625. The New run uses
the checked original-geometry cache. These runs do not prove that the
zero-light condition occurred in gameplay, nor reproduce the physical white
Arwing/invisible-tree fault. Source tests exercise the affected condition.

Serial New/Old `environment-unlit-v2-*` checks pass replay state assertions,
save preservation, log draining and owned-process exit. Both1650 captures were
inspected: blue/panelled Arwing, tree, Fox, world and HUD remain visible.
Measured Azahar cadence is54.535927/47.335735FPS, scene preparation
2.058731/2.036988ms and native submission3.958374/3.957117ms. These are
unpaired snapshots with differing animation/workloads, not console forecasts
or evidence of a speedup. No unchanged extra boot or FPS multiplier is used.

New/Old normal heap peaks56,425,944/41,544,512B against71,303,168/50,331,648B;
minimum linear free16,469,120/1,815,680B against25,165,824/10,485,760B.
Old linear headroom is17.32%, above the15% reserve but narrow. Both retain
4,822,016B free VRAM and measured main stack72,868/reserved524,288B.
Process commitments103,206,912/67,555,328B remain below124/80MiB grants.
These measurements describe this route, not worst-case whole-game usage.

Two host recipe mistakes were caught before gameplay: the first New attempt
specified a nonexistent cache directory; the first Old command requested the
unsupported Old RAM cache. The corrected commands preserve the failed New
summary and use the existing New cache, with no Old RAM cache. No console,
save or asset changes occurred.

Filesystem stalls, intermittent visibility, full original-image lighting and
shadows, missing projected effects and the remaining fur wind/detail gaps
remain open. No physical performance improvement or full parity is claimed.
