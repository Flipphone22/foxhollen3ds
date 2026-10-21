11 October: user confirms the repaired C-stick background works in the deployed normal build. This accepts that reported hardware defect; other rendering issues and performance targets remain open.

# Native UI model shader

11 October: hardware probe `1e6dff12b342` completed all eight cases. In
`artifacts/console-cmenu-probe/20261011-091330-results`, the full shared and
standalone scene shaders each retain only 256 panel pixels. Removing the
projective-Q output also retains only 256. Replacing the two ordinary UV
calculations with direct vertex UV/constant inputs restores 2,800 pixels,
byte-for-byte identical to the minimal GPU skinning control. The oracle has
2,801 pixels; its mask overlap with the repaired draw is 99.964%.

This isolates a hardware-visible problem in the shared temporary UV path.
It does not establish the precise register/control-flow failure, or justify
removing the different coordinate calculations used by fur and specular maps.
The probe is preserved; Azahar alone does not reproduce this fault.

The game candidate adds `pica_ui_model.v.pica`: a 43-instruction native shader
for source-authored, unlit UI materials. Selection uses the prepared material's
UI classification, not model IDs. Existing resident vertices, indices, bone
palettes, poses, source colours, two texture planes, raster state and texture
combiners remain in use. Both bone blends stay on PICA. There are no CPU vertex
calculations, extra passes, asset copies or workers.

Its float ABI remains c0–95; every component of all seven output registers is
initialized. Scene-owned shader switching invalidates existing material, HUD,
glow, fog and colour-fade caches and republishes the required attributes and
palette. World, fur and shadow paths clear the UI selection. The existing
scene/height/layered/fur programs remain intact. The new program is allocated at
initialization and freed under the same parent GPU-retirement contract as the
other scene programs. Pinned Melee complete-output and state-ownership patterns
were consulted; no raw GPU register writes or retired rendering path is added.

Validation:

- `tools/test_pica_ui_shader.py`: 32,768 unlit poses across 22 bones, blended
  weights, root offsets, colours and two UV planes; all 917,504 output lanes
  match the original float24 interpreter output exactly.
- `cmenu-ui-v3-builds/verification.json`: the actual linked UI shader matches
  that test; all three existing scene/fur/material shader binaries and the
  game archive are unchanged. Only the three owner/selector source files and
  the new shader are renderer changes.
- `artifacts/native/cmenu-ui-new`: ordinary shared `.3dsx`, 2,160 scripted
  retraces, 95.12 host seconds, successful Select exit, drained logs and owned
  process exit. Each of the three panel models records 510 draws through the
  new pipeline; 3,060 UI draws total. Images 2010 and 2130 were inspected.
  World and UI interleave each frame. Releasing the C-stick did **not** close
  this latched menu; no separate close-menu acceptance is claimed.
- Shared CIA keeps Old 80 MiB / New 124 MiB grants. Physical game acceptance
  remains pending even though the corrective arithmetic passed the probe.

The comparable 300-frame emulator menu windows report submission time
0.165580 → 0.165653 ms and GPU time 0.677980 → 0.677780 ms. These counters are
not total rendering CPU cost or physical GPU timings. Cadence remains about
16.714 ms. Pose/workload counts differ slightly; this is a short regression
check, not a speedup claim or console forecast.

Observed final normal heap use is 53,887,088 / 71,303,168 bytes, 288 bytes more
than the prior run. Linear free space stays 18,656,128 / 25,165,824 bytes and
VRAM free space stays 4,822,016 bytes. Process commitment remains 102,694,912 /
130,023,424 bytes; measured main stack is 73,008 / 524,288 bytes. The small
shader resides in the executable; it adds no texture or geometry storage.

Build with `artifacts/research/pica-scene-rewrite/build-cmenu-ui-v3.py`. It
restores the exact deployed source/archive, applies `pica_ui_shader_patch.py`,
builds, archives the result and restores all WIP in `finally`. The patch is
already applied to current working sources; do not double-apply. A failed
earlier build caught CRLF-sensitive patch matching, and another caught an ARM
`u32` printf type mismatch; both checks stayed enabled.

Candidate normal: `59d8e6f354e8`; CIA: `ea7b4911b58a`. No replay/Old installation
or game assets are changed by the New-only deployment. Full renderer parity,
console performance and the underlying shared-UV issue outside these unlit UI
materials remain open.
