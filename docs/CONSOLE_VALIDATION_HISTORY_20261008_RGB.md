# Console validation: Ice Mountain live terrain

New 3DS build `e3d6ccc3872f`; separate experimental Old 3DS build `75d14241c7fd`.
This is a development build, not a validated steady 30/60 FPS release.

## Changes and limits

Ice Mountain can move world vertices through game objects such as XYZAnimator.
The previous backend assumed world positions were static and rejected the whole
changed block. Version8 geometry records retain original position indices, so
the native renderer can refresh moved terrain and its culling bounds after a
GPU ownership fence. Unchanged blocks retain their GPU buffers. The recorded
Ice Mountain save required one 15,522-byte publication, not a per-frame rebuild.

Unprepared CMPR textures now use RGB5A1 instead of RGBA8: half the storage,
exact binary transparency and maximum RGB channel error 4/255. This is the
authorized reduction in colour precision; no texels or geometry are removed.
Prepared ETC textures, original assets, movie data and saves are unchanged.
Previous HUD, transparency, model-feature and crash fixes remain included.

The normal-save headless replay restores the missing slopes and walls. Black
trees and some object textures are still visibly incorrect. This does not fix
all lighting, shadow, face or material defects, or establish a physical FPS gain.
The original scene rejection checks remain for unsupported changes to topology,
UVs or colours; they are not disabled to make the terrain appear.

## Hardware check

After installation is confirmed, exit ftpd and launch Foxhollow normally through
Homebrew Launcher. No reboot is needed. Load the existing Ice Mountain save and
check the previously empty terrain while moving around. Press Select to exit,
allow buffered logs to drain, then reopen ftpd. Saves in
`SD:/3ds/foxhollow/saves/` are preserved; no emulator save is installed.

## Build and private files

The four-target build is reproduced by `tools/build_native_matrix.py` in the
configured Linux SDK environment after rebuilding the game archive with the
current overrides. `tools/package_console.py --native-reconstruction
--direct-efb --reflection-lod` creates the New package; add `--old-3ds` for Old.
Packages contain code/settings/instructions, not game assets or firmware.

App: `SD:/3ds/foxhollow/foxhollow.3dsx`; DSP firmware: `SD:/3ds/dspfirm.cdc`.

| Private file beside the app | SHA256 prefix |
| --- | --- |
| `disc.iso` (PAL revision 1) | `f5e5a9a29147` |
| `native-scene.bin` (version8 world records) | `1a24b85a9c1f` |
| `native-scene.cache` (matching New 7 MiB window) | `97e70004e545` |
| `model-lod.bin` | `be5685634a87` |
| `texture-pack.bin` | `760f3b09cceb` |
| `starfox-pica.thp` | `656ad2a7f8b3` |

Deploy the executable, scene, coverage/manifest and matching cache together
using the verified installer. Preserve the original RVZ. The native port still
reads original assets from the private ISO; native geometry, textures and movie
also use their prepared companions. See `PREPARED_SCENE_CACHE.md` and
`artifacts/research/texture-pressure/integration.json` for exact records.

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

## Validation evidence

`ice-world-admission` reproduced the empty terrain from the user's current
GCI save. `ice-live-world` follows the same ordinary load and input sequence:
world block295's 3,196 position rejections disappear, terrain is visibly restored,
and Select shutdown completes with11,049 balanced GPU submissions, zero errors.
The two existing compiled material omissions remain explicit.

Four target builds pass. Host tests cover1,267 world packets/804,317 vertices,
2,534 invalid-input checks and actual publication ordering. UVs, colours and
topology remain byte-identical; only positions change. Every2,102 asset packet
passes the C reader and46,356 stream checks. All53 banks retain complete asset
identity coverage. CMPR checks cover786,432 palette cases plus1,310,976 padded/
unaligned pixels; alpha is exact. New and Old storage/ownership checks pass.

New headless runs peak at56,034,272/71,303,168 normal bytes including reserved
stack; minimum linear free7,399,808/25,165,824; initialized VRAM free4,817,920;
main stack at most26,160/524,288. Existing15% normal and4 MiB linear guards remain.
No additional per-frame vertex work is performed for unchanged world blocks.
Prepared texture runtime SD reads remain zero. The Ice Mountain replay uses
18 prepared-scene reads/445,713 compressed bytes, without cache pressure release.

The matched older-save texture comparison retains essentially the same draw/
index workload, eliminates45 budget evictions and saves1,778,688 upload bytes.
This is memory/work evidence, not a demonstrated CPU or hardware FPS gain.
The console run collected before this fix averaged24.50FPS after frame300 and
22.16FPS over the last600 frames, but had missing terrain and mixed user movement;
it is not a matching-scene performance comparison. Target remains60 rendered FPS,
with steady30 minimum. Physical acceptance, remaining rendering defects, stalls
and full progression/stability validation remain incomplete.
