# Startup menu ownership

`startup_runner.c` connects the existing menu/presets to preparation and
diagnostic tasks. `platform_startup.c` supplies the 3DS LCD, input and clock
interface. The real task, generated recipe configuration and bundle path owner
now connect through `install_boot.c`. A conditional normal-entry patch calls it
before game logging, graphics and allocations. The complete shared normal
executable now runs this menu in headless Azahar in both hardware modes.
ISO-only installation remains incomplete: full conversion and the handoff
from the menu into gameplay have not yet passed integrated validation.

The exact v6 normal binary (`c7fd38d4ef18`) now also passes the New-mode
menu/preset/persistence/exit route in `setup-additive-menu-v2-new`, with both
preset captures inspected. The initial v1 harness rejected ambiguous static
`retraces` symbols before launching. Counter resolution now uses the
`platform_3ds.c` ELF symbol owner, matching the established console runner.

The custom HID emulator optionally accepts `FH_HID_COUNTER_ADD_ADDRESS`.
It adds the read-only startup poll counter and gameplay retrace counter, so an
ordinary input script can pass from the menu into the game without freezing at
the old clock. Each counter must remain monotonic; overflow, malformed addresses
and duplicate addresses are rejected. Missing mappings publish neutral controls.
No game-memory writes, clock adjustments or input-poll changes are introduced.
Host ASan/UBSan checks cover 201 combined-counter states and failures; the New
menu run verifies the startup side with the gameplay counter still zero. Actual
two-phase gameplay execution remains pending.

`prepare_install_game_fixture.py` and `run_install_game.py` provide the next
integration route. They intentionally distinguish a synthetic prepared test
bundle from genuine application conversion journals. The fixture builder
currently refuses the unfinished texture conversion before writing any fixture.
Its publication path and the game runner have not yet executed. A future pass
will establish consumption/handoff only, not complete on-console conversion.

The menu shows the source build ID, asset readiness, current preset and actual
task progress. Its task interfaces support Prepare/resume and up to sixteen
named diagnostics. A task only returns after retiring its files, workers and
GPU resources. Pressing B asks it to cancel through its progress callback;
the menu retains ownership until the task returns. APT exit follows the same
cleanup path. Readiness is independently checked against the complete committed
recipe, including immediately before Start. A successful individual task cannot
turn incomplete assets into a ready installation.

Progress polls input at most every sixteen milliseconds and publishes changed
text at most every hundred milliseconds. Idle menu text is not redrawn each
VBlank. Presets use existing graphics settings persistence. Display counters
are unaffected. The task layer creates no thread or GPU resource and has no
gameplay/save/clock hook.

The LCD wrapper follows the pinned Melee `port/3ds/game.c` ordering of gfx before
console initialization. It adapts the existing Foxhollow replay menu's separate
gfx lifetime and stdout/stderr restoration. Both LCD double buffers are cleared
and explicitly flushed before display to avoid the early uninitialized-bar
failure. A failed cache publication aborts/cancels the menu path; no unchecked
GPU access or direct register write is introduced. Diagnostics must retire their
own GPU owner before returning; the menu itself does not initialize Citro3D.

`test_startup_runner.py` passes nine synthetic task/input scenarios under
ASan/UBSan: task cancellation, APT exit, completed task with incomplete recipe,
successful preparation, preset persistence invocation, diagnostic selection,
changed readiness before Start and fifty idle polls with one draw. A one-second
task makes ten progress screen publications. Two initial harness assumptions
were corrected: starting from Custom selects a different preset row, and total
menu draws include navigation as well as progress. Those failed logs are kept.

All controller modules, existing settings and the real SDK screen wrapper
compile and link for ARM. The link-only ELF was not executed or packaged. Its
113,176 bytes text / 7,592 bytes data / 15,200 bytes BSS include SDK scaffolding,
not incremental game costs. Actual framebuffer/lifecycle behaviour, captured
screen layout, full recipe wiring and gameplay handoff still need integrated
emulator validation before deployment. Evidence:
`artifacts/research/startup-runner-test/result.json`.

The committed-bundle layer and real sequential converter task now exist;
see [INSTALLATION_BUNDLE.md](INSTALLATION_BUNDLE.md). `fh_install_task_ready` and
`fh_install_task_prepare` match this controller's callbacks, reject an incomplete
installation and retire their resources before returning. `install_recipe.py`
generates recipe/source configuration; `install_boot.c` binds selected paths
during the final readiness check and then locks them before game startup.
The complete normal setup build is archived at
`artifacts/research/normal-setup-v3-builds`. Its source ID is `9856a61aa4fd`,
3DSX SHA256 begins `004667a8d5ea`, and CIA SHA256 begins `fb4fdb6b7d50`.
`build_install_normal_v1.py` isolates the checked renderer, applies the asset
binding and typed-texture patches, builds and packages the application, then
restores unrelated working changes and the game archive. The exact 544-file
source closure is archived. `verify_install_normal.py` verifies that closure,
449 unchanged existing files, the unchanged game archive and 511-instruction
linked shader, and the Old 80 MiB/New 124 MiB shared CIA profiles.

`run_install_menu.py` uses standard scripted HID input, a read-only menu poll
counter and isolated profiles. New 3DSX and Old CIA-content runs passed missing
asset rejection, missing ISO reporting, preset selection/persistence, an empty
diagnostics page and normal exit. Captures were inspected for readable text and
correct hardware/build identification. A third run opened the original private
ISO, displayed fingerprint progress, cancelled through B and returned safely.
It did not prepare or publish an asset stage. The private ISO hardlink was
removed afterward; source size/mtime and isolated save markers were unchanged.
All owned emulator processes exited. Evidence is under
`artifacts/native/setup-menu-v1-{new,old}` and
`artifacts/native/setup-source-cancel-v1-new`.

The wrapper emits one small startup receipt after retiring its LCD resources;
it does not write per-poll logs. These checks establish menu/cancellation
behaviour, not whole-install memory peaks, conversion time, gameplay FPS or
physical-console correctness. Diagnostics registration is still empty. No setup
build has been deployed, and existing installed builds/assets/saves are unchanged.

The next local build, `normal-setup-v4-builds` (source `b83724b445b4`,
3DSX `1f8f5861e2a1`, CIA `923c2aed5f4f`), extends the existing Melee-derived
CPU/L2 owner to the startup menu. Previously its initialization occurred only
after this menu. The wrapper now initializes that owner, polls its pending
wakeup request after APT polling, and unhooks it on every return before gameplay
creates its own owner. The pinned `cpu_speed.c` policy itself is unchanged:
Old 3DS does not receive a fast-mode request, and unknown clock-query results
are not treated as proof of an actual frequency. No extra worker is introduced.

`startup-cpu-test` exercises the actual LCD wrapper and CPU owner with a mock
SDK in eighteen New/Old start/exit/APT-exit/wakeup/flush-failure combinations.
ASan/UBSan checks pass, stdout/LCD ownership is restored, and a missing-unhook
mutation fails. Its first mock tick value was less than one millisecond;
that fixture assumption was corrected and the failure retained. These are
lifecycle tests, not evidence of a physical CPU frequency or conversion speed.

The complete v4 New ISO-cancellation run passed in 9.893 host seconds; the Old
menu/preset/exit run passed in 14.711. Captures were inspected. Those elapsed
times overlap host conversion work and are not performance comparisons.
`setup-cpu-cancel-v1-new` and `setup-cpu-menu-v1-old` contain receipts and captures.
The only changed source relative to v3 is `platform_startup.c`; the generated
conversion recipe/header is identical. The same saved game archive, linked
shader, and shared CIA memory profiles remain verified.

The isolation helper previously packaged the CIA twice. The second envelope
could differ after `build.json` was written, although the verified CXI content
remained identical. The helper now trusts and checks the packaging already done
by `build_native.py`. V3/v4 metadata was reconciled against final byte hashes;
each original report and correction record is preserved. No binary was changed.
`verify_install_normal.py` now rejects inconsistent embedded CIA metadata.
