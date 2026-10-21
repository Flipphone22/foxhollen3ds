# Startup-menu benchmarks

Current local candidate `0d6f83a3f5d5` uses DSP audio by default for Play and
Benchmarks on both models; CPU is a startup comparison choice. It includes
collision-query reuse, fused raster snapshots and prepared native program
configuration plus native DSP-rate common reverb with software sends retained.
Old palace validation passed; physical FPS remains unmeasured.
This candidate is not deployed. See NATIVE_PROGRAM_PLANS.md and ISO_SETUP.md.

## Deployed Old build: resident renderer integrated

Normal build **45631515f007** in `unified-resident-v2-console-setup-builds`
supersedes the menu-only candidate described below. It integrates the resident
world/actor command renderer and sparse offsets, retaining the deployed normal
four-stage conversion recipe and established CPU audio. Experimental DSP work
is excluded. Both formats use normal title `000400000F484C00`, Old80/New124.
The source composition is reconstructed exactly in `acceptance.json`; all 18
GPU programs and the game archive match the resident parent. Root WIP restored.

Old startup palace benchmark passes (307 rows, status1); normal saves/settings
are unchanged. New ordinary Play loads the cave and exits through Select. Both
captures were inspected and both owned processes exited with logs drained and
no observed layout faults. Diagnostic timings are excluded from performance
claims. Sampled normal-after-stack/linear/process headroom is 16.33/38.98/15.24%
on Old and 23.00/71.86/19.90% on New. Physical speedup remains unmeasured.

Package `build/console-validation/foxhollow-iso-setup-a8490a3cae2a32db.zip`
is now **deployed to Old229**, transfer
`artifacts/console-test/20261016-002439-old229/transfer.json` COMPLETE.
All six normal app payloads passed staged full-byte comparison and final sizes.
Earlier data-connection resets recovered after the user restarted ftpd; five
later metadata-query resets recovered within the transfer. Assets, saves and
settings were untouched; New223 was not updated.

Install `SD:/3ds/foxhollow/foxhollow.cia` with FBI to update the existing HOME
icon. Choose **Benchmarks / playback**, then **00-resident-palace** and press A;
use PICA ACTORS+HUD, MAIN, VRAM OFF, profiling OFF. It exits automatically;
reopen ftpd afterward. Existing case fixtures were already verified on SD.
No separate replay CIA is needed. Hardware speedup is not yet established.

## Historical menu-only integration

The user rejected separate replay launchers on 16 October. The earlier
`resident-hardware-v1` Old229 upload is historical evidence, not an installation
recommendation. Do not install its separate CIA. Normal console installations
remain source `2fba94b84350` until a verified replacement is explicitly recorded.

The local `unified-startup-v3-console-setup-builds` candidate adds
**Benchmarks / playback** to the existing Foxhollow startup menu. Play and
benchmarks share the normal title `000400000F484C00`, ELF, renderer and committed
asset installation. This candidate starts from the deployed normal archive;
it **does not yet integrate the resident actor-stream performance candidate**.
It is not an FPS improvement or a reason to request another console benchmark.

`tools/unified_startup_bench_patch.py` reconstructs the changes from the archived
normal sources. `tools/build_unified_startup.py` builds in isolation and restores
root WIP and the game archive. The four-stage conversion recipe, linked GPU
programs, game archive and Old80/New124 CIA profiles remain unchanged.
Never build the unrelated root renderer WIP blindly.

Mode selection now happens before game initialization. Normal play retains
recording, graphics controls/settings persistence, the normal save directory and
live GPU telemetry. A benchmark enables the existing ordered input player,
allocates its bounded telemetry rows and selects a private copied save. Its
graphics overrides are not saved to the user's configuration. The historical
replay-assets marker cannot redirect either mode to a second asset installation.
Benchmark cancellation returns to the startup menu. Failed case preparation
cannot publish its temporary save path to a subsequent normal game launch.

The SDK console handoff required an additional repair: the pinned libctru
`consoleInit` installs its console devoptab only on its first invocation. The
startup owner restores prior stdio on retirement, so the next menu previously
rendered no text. The unified owner retains and rebinds the immutable SDK
devoptab when entering another menu. No framebuffer, GPU resource, or
stack-owned `PrintConsole` is retained across owners. The existing pinned
Melee-derived LCD lifecycle remains in place. Pinned Melee `card_storage.c`
profile isolation informed keeping replay saves separate inside one application.

Evidence:

- `unified-startup-test/result.json`: actual portable menu/runner and boot owner
  under ASan/UBSan; readiness changes, benchmark selection, cancellation/back,
  exit, settings writes and installation lock ordering.
- `unified-startup-test/consoleInit.txt`: pinned SDK disassembly establishing
  the one-time stdio initialization; this was not a speculative screen fix.
- `unified-startup-v3-console-setup-builds/scope.json` and `restored.json`:
  isolated shared build, unchanged shaders/recipe/game archive, root restored.
- `artifacts/native/unified-startup-v3-benchmark-old/summary.json`: actual CIA
  content, startup → benchmark picker → palace, status1/step10/307 rows,
  normal saves/settings unchanged, complete log drain and owned-process cleanup.
  Startup, picker and palace captures inspected. No layout faults in 106496
  observed native draws. No physical performance claim.
- `artifacts/native/unified-startup-v3-play-new/summary.json`: the same ELF's
  3DSX takes ordinary Play, loads the cave save, renders Fox/world/HUD and the
  normal recording control, and exits on Select with complete log drain.
  Loaded-cave capture inspected; no native layout faults in167936 observed draws.
- `unified-startup-v3-console-setup-builds/acceptance.json`: exact source/binary
  identities, unchanged recipe/title, host-test hashes and both runtime records.

Old-mode emulator samples: normal heap peak41384928/50331648 bytes,
linear minimum free2807040/10485760, initialized VRAM free4723712,
main stack upper bound72984/524288, process68931584/83886080.
These are sampled emulator budgets, not physical measurements or endurance
coverage. The benchmark log allocates1835008 bytes only when starting a case.

Preserved failure: v2 completed the replay, but its picker capture was blank;
the harness also read the result JSON before its final flush. The v2 summary
remains failed. v3 repairs console stdio ownership, and the harness waits for a
fully parseable result before validating it. It does not weaken completion,
source identity, save, drain or scene assertions.

The standalone `install_console_test_apps.py` entry point now exits before any
network or filesystem mutation. Historical local replay binaries remain useful
for regression comparisons; future deployments must use the existing normal
application. The shared startup candidate is not deployed. Merge and evaluate
the resident renderer improvements in this application before presenting it as
the requested performance test build.
