# Scoped native water submission, 14 October

The ripple visibility candidate saved geometry but failed the CPU feature gate.
In clean New-profile Thorntail routes, its ON rendering CPU total was8.2761 ms
versus7.5418 ms OFF: +0.7343 ms, above the0.20 ms allowance. Bounds alone are
insufficient. These are guest timings with different source animation counts,
not physical-console timings or instruction-identical replay evidence.

`water-batch-v2-console-replay-builds` replaces repeated state reconstruction
inside the two-plan native sheet owner. It does not restore a GX renderer or
build vertices on the CPU. The source game adapter first copies up to30 live
model-view matrices and opacity bytes into a fixed1,560-byte array. It then
submits that family as one native scope, preserving instance order and keeping
both transparency passes adjacent per instance. Ripple/splash/drop/wake order,
camera math, original simulation and random calls remain unchanged.

Camera, viewport, shader, samplers, geometry and fog state are common within
this scope. Material pass differences are compiled once at load time. The
native owner publishes common state for its first admitted draw; later draws
change only pose, opacity, reflected coordinates and pass-specific material
state. There are no intervening game callbacks. The existing command reserve
now exposes whether it split/reset the queue. A reset, including on the second
pass, forces complete publication again. The old void reserve API retains the
same behavior for every other owner. No fence, cache flush or resource-lifetime
requirement was removed. Fog contributes once, and source blend order remains.

This follows the pinned Melee `native-3ds/port/3ds/citro3d_fix.c` ownership rule:
equal resource addresses alone do not establish unchanged mutable state. All
changes go through SDK dirty-state APIs. Unlike Melee's cross-draw descriptor
comparison cache, this optimization has a narrow explicit scope with immutable
common parameters and an explicit queue reset signal. It emits no custom GPU
register packets. Existing conservative water bounds remain before submission.

## Checks

- Actual two-plan owner/compiler/reducer:16 lifecycles, allocation failures,
  alternating plans,30-instance batches, no-reset/every-reset/mixed-reset
  schedules, live pose/opacity and retired readers pass ASan/UBSan. A no-reset
  batch makes1 full publication and59 retained publications instead of60 full
  publications. Lost queue reset, live pose, fog, retirement, UV, coverage,
  indirect scale and visible-cull mutations fail the checks.
- Actual generated game adapter: eight simulation/spawn functions unchanged;
  independent OFF/ON, empty/unready pools, one capture, source ordering and
  distinct per-instance snapshots pass. Removing the pose copy fails.
- Actual reserve helper: capacity threshold, returned reset signal and old
  void behavior pass; a false reset signal fails.
- Actual GPU fixture: eight silent Azahar cases, three instances per batch,
  48 draws,28 full and20 retained publications. Retained and forced split
  paths pass the existing independent pixel oracle. Maximum mean error0.4253,
  p99 error1.5876 and individual error2.1144 on a0..255 scale. Case2 inspected.
  This checks the prepared approximation, not original-game image parity,
  exact GX filtered bump-alpha quantization or physical GPU performance.

Evidence: `ripple-batch-owner-test`, `water-batch-adapter-test`,
`water-batch-reserve-test`, and `artifacts/native-water-batch-device/v2`.

The initial v1 game build stopped because its early declaration preceded the
source's pool-size macro. The fixed array is now explicitly30 entries with a
source-pool static assertion at the consumer. No failed binary was run.
The first GPU fixture timed out because a fixture-wide32-bit enum switch
violated the native SDK ABI. The real platform owner uses devkitARM's default
short enums; the game/reducer's32-bit boundary exposes only integer, float and
pointer data. Restoring the fixture's matching SDK ABI resolved this failure.
Both failed attempts and successful restoration records are retained.

The local replay rebuild changes only game19.o and nine source files. All18
GPU programs remain identical; the water program stays31 instructions.
Old80/New124 CIA profiles and worktree/archive restoration pass. Both prepared
plans still occupy81,920 bytes even when OFF. No assets, saves, preferences or
console installations have changed. RIPPLES remains OFF in all four presets.

Runtime cost, memory and normal-build acceptance are recorded below when
available. Full visual parity and physical performance remain incomplete.

## Shared build and ordinary gameplay checks

Replay source`e4e84ba77eaf` (3dsx`e9069f94cb31`, CIA`d8e2cc5de35e`) and normal
setup`26df937cf7fb` (3dsx`0e7874a307e1`, CIA`7337b4f76a1b`) are built locally,
not deployed. Exact615-source, game-archive and18-program equality passes,
with both installation recipes unchanged. The prior normal setup/menu evidence
is reused for unchanged UI behavior; this pair has no new normal-app boot or
full ISO conversion test. `water-sheet-runtime/batch-build-verification.json`.

Serial New/Old ordinary Thorntail routes pass in88.91/77.82 host seconds:
status1, original saves unchanged, logs drained, owned emulators exited. Both
1740 captures were inspected: terrain, actors, HUD and water effects remain.
New submits5,265 ripple instances/10,530 draws across939 batches; full state is
published939 times and retained9,591 times (91.08%). Old submits4,399 instances/
8,798 draws across861 batches, retaining state7,937 times (90.21%). No queue
reset happened within these short game batches; the host and GPU fixture
explicitly exercise that case. New rejects804/6,069 candidate sheets and
Old485/4,884; visible source counts differ, so these are not identical workloads.

| Sampled resource | New profile | Old profile |
|---|---:|---:|
| Normal heap peak |49,228,456/71,303,168 B|41,691,776/50,331,648 B|
| Minimum free linear |16,641,536/25,165,824 B|1,964,544/10,485,760 B|
| Minimum free VRAM |4,690,944 B|4,690,944 B|
| Measured main stack |73,044/524,288 B|73,044/524,288 B|
| Process commitment |103,407,616/130,023,424 B|67,756,032/83,886,080 B|

The fixed pose array adds1,560 bytes of ordinary static storage; there are no
new linear/VRAM resources or workers. The81,920-byte geometry and1,415,271-byte
temporary load reservation remain explicit. Sparse samples are not proof of
every possible peak.

Both runs overlap another project's compiler; the Old check also overlaps the
end of the normal build. Guest56.91/50.31 FPS and owner0.04310/0.04391 ms per
rendered ripple are retained for diagnosis, **excluded from performance
acceptance**. The New rendering total8.2888 ms cannot establish improvement
over the clean visibility candidate8.2761 ms. No repeated contended timing runs
were launched to manufacture a pass. A clean same-binary toggle comparison,
full source-game appearance and physical GPU cost remain open. The effect stays
OFF in all presets. Evidence: `ripple-runtime/batch-result.json` and
`artifacts/native/water-batch-v2-on-{new,old}`.
