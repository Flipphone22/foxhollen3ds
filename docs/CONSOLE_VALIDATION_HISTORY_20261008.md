# Console validation: current development build

For New 3DS build `8093a41bc86c` and the complete-bank scene package. Old build
`90310a70eb17` is built separately; it does not enable the New-only RAM caches.
This is a development build, not a validated 30/60 FPS release.

## What this build changes

- Fixes the physical poison-projectile crash: effect 0x714 now reads the
  producer's one-byte alpha with the correct type. The old four-byte read
  caused an ARM alignment exception. The effect and its fade remain enabled.
- Retains virtual-card inventory across mounts within 128 KiB, removing repeated
  save-file reads. Save writes still complete synchronously and preserve errors.

- Replaces manually selected maps with coverage of all 53 discovered asset banks.
  A same-binary comparison restores Ice Mountain walls and ground that were blank.
  Installation rejects missing bank/asset identities and stale coverage reports.
- Keeps the whole-game prepared geometry cache within 7 MiB normal RAM. Blocks
  outside the resident window are read on demand; all are verified at startup.
- Includes conservative texture-write indexing, preserving exact comparisons,
  ownership checks and the earlier normal-transform crash correction.

The latest collected physical build was `7c4441134b50`: mixed gameplay frames
300 through 21850 averaged 22.78 FPS; the last recorded 600 frames averaged
17.31 FPS. Four intervals exceeded one second. It crashed in the reported
Snowfall Mountain area. The newest OS dump matches its instructions and identifies
the corrected alpha read. Buffered logs end before the exact crash frame.
This candidate has no physical performance or stability result yet. Known lighting,
texture/effect and broader compatibility issues remain. Original-disc streaming
and game loading can still stall; neither cache establishes a complete fix.

## Next hardware check

After installation is confirmed, exit ftpd and launch Foxhollow from Homebrew
Launcher normally. No reboot is needed unless recovering from a console hang.
Load your existing save and continue normally through the mountain area where
the crash occurred. Note multi-second pauses separately from
low sustained FPS. Startup and initial loading may take longer with preloading.
Press Select to exit, allow buffered logs to drain, then reopen ftpd.

Your console saves remain in `SD:/3ds/foxhollow/saves/`. Never install the local
synthetic rendering fixtures: they change the starting position solely for
rendering tests and are not progression evidence. A raw memory-card image is not
accepted; the native port uses GCI saves with GSAE/01 identity even with PAL assets.

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
| `native-scene.bin` | Complete-bank scene, `36165281248f` |
| `native-scene.cache` | Matching optional New window cache, `4372ffd07760` |
| `native-scene-coverage.json` | All-bank inventory bound to package and manifest |
| `model-lod.bin` | Alpha-coverage topology, `82412755d509` |
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
