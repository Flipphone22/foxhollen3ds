# Material opacity correction ? local, not deployed

The original Swizzled/KAlpha families use object/material alpha, independently
of vertex alpha. Their separate opacity plane replaces base-texture alpha.
The native ordinary material path previously multiplied vertex alpha into
these families and also multiplied base alpha before the opacity plane.

The load adapter now selects the original alpha source once, passes the live
object/material alpha in the existing combiner constant, and compiles mask
replacement correctly. The specular compiler uses the same source choice.
The vertex-alpha family retains its existing behavior. There are no new GPU
passes, textures, shader instructions, workers or per-vertex CPU operations.

Actual original Swizzled alpha statements and actual Citro3D descriptors pass
163,840 varying-alpha comparisons, including masks, intensity channels, dual
layers and specular. Three old-behavior mutants fail under ASan/UBSan. Existing
1,671,168 specular RGB comparisons and 1,920 deferred-light comparisons pass.
This is source/math coverage, not an assertion that all eye/texture bugs are fixed.

Serial New/Old cockpit replays both exit cleanly, preserve saves, and pass state
checks. Both 900 captures were inspected. 71 loaded materials use constant
alpha; ZERO loaded separate-opacity planes means mask repair is only host-tested
in this route. No full-game or hardware visual acceptance claimed.

Azahar New rendering CPU: scene 0.773680 + submission 1.943840 ms, versus
0.777170 + 1.942704 before. Old: 0.776623 + 1.942155 versus 0.777587 + 1.943296.
These differences are noise-scale, not a speedup claim; sampled phases differ.
Cadence: New 59.8311, Old 59.8169 Azahar FPS. Not physical-console predictions.

Added peak ordinary heap: 456 bytes in the cockpit. New 47,855,472/71,303,168;
Old 40,515,440/50,331,648. Linear minima unchanged at 18,484,480/25,165,824 and
3,804,416/10,485,760. VRAM free 4,846,592; measured stack 72,844/524,288.
One shared CIA remains Old80/New124. Normal/replay use the same game archive,
67 matching renderer sources and unchanged tested 511-instruction shader.
Normal UI code is unchanged; prior UI evidence reused, no new normal runtime.

Replay: `68e5bb57e1af`; normal: `aafefb6a7b45`. Neither deployed.
Stable sources/archive: `actor-alpha-source-v1-builds`. Builders isolate six
VRAM WIP files and restore them plus the game archive in finally. The alpha
patch is already applied to active and latest stable sources; don't double-apply.

Consulted pinned Melee source/cache visibility and Citro3D texenv buffer/cache
publication. This change specializes native material descriptors at load; it
introduces neither a GX interpreter nor direct register writes.

The six-stage layered-specular draft is shelved under layered-specular-design,
not compiled or executed. Its three independent coordinates conflict with
current procedural fog; no incomplete replacement has been enabled.

Latest user reports: morphs appear fixed in gameplay; lighting/shadows interact
incorrectly with day/night; FS stalls, flickering/disappearing objects, white
Arwing, invisible trees with shadows, and unacceptable fur remain. Current fur
is NOT visually accepted. Physical run collected in console-visual/20261010-153312-223
ends normally at frame24761. Aggregate read bursts accompany 10.48/5.30/5.18-second
presentation gaps; read-burst-analysis.json records exact windows. Slow-call
trace alone misses the thousands of sub-50ms requests. Do not call stalls fixed.

Shadow source investigation: native silhouette uses global gShadowOffset; the
original projected shadow instead selects skyGetObjectLightDirection per object,
smooths against the prior object direction, and clamps near-horizontal angles.
Original projected alpha also uses shadowAlphaStep/shadowTintA; native contact
uses the crash-shadow fixed alpha path. No fix for these differences yet.

Next: investigate synchronous prepared-asset read amplification/visibility,
then original day/night shadow state and faithful fur. Preserve simulation,
collision, saves and measured CPU/GPU/memory budgets. Full goal incomplete.
