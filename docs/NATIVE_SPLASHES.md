10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native splash rings (local candidate)

The engine-19 expanding splash ring now has a native PICA backend. This is a
partial water effect, not reflection or complete water-rendering parity. It is
not deployed. The page-7 SPLASHES control defaults off on both hardware models.

The source ring has 128 unique vertices and 630 indices. These are prepared once
when the water resources load and retained in a 4,864-byte linear allocation.
The two textures use the existing native image pool. Each splash supplies eight
affine band transforms and alpha values, the live camera, viewport and ambient
colour. The GPU transforms the ring; no per-frame CPU vertex expansion, geometry
copy, source interpretation or asset read is introduced. The two original
back/front cull passes remain. No extra thread or render target is used.

This follows the pinned melee-3ds ownership pattern: flush immutable data before
publication, retire queued readers before release, and retain command-frame
ownership in the host. New 3DS core 2 is already requested by the audio worker;
the implementation does not assume two additional idle graphics cores.

The added vertex entry uses 24 instructions. The actually linked shared program
contains eight entries and 500/512 instructions. The actor entry has no new
branches. This is little remaining instruction headroom; further effects must
use suitable existing programs or an explicitly managed separate program rather
than overflowing the hardware limit. The build verifies assembled/linked bytes.

Material limitations are explicit: native ambient plus foam is used in place of
the reflected-scene samples. Continuous mask alpha differs from the source's
five-bit indirect-alpha quantization by at most 7/255. This does not implement
ripples, wakes, drops, the separate lightmap splash quad family, reflections or
distortion. The retired CPU-expanded minimal splash path is not called.

Validation so far:

- `tools/test_pica_splash.py` compares the actual source matrix/strip producers:
  all 630 indices and alpha bytes match, with 393,216 transformed coordinate
  samples differing by at most 0.0009765625 game units from float operation order.
  Simulation, splash/ripple spawning and RNG-bearing producers remain unchanged.
- `tools/test_pica_splash_lifetime.py` exercises the actual native owner over
  128 release/reload cycles and 2,562 draws under ASan/UBSan. No draw allocations
  or geometry writes occur. Missing-retirement and shutdown-reopening mutants
  are rejected. This is not GPU visual validation.
- The first v2 cave run correctly failed at shutdown: cleanup reopened a frame
  after host UI storage had been freed. Mid-game retirement and already-retired
  shutdown cleanup were separated. The v3 cave run passes state checks, log
  draining, save preservation and owned-process exit. It records zero active
  splash objects, so it validates resource lifecycle and unaffected rendering.
- New cave v3: 59.831226 Azahar FPS, scene 1.821909 ms, submission 5.070879 ms;
  prior alpha candidate submission 5.074215 ms. No meaningful change established.
  Heap peak 48,110,136/71,303,168 bytes, linear minimum free
  19,451,776/25,165,824, VRAM free 4,858,880, process
  103,137,280/130,023,424, measured main stack 72,836/524,288.
- v4 also corrects native viewport/rotated-target publication. The first short
  Thorntail movement run contains zero splash draws. The subsequent ordinary
  movement route exercises 67 splash submissions on New and 182 on Old; both
  pass state checks, draining, save preservation and owned exit. The New capture
  `splash-thorntail-route-new/hid-1770.png` shows the ring at Fox's feet with
  world/actors/HUD intact. Old captures also retain those scene elements and
  the touch item stick. This is limited scene coverage, not original-material
  visual parity or physical acceptance.

The same New route with SPLASHES off/on measures 56.6363/56.3648 Azahar FPS
over approximately 22 seconds. Native submission averages 3.55229/3.54481 ms;
scene work 1.78441/1.77463 ms. These are short runs with different animation and
timing-dependent paths, not instruction-identical workloads or evidence of a
speedup. No large regression appears. The splash-specific CPU submission mean
is 0.03745 ms/object; GPU cost is separate. Old ON measures 49.1614 Azahar FPS,
submission 3.54954 ms and 0.03751 ms/splash. No Old OFF pair or physical prediction
is claimed. Old and New generated different splash counts; this is not a
determinism test.

Thorntail peak normal heap is 48,741,368/71,303,168 New and
41,418,072/50,331,648 Old. Linear minimum free is 18,692,352/25,165,824 New and
4,011,264/10,485,760 Old; initialized VRAM free remains 4,858,880 on both.
Process commitments are 103,137,280/130,023,424 and 67,485,696/83,886,080.
Measured main stack is 72,908/524,288 on both. Startup's unmeasured stack
reservation is excluded from the measured peak. See `route-validation.json`.

Evidence is under `artifacts/research/pica-scene-rewrite/splash-native-test/`
and the `pica-splash-v*-builds` archives. Latest local replay v4 is
`c04354383df2`. There is no splash normal-build or deployment claim.

Feature gates remain: matching active ON/OFF captures; no missing actor/world/HUD
draws after handoff; bounded residency and transitions; New added CPU <=0.25 ms
for the measured splash workload and Old <=1 ms, with GPU/frame time evaluated
separately. Emulator timing is relative evidence only. Expensive or unvalidated
effects remain opt-in rather than silently spending the gameplay frame budget.
