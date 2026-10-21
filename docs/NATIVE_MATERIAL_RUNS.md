# Native material runs and renderer branch reconciliation

15 October 2026. Goal remains incomplete. No console deployment or physical FPS result.

## Experiment and result

`material_run_patch.py` retains a material publication only within an uninterrupted
actor/world-block call. New objects, changed formats, external invalidation and
command-buffer/pipeline resets force full publication. UI, moon and movie owners
are excluded because they override per-group culling or own special sampler state.
No geometry, game work, GPU shader, fence or pass was removed. The design follows
the pinned Melee raster-state ownership checks (`native-3ds/port/3ds/raster_state_verify.h`).

The actual archived binder and retained wrapper pass 196,608 ASan/UBSan SDK-state
comparisons. Six mutants fail when restart, format, compact layout, special-owner
exclusion, external invalidation or full-binder retirement is omitted. Shader,
sampler and SDK calls in this test are modeled; it is not a physical GPU proof.
The complete-branch test also exercises a foreign actor-height-fog bind between
ordinary draws. Test extraction was corrected to select function definitions,
not the earlier call site in the actor-height wrapper.

The cave workload is a poor match for this cache: 1,752 reuses against 164,049
full material publications, about 1.06%. No net CPU speedup is established.
**Do not promote this per-draw cache.** It adds work to every miss. If revisited,
compile eligible runs or batches during loading and avoid checking/cache-writing
for groups that cannot reuse state. It must preserve transparency, culling,
material animation, shader transitions and draw visibility.

## Branch problem caught by coverage

The first candidate used the latest normal-build archive, which lacked the actor
height-fog renderer present in the established replay branch. Its cave run exited
normally, but the required actor-fog coverage assertion correctly failed. V1 is
not a valid replacement for the fuller replay. This was an inherited branch gap,
not caused by material retention; no candidate was deployed.

The complete integration starts from `actor-fog-headroom-v1-console-setup-builds`
and applies the required all-preset lighting policy, CPU display/details, LCD
flush coalescing, presentation ownership/components and camera retention. Its
validated game archive is `2f219edec50e03e868c5b11931e3b6f094bde2c0c81b6d0c6196af0f775fa22a`.
Both variants must use that same archive and the complete source inventory.

`material-run-v2` tested this composition with the experimental cache still on.
Old80 cave replay completed in 67.22 host seconds, with status1, saved fixtures
unchanged, drained logs and owned-process exit. It submitted 48,399 actor-fog
draws. The 1410 capture was inspected: Fox, cave, ladder, tunnel, mushrooms,
textures, HUD and fog are present. Animation phase varies; this is not full image
parity or an FPS comparison against the fog-less V1 run.

Sampled Old80 memory: normal heap42,003,728/50,331,648 bytes; minimum linear free
4,188,800/12,582,912; initialized VRAM free4,723,712; measured main stack
72,984/524,288; process69,898,240/83,886,080. Thirteen unmeasured stack samples
contain the reservation as a placeholder and are excluded from the measured peak.
These values cover this cave fixture, not all areas or extended gameplay.

## Accepted integration and build guard

`renderer_reconcile_patch.py` keeps the full renderer and the accepted CPU work,
but excludes the low-hit material cache. `build_renderer_reconcile.py` creates
isolated normal/replay builds and invokes `verify_renderer_reconcile.py` after
restoring root state. The guard requires the exact complete source composition,
unchanged full-branch game archive, all 18 parent GPU programs, linked native
actor-height-fog owners and Old80/New124 CIA profiles. It rejects the experimental
material-run entry point. The future installer builder also carries camera
retention; it does not carry the material cache.

The final reconciliation is not a new FPS fix. Runtime evidence from V2 is reused
only for the unchanged full integration; removing the cache restores the exact
prior full binder that was the test oracle. Final archives are not claimed to
have received separate runtime boots. No repeated normal conversion or unchanged
probe is needed for this removal. Full performance, visual parity, progression,
save/reload and endurance acceptance remain outstanding.

Evidence: `material-run-test/`, `material-run-complete-test/`,
`artifacts/native/material-run-v1-old/` (coverage failure retained),
`artifacts/native/material-run-v2-old/`, and the reconciliation archives'
`renderer-contract.json` and paired acceptance record.


Final local identities: normal `0f554ac7cea2` (3dsx `12d65c148723`, CIA
`1e60693af941`); replay `a3abb959a20e` (3dsx `c610952d33db`, CIA `0a4dffb57f23`).
Both guards pass; 685 source records and game archive match across variants.
?Full branch? means the currently implemented actor-fog rendering branch, not
complete original-game visual parity. The next source-level cost opportunity is
the actor-fog draw owner's unconditional pipeline invalidation at each object
entry/exit; it currently prevents compatible consecutive objects from retaining
state. No such retention change is included in these builds yet.
