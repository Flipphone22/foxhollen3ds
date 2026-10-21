# Reduced native geometry candidate

The 7 October boundary-preserving candidate is private asset data, not a new
game-code build. It is not yet deployed or physically performance-validated.
It reduces native render triangles while preserving original game assets,
collision, callbacks and simulation. It requires broader visual validation.

Prepare it under the locked Linux environment:

```sh
python3 /work/tools/prepare_native_proxies.py \
  --source /work/artifacts/research/minimal-normal-palette-v11/native-scene.bin \
  --output /work/artifacts/research/native-proxy-visual/reproduced \
  --ratio .25 --error .1 --compact-vertices --topology
```

The output directory must not already exist. The verified output is
11,052,204 bytes, SHA256
`2225e2349f7a9e5a629445ce0fe2cf14562e4d782711682f4edc7171b9d244ea`.
The original v11 pack remains intact. Two preparations produced identical
pack bytes; manifests record the preparer source revisions separately.

The host-only meshoptimizer dependency is pinned to
`73583c335e541c139821d0de2bf5f12960a04941`. Edge collapses operate within
ordered material sections. Independent checks retain component coverage and
exact open/nonmanifold boundaries. Surviving vertex bytes and animation
references are copied exactly; only triangle topology and unused storage change.
This can still change interpolated UVs, lighting and deformation, so structural
checks alone do not establish visual acceptability.

`tools/audit_native_proxy.py` verifies all 558 packets, 3,202 groups,
272,645 surviving vertices, 65,231 model references and 3,289 sections.
`tools/test_native_proxy_audit.py` rejects corrupt vertex attributes, animation
references, callback maps and indices even with repaired CRCs and manifests.

The stationary Palace comparison uses the same archived executable
`6e9dcbd2a38d`, input script and starting save. Rendered frames 1000–1189:

| Geometry | Vertex references | Draw calls |
| --- | ---: | ---: |
| Original v11 | 3,181,740 | 8,929 |
| Boundary-preserving candidate | 2,543,340 | 8,929 |

Across that run, lit model references fell from 11,683,659 to 7,959,741.
These are workload counts, not physical GPU timing or FPS forecasts. Camera
and animation timing can differ slightly. Inspected roof and intro images
retain the floor and character. A separate moving-camera run uses the current
`4d04d721ede6` console executable; its inspected wall, floor and pillar views
remain present. This route does not cover the central-room freeze or Shrine.

The per-component sloppy candidate is rejected for visible floor/UV distortion,
despite passing coarse extent/area guards. Keep its evidence; do not deploy it.

Evidence and exact commands are under
`artifacts/research/native-proxy-visual/`. Do not replace the pending hardware
freeze-check assets mid-test. The next asset trial should use this candidate
only after that result is collected, with an explicit backup and readback.


Exact Old3DS cdf35e4f5c17 also completed the stationary Palace run and Select
shutdown. Sampled normal+reserved main stack:40,374,664/48,234,496 bytes;
linear5,225,088/6,291,456; VRAM1,477,632/6,291,456. New moving route:
47,970,440/71,303,168 normal+stack,5,962,112/25,165,824 linear, same VRAM.
Measured main-stack upper bounds25,908/25,964 bytes (Old/New); these are sampled
watermark evidence, not exhaustive call-chain bounds. Old pools retain more
than15% sampled headroom but less spare memory than New. Console performance
and unvisited-area allocation peaks remain unverified.

The user reports the room-view hang fixed with installed4d04 and original v11
assets, and is still testing. Keep that evidence separate from this asset pack.

Further interior coverage on 7 October uses New emulator binary
`279bf40acf4b` and the same candidate pack. Both ordinary-input routes reached
the lower hall, but stopped at collision before the encounter-6 EarthWalker
dialogue. The west route is experimental, not a successful navigation fix.
The runners were deliberately cancelled after these failed assertions and
confirmed their owned emulators exited; neither run is a clean gameplay or
save/reload pass. Commands and failures are preserved in
`artifacts/research/native-proxy-interior/` and the corresponding
`artifacts/native/native-proxy-interior[-west]/` directories.

Inspected interior images show floor, columns and characters. The west image
also has a nearby enemy filling much of the view; its appearance has not been
compared against the original pack at the same pose. These captures therefore
add coverage, not proof of interior visual equivalence. Raw XWD captures were
archived with hash verification; PNGs and logs remain directly available.
Shrine, dialogue progression, matched character animation and physical FPS
remain unverified. Do not promote the candidate to installable on this evidence.
 prepare_native_proxies.py accepts --world-attributes and
--model-attributes with --topology. model_lod_simplify.cpp adds the actual
meshopt_simplifyWithAttributes adapter. World error includes RGBA(weight1) and
both UV pairs(weight.1). Model error includes RGBA1, UV.1, normals.5 and dense
bone-influence weights1. Bone IDs are categorical, never interpolated as IDs.
Layered models retain original topology because their second UV coordinates are
not resident in the packet. Source assets, collision, callback/material ordering,
animation, and every surviving vertex/reference byte remain unchanged.

Five actual-simplifier planar fixtures cover RGB, both UVs, normal and bone
influence peaks. Geometry-only max interpolation error .70957 falls to .11750;
reference count1536 falls to276 (294 for bone influence). Independent boundary
coverage and per-sample interpolation checks pass. This is a metric regression,
not a visual-equivalence or hardware-performance proof. Existing four semantic
corruption controls continue rejecting bad vertex, reference, callback and index
data even with repaired CRCs/manifests.

New private packs (not installed):
- world-only8948d18f3e34,11099948bytes; model237486->166518refs,
  world579378->516696refs. Audit558assets/3202groups/274523vertices/3289sections.
- full31fff2282a7e,11582432bytes; model237486->188790refs, same world516696.
  Audit558assets/3202groups/280279vertices/70987modelreferences/3289sections.

Exact installedNew4d04 binary, same copied save, same ordinary menu/load/idle HID
prefix. No overclock, no game-state writes. Three focused candidate runs all
complete Select shutdown and owned cleanup: topology2225 at3000 (122.104s),
world-only8948 at3000, full31ff at2500 (102.561s). Reuse prior originalv11 run.
Window rendered frames1900..2099: original5415246refs/21627draws; full4589118refs/
21586draws (15.26%fewerrefs). Mean emulator submission15.181->15.093ms is
essentially unchanged, not a demonstrated meaningful whole-frame CPU gain.
No universal multiplier or physical FPS estimate. Asynchronous screenshots and
retrace windows do not guarantee identical simulation poses. At HID2430 all
candidates show dark/green ornament cones where original is yellow-lit; original
HID2100 and2700 also retain that light. World-only then model attribute additions
did NOT resolve the observed discrepancy. Keep all candidates local pending
asset/pass isolation; do not blindly tighten error and restart boots.

Full31ff sampled normal+reservedstack47842224/71303168, linear6126592/25165824,
VRAM1473536/6291456, measured mainstack25916/524288. Exclude pre-renderer
uninitialized VRAM and unmeasured stack sentinels from peaks. Not extended-area
allocation proof. Old binary unchanged; previousOld candidate test does not
validate either new attribute-aware pack. Audio recorded but not audited here.

Seven completed runs' raw XWDs were losslessly zipped and every member hashed
before removing only its raw duplicate. PNGs, logs, audio and private saves
retained. archive-runs.py checks owned terminal state. All emulator/archiver
tool sessions ended; no console test pending. No remote files changed.
