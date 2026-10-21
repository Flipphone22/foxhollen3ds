# Native fur wind candidate

11 October: normal `a6dd17597b51` / CIA `9776451d707f` is a **local comparison
candidate**, not deployed or accepted as a fur repair. The installed New build
remains `86b336c4bfa5`. Projected fur lighting remains unimplemented.

The earlier wind prototype predated the corrected homogeneous texture
coordinates and used attribute6, which now belongs to authored groom offsets.
`pica_fur_wind_projective_shader.py` combines the source caustic lookup with the
current shell/groom shader, preserves Q until fragment sampling, and uses fixed
attributes7–11 for lookup constants. Both source scroll values are read from
`newshadows_getReflectionScrollOffsets`, scaled by the original one-half factor.
The original `newShadowsBeginFrame` advances these values; no new animation clock
or simulation changes are introduced.

An isolated `FUR WIND` option selects a separate327-instruction program. OFF
selects the existing216-instruction fur program, whose linked bytes are unchanged.
Both use96 float registers,16 booleans and7 output registers. All prior shader
blobs, including ordinary scene, groom, shadows, weather and lightning, remain
unchanged. Existing frame ownership, invalidation and command publication are
retained. Consulted the pinned Melee persistent binding/cache discipline; no raw
GPU register path, new worker, geometry rebuild, extra shell, texture or target.

The lookup recreates the source64×64 caustic field and its filtered byte lookup
at each vertex. **Interpolation between vertices is still an approximation.**
The previous real-mesh audit found large coordinate errors on some triangles;
this integration does not erase that result. Its purpose is to measure the
actual native-resolution image and cost, rather than declare fidelity from a
coordinate threshold alone. It must not become the default or a deployed “fix”
without evidence of an improvement. Stream-skinned fur currently retains its
existing path and does not consume this wind specialization.

`test_pica_fur_wind_projective.py` executes the actual assembled shader through
Azahar's interpreter:32,768 ON/OFF cases, groom combinations,851,968 retained
position/colour/lighting/coverage lanes exact, maximum source lookup error
0.219074 noise texels. A missing-Q mutant fails. Float32 inside Azahar's float24
type is not physical PICA precision evidence. The first harness compilation
duplicated constants/oracle functions inherited from its fixture; those duplicate
definitions were removed before the passing check.

`fur-wind-candidate-v1-builds/verification.json` checks the linked shader, every
archived source hash and shared CIA Old80/New124 metadata. The builder isolates
the lightning stable snapshot, rebuilds only the affected game object unit and
restores unrelated WIP, build scripts and the game archive in `finally`. The
wind patch is **not applied to the ordinary worktree**. Continue using
`lightning-v1-builds` as the accepted stable source; do not blind-build VRAM WIP.

The same normal3dsx passed two serial New-profile cockpit runs, with Select
shutdown, drained logs, owned-process cleanup and unchanged saves. Captures900
were inspected: Fox, fur silhouette, world, movie and HUD remain. ON records
11,041 wind draws/37,064,637 fur indices; OFF11,056 draws/37,114,992 indices.
The one active-frame difference is retained in the comparison.

Approximate total actor submission divided by fur-object groups is
1.255421ms ON versus1.240853ms OFF, +0.014568ms. Direct fur-owner spans are
0.327957 versus0.313083ms and overlap that actor total; never add them twice.
The host runs took60.38/60.63 seconds, but capture/host noise and one-frame
workload difference prevent a speedup claim. The shader's larger GPU workload
is not established by these CPU figures. Provisional added CPU gates remain
0.20ms New/0.50ms Old; Old runtime and physical GPU gates have not been tested.

Both runs: normal heap53,620,392/71,303,168 B; minimum free linear
18,156,160/25,165,824 B; free VRAM4,822,016 B; measured stack72,956/524,288 B;
process102,719,488/130,023,424 B. These compare ON/OFF within the new binary,
not the memory overhead against the deployed app: the optional shader has
resident code/program storage even when OFF.

The first OFF attempt was rejected by disk preflight before an emulator started.
Completed ELF/map evidence was NTFS-compressed with before/after SHA checks;
paths and bytes were preserved, then the check ran successfully. Captures remain
losslessly archived. Evidence: `fur-wind-projective-test`,
`fur-wind-candidate-runtime/comparison.json`, and the two
`artifacts/native/fur-wind-candidate-v1-{on,off}-new` folders.

Next acceptance work: a controlled original-renderer comparison at native output
resolution, including motion, before accepting the vertex approximation. If it
does not improve the image, replace its representation rather than stacking
additional passes or treating extra movement as fidelity. Projected lighting,
source-image acceptance, full visual parity and hardware performance remain open.

Follow-up: the [native-resolution source comparison](NATIVE_FUR_REFERENCE_COMPARISON.md)
now verifies a 400x240 framebuffer and matching 5:3 camera. It does not establish
an improvement sufficient to accept this wind candidate; phase-matched motion,
projected lighting and physical GPU cost are still open. No new deployment.
