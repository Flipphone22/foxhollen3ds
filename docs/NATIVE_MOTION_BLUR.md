# Native temporal motion blur

14 October current local owner uses RGBA8 history (512KiB rather than the
256KiB RGB565 implementation described in the original record below). Depth
blur shares its alpha, removing a separate capture draw and two target switches.
Motion-only also reserves512KiB so toggles never replace a live history image.
Combined depth-filter scratch adds512KiB, with1MiB VRAM headroom still required
at allocation. See NATIVE_POST_DEPTH.md for actual pixel/resource tests and
dialogue comparisons. This change is local, not deployed; physical performance
and original colour-quantization acceptance remain open.

13 October: optional implementation, local only. This does not implement the
separate blur filter or heat haze. Full visual parity and physical performance
remain unvalidated.

`MOTION BLUR` is on graphics page 12, OFF in all four presets. OFF makes no
history captures or blur draws and allocates no history on a fresh run. After
use, OFF invalidates the history while keeping bounded storage until GPU-retired
shutdown. Recording locks the control and settings persist it normally.

## Original semantics and native implementation

The `main/lightmap.c` scene boundary requests blur when camera shake or the
original motion flag is active, using `gMotionBlurAmount`. It blends the previous
capture, then updates history only when `getHudHiddenFrameCount()==0`. Deferred
objects, later effects and HUD retain their original ordering. Simulation,
progression, original effect state and saves are unchanged. Only `lightmap.o`
changes in the game archive.

`track/intersect_render.c::renderMotionBlur` uses RGB from the old reflection
texture, a truncating 8-bit constant alpha and source-alpha blending. Native
PICA draws implement that operation with the existing UI shader. A separate
GPU draw captures colour; there is no CPU pixel conversion, raw depth readback,
per-frame vertex construction, additional worker or additional shader program.

Persistent storage is one 512x256 RGB565 VRAM history (262144 bytes), two quads
in 512 linear bytes, two prepared material/buffer descriptions and small counters.
The original captures 320x240 from 640x480; this port resamples its native
400x240 image. Thus temporal ordering and blending are preserved, but the
source resolution/filter is not claimed pixel-exact. The first enabled frame
seeds history instead of reading uninitialized pixels.

CPU work is the original predicate/alpha, bounded resource admission at first
use, command publication and accounting. GPU transform/sample/blend performs
the image work. Initial added rendering CPU gates are 0.20 ms New / 0.50 ms Old;
console GPU cost and the full 16.67/50 ms frame targets remain separate gates.

## Hardware integration references

Consulted pinned Melee `native-3ds/port/3ds/renderer.c::gpu_efb_copy` and
`citro3d_fix.c`: explicit framebuffer ownership, retained sampler state and
invalidation after auxiliary rendering. Also checked pinned Citro3D
`renderqueue.c` split/flush behaviour. This port departs from Melee's intermediate
linear EFB copy: it samples its existing VRAM framebuffer texture directly,
with ordered command-list splits before target transitions. The shared auxiliary
owner joins prepared work, reserves commands, restores the main target and
invalidates scene/material/fog state. All GPU readers retire before release.
No raw depth-readback path is restored.

## Evidence and limits

- `motion-blur-owner-test/result.json`: actual C owner, ASan/UBSan; 35600
  temporal/OFF cases, 100 lifecycles and 65537 alpha values. Missing publication
  and missing OFF guard mutants fail. These mocks alone do not prove GPU output.
- `artifacts/native-motion-blur-device/v6/result.json`: eight actual PICA pixel
  cases in silent Azahar. Distinct quadrant images test orientation, fractional
  blending, blend-before-capture, HUD-hidden history retention and OFF/re-enable.
  Maximum RGB error is 7/255, within RGB565 quantization. Images inspected.
- `artifacts/native-motion-blur-device/linked-v5/result.json`: repeats those
  eight cases using entry 1 of the exact `minimal_shared_shader` extracted from
  the game ELF; same results, owned process exited.
- `motion-blur-source-test/result.json`: 6939 original predicate/order cases;
  adapter leaves surrounding source unchanged. Menu taps, recording locks,
  four preset defaults, settings persistence and idle no-flush tests pass.
- New exact-binary ON/OFF game route: 57.1357 / 57.1404 Azahar FPS; rendering
  CPU +0.12229 ms. ON makes 1617 captures; OFF makes zero with zero history
  allocation. **This route requests zero blur draws**: it validates game capture
  integration and ordinary rendering, not the visible gameplay effect. Different
  pose/workload phases preclude a precise speedup claim.
- New ON normal heap 49210768/71303168; minimum linear free 16752896/25165824;
  VRAM free 4461568; measured stack 73068/524288; process commitment
  103337984/130023424 bytes. Capture cost is within the initial local CPU gate;
  this does not establish physical-console throughput.

Failures retained: first game build needed auxiliary function declarations;
initial device images exposed flipped display coordinates/UV. Subsequent
checks were obscured by Azahar's `Update Available` X11 window (exact 320x100
geometry established read-only). Isolated profiles now disable update checks;
the same v3/v6 fixture binary then passes. A UI test exposed reuse of the
original-textures sentinel as a real effect bit. Menu actions now have a separate
kind field, so adding effect bits cannot toggle original assets accidentally.
The first explicit-kind ARM build rejected missing initializers; corrected
without weakening warnings. No failed build was deployed.

Shared normal v6 (build ID11b2c703d94d, 3DSX4ccf98aafa10/CIA b2e781ecafb65)
and replay v5 (3DSX78705700e908/CIA a39caa45127e) are built. Normal v6 was
uploaded/readback-verified to New223 at the user's request; replay remains local.
`console-test/20261013-104820-new223/transfer.json` records completion.
Build verification
checks exact renderer sources, all unchanged linked shaders, matching game
archive64298a7423a1, Old80/New124 grants in CIA headers and unchanged conversion
recipe. The New ON/OFF measurements used v3: v5 changes only the menu's action
namespace. The ISO-setup adapter retains its separate RGB8 movie support;
generated lightmap translation units match before sharing the updated archive.

At the user's request, deployment proceeds without further slow runtime or
conversion checks. Old-profile cost, exact-normal startup/toggle execution,
naturally requested blur in a game scene, original-image comparison, full
effect interactions and physical target transitions/cost remain unvalidated.
The optional implementation does not close those items or the overall goal.
