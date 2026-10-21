# Batch-resident water transforms

14 October: experimental continuation of the scoped native water batch. The
previous turn made concrete progress by replacing repeated state setup and
checking GPU pixels and New/Old gameplay. Its performance gate remains open;
another project's compiler is still active, so this turn does not repeat
contended game timing runs to claim a speedup.

The prior batch still uploaded a reflection matrix and indirect scale before
each of its two draws per instance. Both matrices are constant for the family
batch. The candidate uploads them once when it publishes common state, then
selects the appropriate transform and scale with one vertex-shader boolean.
Full queue resets republish both matrices and both scales. The boolean is set
on every pass, including after a program handoff. Blend order, both independent
projective denominators, geometry, mask, tint, depth and fog are unchanged.

The first matrix keeps uniform7..9; the second uses15..17 from existing padding.
Uniform14 contains both pairs of source-texel scales. Fog stays94 and all other
uniform locations stay unchanged. Initialization validates the added float
location15 and SDK boolean location0x68; the SDK setter also takes0x68 and
subtracts that base internally to select hardware boolean0.
This preserves the pinned Melee full-publication discipline through SDK APIs.
It introduces no relative uniform addressing, GPU register packets, resource
copies, CPU vertices, allocations, worker or additional pass.

## Explicit cost tradeoff

For a30-instance batch without a queue reset, the actual owner test counts135
float-uniform setters, including the fog setter. The prior source performs
`8 + 12*N`, or368 for30 instances. The candidate performs`15 + 4*N`, plus two
boolean setters per instance. This is233 fewer float setters (63.3%), **not**
an ARM instruction-count result or a measured CPU-time reduction. AtN=1 only
one float setter is saved while two boolean setters are added. Frequent queue
resets can also reduce or reverse the CPU benefit.

The separate water program grows31→36 stored instructions and uses one boolean.
Both assembled branches execute32 instructions per vertex versus31 previously.
The extra uniform branch has hardware costs that are not established by this
count. No net speedup, GPU-time reduction, or physical throughput is claimed.
All other17 GPU programs are unchanged; the large shared scene program is not
expanded. Promotion requires a matched CPU/GPU cost comparison.

## Correctness evidence

`test_water_uniform_shader.py` executes65,536 vertices in the pinned PICA
interpreter against both the prior shader and an independent double-precision
oracle. All24 output lanes are bit-identical to the prior shader, including
distinct projective denominators, distortion, opacity, mask and fog. Relative
oracle error remains2.12063e-5. Wrong second-Q, omitted displacement-Q, wrong
UV fraction and lost fog mutants fail. Assembled output/control-flow checks
pass. `count_water_shader_paths.py` counts both branches independently.

Actual two-plan owner/compiler/reducer tests cover16 lifecycles,6,178 draws,
five admission failures and30-instance no/every/mixed-reset batches. The test
checks retained geometry, both resident transforms, scale selection, live
pose/opacity and reader retirement under ASan/UBSan. Lost boolean selection,
wrong second-transform slot, reset, pose, fog, retirement and earlier coverage
mutants are rejected. The corrected check uses the actual pinned SDK
`C3D_BoolUnifSet` inline implementation, including its encoded location and
dirty-bit behavior. An unencoded-index mutant is rejected too. These tests
still model GPU service boundaries; the GPU fixture and linked builds provide
separate evidence.

Evidence: `water-uniform-shader-test/{result,instruction-paths}.json` and
`water-uniform-owner-test/result.json`. The first application build was stopped
by the existing exact31-instruction guard. The builder now requires the tested
36-instruction program and its exact assembled hash; the guard was updated,
not removed. Failed build and restoration records remain available.

The first GPU fixture completed, but failed pixel comparison: the second pass
sampled the first reflection. The candidate passed hardware boolean index0 to
an SDK API that expects encoded location0x68. The interpreter had been given
boolean0 directly, and the initial host stub repeated the same wrong API
assumption; neither proved SDK publication. Replacing that stub with the real
SDK setter exposed the mistake. The owner now passes0x68. The failed GPU
images/result remain in `artifacts/native-water-uniform-device/v1`; the initial
host result is retained as `result-initial-modeled-setter.json`. No failing
candidate was deployed. Corrected GPU acceptance is recorded separately below.

The corrected GPU fixture (`artifacts/native-water-uniform-device/v2`) passes
all eight cases, 48 draws, and full/retained-state handoffs in 10.73 host seconds.
Its eight images are pixel-identical to the prior batch fixture; the mirrored
second-channel image was also inspected. Independent pixel-oracle maximum
mean/p99/worst errors remain 0.4253/1.5876/2.1144 byte levels. This validates
the tested GPU output and SDK selection, not original-game parity or speed.
`baseline-image-equality.json` records the exact image comparison. Owned exit
completed normally.

Both v3 variants are built locally, **not deployed**:

- Replay source `3a090e405173`, 3dsx `07214fa4f2e0`, CIA `3958f9719f25`.
- Normal setup source `6525adee8679`, 3dsx `d55fe58d6a24`, CIA `d8d608f86159`.

The `water-uniform-v3-*-builds` snapshots share all 615 inventory entries,
game archive `c996af790c5a`, and 18 linked GPU programs. Water's linked shader
is `de27770c801b`; the other 17 shaders are unchanged. Shared Old80/New124 CIA
profiles, installation recipes and worktree/archive restoration pass.
`water-sheet-runtime/uniform-build-verification.json` is explicitly static
identity evidence. There was no new normal-app boot or full game replay for
this candidate; the preceding batch game's coverage remains separate.

All owned build/test processes have finished. Unrelated compilation prevented
a clean host timing comparison; it was not interrupted. No performance target
is accepted from these results. This is an experimental successor to the water
batch snapshot, not promotion of other independently retained experiments.

The game archive, assets, saves, settings and deployed consoles are unchanged.
Both effects remain OFF in all presets. Filtered GX bump-alpha quantization,
original-game visual acceptance, clean performance and physical shadow-flicker
acceptance remain incomplete. See `NATIVE_WATER_BATCHES.md` for prior resources
and integration evidence.
