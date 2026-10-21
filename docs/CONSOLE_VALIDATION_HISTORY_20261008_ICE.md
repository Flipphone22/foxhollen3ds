# Console validation: current development build

For New 3DS build `749e95982cfb` and the complete-bank scene package. Old build
`99f8291928a90` is built separately; it does not enable the New-only RAM caches.
This is a development build, not a validated 30/60 FPS release.

## What this build changes

- Restores original live A-button/enemy-health and C-menu material decisions,
  including animated images, opacity masks and dimming.
- Fixes projected pause holograms and the opaque blue communicator wedges with
  native PICA texture coordinates, transparency and additive blending.
- Corrects ordinary constant-opacity materials that incorrectly inherited vertex
  or lighting alpha. Preceding shadow/projected-alpha chains remain separate.
- Repairs simplified geometry that deleted small features or damaged sparse
  facial morphs; Peppy's face and glasses are restored in captured menu tests.
- Retains the projectile alignment-crash fix, save-card cache and complete-bank
  assets from the previous build. Existing console saves are preserved.

This is a visual-correctness development build. It does not establish 30/60 FPS,
complete rendering correctness, or absence of further crashes. Restored topology
adds geometry: an earlier matched menu comparison used about 16.5% more indices.
Physical cost still needs measurement; this is not a claim of an FPS improvement.
Lighting, shadows, some faces/eyes and material effects remain under investigation.

The latest collected physical run (20261007-203638) ended cleanly, with all221698
GPU submissions completed. Its mixed-scene cadence after frame300 averaged23.78
FPS; last600 frames20.78FPS. Three intervals exceeded1second (maximum1.62s).
The SD executable was8093; logs have no independent per-run build identifier.
These are not matching-scene results for this new build.

## Next hardware check

After installation is confirmed, exit ftpd and launch Foxhollow from Homebrew
Launcher normally. No reboot is needed unless recovering from a console hang.
Load your existing save and play normally. Check the item menu, pause hologram,
interaction prompt and nearby enemy-health display. Note missing geometry or
multi-second pauses. Startup/initial preloading may take longer. Press Select to
exit, allow buffered logs to drain, then reopen ftpd so the run can be collected.

Your saves remain in `SD:/3ds/foxhollow/saves/`. No emulator save or synthetic
rendering fixture is installed. The native port uses GCI saves with GSAE/01
identity even with PAL assets.

## Build and installation

Run from the configured workspace:

```powershell
wsl -d Ubuntu -u root --exec sh <repo>/tools/linux_exec.sh python3 /work/tools/build_native_matrix.py
wsl -d Ubuntu -u root --exec sh <repo>/tools/linux_exec.sh python3 /work/tools/package_console.py --native-reconstruction --direct-efb --reflection-lod
```

Packages under `build/console-validation/` contain the executable, runtime
settings, build records and instructions, without game data or firmware. The
app path is `SD:/3ds/foxhollow/foxhollow.3dsx`. Keep normal DSP setup at
`SD:/3ds/dspfirm.cdc`.

Private files beside the app:

| SD filename | Current source / identity prefix |
| --- | --- |
| `disc.iso` | Verified PAL revision 1 private disc, SHA256 `f5e5a9a29147` |
| `native-scene.bin` | Complete-bank scene, `bfb5e2626f9f` |
| `native-scene.cache` | Matching optional New window cache, `d83cf39ce262` |
| `native-scene-coverage.json` | All-bank inventory bound to package and manifest |
| `model-lod.bin` | Feature/morph-preserving topology, `be5685634a87` |
| `texture-pack.bin` | Surface/first-Fox directory pack, `760f3b09cceb` |
| `starfox-pica.thp` | Prepared PAL movie, `656ad2a7f8b3` |

Keep associated preparation manifests and preserve the original RVZ. Use the
verified installer for staged full readback and backups. The original disc is
still read for game assets; rendering uses separate prepared geometry/textures,
and the movie uses the prepared THP. See `DISC_BASELINE.md`, `PREPARED_MOVIE.md`,
`MODEL_LOD.md`, `PREPARED_TEXTURES.md` and `PREPARED_SCENE_CACHE.md`.

## Controls

| Input | Action |
| --- | --- |
| Circle Pad | Left stick: movement/aiming |
| New 3DS C-stick | GameCube C-stick: items menu |
| A, B, X, Y | Matching GameCube button |
| L, R | Matching shoulder button, full analog trigger |
| ZL or ZR | GameCube Z |
| Start | Start/pause |
| Select | Exit the port after queued work completes |
| New 3DS D-pad | GameCube D-pad |
| Old 3DS D-pad | C-stick directions for items |
| Old: hold touchscreen + D-pad | GameCube D-pad, including world map |

Button actions remain game-dependent. Rumble is unavailable.

## Evidence and budgets

Headless normal-save checks completed with clean Select shutdown. The store
and Ice Mountain visual pairs use separate synthetic local starts; they establish
rendering of missing assets, not normal travel, interactions or progression.

The normal-load and corrected Ice Mountain checks peaked at 56,465,176 /
71,303,168 bytes normal RAM including reserved main stack (20.81% free), with
at least 6,619,904 / 25,165,824 linear bytes free. Measured main stack was
26,104 / 524,288 bytes; initialized VRAM free stayed above 4,813,824 bytes.
These are sampled emulator measurements. Runtime guards
retain 15% normal and 4 MiB linear reserves. Optional caches yield to live assets.
Both checks completed Select shutdown with drained logs and no renderer rejection.

Normal loading used zero runtime prepared-scene reads; the Ice Mountain fixture
used 27 reads / 655,740 compressed bytes. Prepared textures used zero runtime
SD reads, and neither cache was pressure-released. Physical SD latency, CPU/GPU performance, cache behavior
and long-run memory peaks still need console validation. No emulator timing
multiplier is used. Target remains 60 rendered FPS, with steady 30 minimum.

Preserve each run's `native.log`, `game.log`, `memory.jsonl`, `minimal-render.csv`,
`render.csv`, `sd-io.csv`, `audio.csv`, `log-io-summary.json` and failure captures.
They are overwritten by later runs. For the native renderer, calculate FPS from
`minimal-render.csv` present intervals; submit time alone is not frame time.
`Native scene IO` and `Native texture cache IO` separate prepared-cache activity
from runtime SD fallback. GPU queue-tail timing is not total GPU busy time.

Full implementation evidence and unresolved work are in `IMPLEMENTATION.md`.
Earlier console notes are retained in the repository's
`CONSOLE_VALIDATION_HISTORY_20261007.md`.
