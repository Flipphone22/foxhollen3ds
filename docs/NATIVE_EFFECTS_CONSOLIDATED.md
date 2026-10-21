# Shared effects integration, 14 October

The source-boundary specular prefetch and resident water-detail candidates had
been validated separately. The new `effects-consolidated-v2` pair builds both
in one native renderer, based on `ripple-detail-v2`. This avoids losing the
specular work when continuing from the newer water snapshot. It does not
reconnect any retired rendering backend or change game simulation or assets.

Six specular scheduling/material files change relative to the water candidate.
Their normalized implementation text matches the previously tested prefetch
candidate exactly; the scene header additionally retains the water branch's
`fh_pica_scene_reserve_changed` declaration. All18 linked GPU programs and the
original game archive remain identical to the water baseline. The existing
Old80/New124 CIA profiles and installation recipes are retained. Builders isolate
the selected snapshot and restore the worktree/archive in `finally`.

The initial v1 preflight rejected that extra header declaration before modifying
the worktree or building anything. V2 explicitly preserves it and checks the
remaining text against the tested prefetch sources. The reused tests cover map
publication/expiry/late repair, actual material masks, ownership and equations;
they are not rerun merely to duplicate unchanged evidence. See
[NATIVE_SPECULAR_PREFETCH.md](NATIVE_SPECULAR_PREFETCH.md) and
[NATIVE_RIPPLE_DETAIL.md](NATIVE_RIPPLE_DETAIL.md).

Pinned Melee's `citro3d_fix.c` was consulted again: mutable resource addresses
are not sufficient for binding reuse, and the texture-cache invalidation must
remain. This integration changes neither SDK publication nor queue ownership.
No new shader, geometry pass, worker, allocation policy or hardware register
operation is introduced. Prefetch metadata costs remain those of the existing
candidate; dual ripple meshes retain their documented15,488-byte increment.

The Old-profile palace check completes in60.32 host seconds with status1, saves
preserved, log drain and owned emulator exit. Capture1410 was inspected: Krystal,
the palace, purple creatures, effects and HUD remain present. There are788 full
map updates,532 half-map copies,3,036 demand hits/four late repairs and23,447
specular material draws with zero unready. Avoiding256 copies relative to the
eager rule for these same full updates is **not** a total-frame speedup claim.
The normal build and unrelated foreign compilation overlapped this correctness
run; timing comparison is explicitly excluded.

The New-profile Thorntail route completes in72.76 host seconds, with state/save/
log-drain/owned-exit checks passing. It submits9,036 fast ripple draws alongside
28,106 specular material draws, with zero unready or unavailable effects. There
are2,946 full-map updates and2,464 half-map copies (482 fewer than eager copies
for those same updates). Both features remain optional. Captures1500/1650/1740
were inspected: world/Fox/HUD remain visible, but vegetation and changing poses
limit ripple-specific image acceptance. This is not a full water fidelity gate.
Timing comparison is excluded due observed compilation, as in the Old check.

| Sampled resource | New Thorntail | Old palace |
|---|---:|---:|
| Normal heap used/reserved |49,230,272/71,303,168 B|41,079,592/50,331,648 B|
| Linear free/reserved |16,631,168/25,165,824 B|3,234,176/10,485,760 B|
| VRAM free |4,690,944 B|4,723,712 B|
| Main stack measured/reserved |73,044/524,288 B|72,984/524,288 B|
| Process commitment/grant |103,415,808/130,023,424 B|67,764,224/83,886,080 B|

These are different scenes and sparse samples; they are not a cross-hardware
performance comparison or continuous peak proof. In particular, the prior Old
Thorntail1.86MiB linear headroom still matters; do not substitute the palace's
larger free-space measurement for that workload.

Normal setup source`6f0292b8f0e3`/3dsx`1e88a0def639`/CIA`1cbf45390901` and
replay source`664d6f45567b`/3dsx`e50380ba87c5`/CIA`28887873e431` are built locally,
**not deployed**. Both share615 inventoried sources, game archive`c996af790c5a`
and18 linked GPU programs. CIA grants, installation recipes and full worktree/
archive restoration pass. No fresh normal-app boot or ISO conversion is claimed;
unchanged setup evidence and actual prior UI/settings coverage are reused.
Reproduce with `build_effects_consolidated.py`, `run_effects_consolidated.py`,
`analyze_effects_consolidated.py` and `verify_water_sheet_builds.py --family
effects-consolidated --revision v2`. Evidence is in
`effects-consolidated-runtime/integration.json` and
`water-sheet-runtime/consolidated-build-verification.json`.

No console installation, private assets, saves or preferences have changed.
Physical whole-shadow flicker remains unconfirmed. Inspection of the actual
snapshot's live receiver publication did not establish another ownership defect;
the authored range tree is intentionally independent of animated positions.
Do not cite tests of the root's older triangle shader as current point-program
evidence. Full rendering, physical performance and progression acceptance remain
incomplete; the main goal is active.
