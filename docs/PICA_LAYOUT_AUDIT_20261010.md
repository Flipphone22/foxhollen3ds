10 October deployment update: New 3DS normal `0b55ba943be8` and replay `bddd446fde84` uploaded and readback-verified at user request. Saves/settings preserved; Old 3DS unchanged. CIA copies require FBI installation. Earlier local-only wording below describes the pre-deployment checks. Physical startup recovery and fur acceptance remain pending.

# Console startup regression and emulator coverage

The physical startup capture identified an invalid morph vertex-fetch layout:
40-byte stride with only 32 bytes described. The deployed correction uses the
original resident vertex base and explicit leading 8-byte padding, preserving
the exact attribute addresses. See [the repair record](MORPH_FETCH_FIX_20261010.md).
Physical recovery still needs confirmation; a successful emulator run alone
does not establish that the console hang is resolved.

## Catching this before another console deployment

`tools/build_azahar_pica_layout.py` builds an isolated diagnostic Azahar with
`tools/azahar_pica_layout.h` checked at `PicaCore::DrawArrays`, before either
accelerated or software submission. `tools/run_console_headless.py
--pica-layout-audit` selects it and rejects invalid/missing reports. It checks
component counts, element alignment, explicit padding and total fetch extent
against each enabled loader's stride. It does not modify guest commands,
resources or clocks. This is a focused hardware-contract gate, not full PICA
validation. Its host execution cost excludes these runs from throughput
comparisons.

The exact previous normal binary, with the console's saved graphics settings
and original assets, fails at draw 341: loader 1, extent 32, stride 40, 1116
vertices. That matches the layout and vertex count in the physical capture.
The repaired normal binary passes startup, cockpit playback and clean Select
exit: 721 retraces, 36,864 audited draws recorded, no invalid layout. The
540 capture was inspected. Evidence: `artifacts/native/pica-layout-audit-before-new`
and `artifacts/native/pica-layout-audit-after-new`.

Reports publish every 4096 draws. A `checking` report at owned-process cleanup
therefore gives a lower-bound draw count, with at most 4095 trailing draws not
included in the reported count; all those draws still executed the check.

## Additional optional-effect failure found and repaired locally

The Thorntail water route with particles, silhouette shadows, splash rings,
droplets, fur and specular enabled stopped at retrace 925 with `PICA fur missing
animated base`. This was a game-side fur adapter panic, not a layout failure.
The assumption that a previous ordinary pass had prepared the pose was wrong:
the original first fur pass can itself prepare animation, morphs and hit volumes.

The adapter now invokes that preserved state preparation only when needed,
sets the first-shell index to zero and preserves the supplied world transform.
Pass 4 explicitly returns after state preparation and before visual dispatch;
the removed GX instruction interpreter remains absent. Resident geometry and
GPU skinning remain unchanged. Already-prepared objects incur no second update.
No new allocation, worker or shader is introduced.

`tools/test_pica_fur_first_pass.py` checks 1000 first-pass and 1000 already-ready
cases with ASan/UBSan, including parent and world-transform preservation, and
audits the generated ARM source boundary. The corrected route completes and
drains logs, preserves saves and exits the owned emulator. It records 348,160
audited draws without a layout failure; 548 fur shell objects / 1644 draws,
98 splash draws, 28,823 shadow draws and 39,852 specular material draws execute.
The 1770 image was inspected: world, Fox and HUD are present. This does not
establish complete effect fidelity or physical performance.

Original failing evidence: `artifacts/native/pica-layout-effects-on-new`.
Corrected evidence: `artifacts/native/fur-first-pass-v1-on-new`.
Fur fix is local, not deployed. The installed New 3DS startup candidate remains
normal `4fe1d96ad2a1`, replay `2192fb6abc45`; Old 3DS is unchanged.

Final local builds are normal `eb279defa688` / CIA `0c660bcd0eee` and replay
`596b50a968e3` / CIA `55ee56ce5483`. A pointer-type cleanup after the runtime
check changed debug/source identity; every executable ELF section is identical
between the tested v1 replay and final v2 replay. The normal and replay builds
share the same game archive, 66 renderer sources and unchanged tested
511-instruction shader. Build profiles remain Old 80 MiB / New 124 MiB.
`fur-first-pass-test/build-verification.json` and
`type-cleanup-code-equivalence.json` record these checks.
