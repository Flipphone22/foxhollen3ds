# Directional-light experiment — do not promote

Goal active/incomplete. Previous turn had no implementation progress; this turn
implemented, built and checked a GPU constant-work reduction, then rejected its
promotion using the actual scene's light distribution. Hardware unavailable;
no FTP, probe request, deployment, save or asset modification.

Continue from **actor-live-v2-builds** (normal actor-live-normal-v1-builds), not
directional-v3-builds. V3 is a research artifact: shader tests and New/Old
replays pass, but Old cave has zero directional lights and the changed shader
adds two instructions per point light. No net GPU/FPS win is established.
See [NATIVE_DIRECTIONAL_LIGHTS.md](NATIVE_DIRECTIONAL_LIGHTS.md) for code,
counts, outputs, timings, memory and known gaps.

Builders restore all replaced/absent/shelved files and libgame.a in finally.
Global WIP is not the stable build source. The new directional header exists in
the worktree but is unused by the stable snapshot. The patch was applied only
inside isolated builds. Never blindly build global WIP or reapply patches to
already-patched snapshots. All owned sessions are terminal:42117failedV1,
42275V2build,1440VMtest,21728V2New,52608V3build,45181compression,
83494V3New,59351V3Old. No normal directional build or live process remains.

Artifacts: directional-test/{assembly,shader,cache,build-verification,
build-verification-v3,runtime-v2,runtime,old-runtime-v3}.json;
directional-v{1,2,3}-builds. V1 CMP source restriction and focused CALLC operand
syntax errors were fixed before V2. V3 New initially failed the storage guard
before creating a run; resolved by preserving/compressing completed ELF files.
Compression manifests20261012-132107(12files269252608bytes) and132712
(16files357965992bytes). No compression/build overlapped emulator timing.

Next available action: normal-demand shader specialization for ambient-only
draws. Read current snapshot platform_pica_scene.c and material consumers.
The light log counts37,990enabled object publications but3,081active lights in
Old cave; environment bindings18,695 can also require normals. Account for
ordinary, layered, fur, morph and specular/environment handoffs before skipping
normal transforms. Compact model validation and refresh guarantee UV1=UV0 when
secondary_uv is false; runtime prepare compacts old direct packets when
fh_minimal_enabled. Audit this invariant in the **stable snapshot** before
reusing actor_second_uv's boolean. No such change has been implemented yet.

Potential shader layout: keep position skinning always; make the two bone-normal
branches use a dedicated normal-demand boolean. Put view-normal/environment
generation under that boolean, separately from vertex colour lighting. Keep
the source ambient/active-light colour operations under actor_lighting. A
specular UV consumer also needs view normals when vertex lighting is disabled;
do not key demand only on lighting.active or on the global LIGHTING option.
Changing the duplicate-UV selection to an unconditional prepared UV1 load could
fund the extra branch within the program cap. Verify the actual assembled
program and material/option handoffs; this is not yet a proven implementation.

Other open work remains: full shadow fidelity/budget, visual parity, SD stalls,
the complete performance/progression acceptance, and physical validation. The
index-order audit remains offline only; blending/equal-depth order must be
handled before a universal reorder. Do not recast an incomplete renderer or
single replay as the completed port.
