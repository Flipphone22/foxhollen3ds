# Console images on clean exit

11 October: normal `5dd6a33aec44` / CIA `bd33b0dc9555` and replay
`77bc75172b99` / CIA `cb0f8c69b934` built with shared native sources.
Deployment status is recorded separately in IMPLEMENTATION.md.

Pressing Select for a clean exit now records
`SD:/3ds/foxhollow/last-frame-top.ppm`. It is a private 400x240 RGB image for
diagnosing the reported white Arwing, disappearing trees and other visual
defects on hardware. This does not itself repair those defects.

The screenshot is taken after the renderer has retired its GPU/transfer work,
while GSP and the LCD buffers still exist. The code invalidates the GPU-written
LCD range, then reads its portrait ABGR pixels on the CPU into a temporary
288,000-byte RGB buffer before filesystem IPC. This follows pinned Melee's
`native-3ds/port/3ds/game.c:capture_frame` discipline. Pinned citro3d
`C3D_Fini`/`C3Di_RenderQueueExit` supplies the existing completed-queue boundary;
no new queue, worker, GPU copy, raw depth readback or recurring screenshot task
was introduced. No framebuffer flush/clean overwrites the GPU's result.

The public libctru API exposes the current drawing buffer. With double buffering,
this can contain the frame preceding the visible scanout; it is not claimed to
be an exact scanout or crash capture. The SDK's 240x400 physical dimensions and
RGBA8 format are checked before access. A missing buffer, allocation failure,
unsupported format, cache failure or file error logs a nonfatal failure.
Incomplete output is removed. The prior image is replaced on the next successful
capture; saves and game assets are untouched.

There is no screenshot allocation, pixel read or SD write during gameplay.
The request flag is set only by the existing Select exit paths. Automatic replay
completion does not request a screenshot. Shutdown can take longer to write this
image; no physical SD-duration improvement is claimed. Scratch allocation occurs
after the final ordinary memory report and is therefore reported separately,
not hidden inside the preceding sampled heap/stack peaks.

The PPM header and successful native.log entry share the retrace and clock stamp.
`tools/collect_visual_console.py` collects this one current file with a strict
288,256-byte cap, validates its dimensions/payload, checks the matching log stamp,
and creates a local PNG. A stale image is explicitly marked as not matching the
collected log. The collector does not scan old image directories or read assets.

Actual-source ASan/UBSan tests cover all96,000 pixel positions and RGB channels,
orientation, no-request no-op, full CPU materialization before first filesystem
operation, invalidation failure, unsupported layout, open/write/close failures,
undersized destinations and partial-output removal. Missing invalidation fails
the regression. Four malformed image variants and an old-log stamp are rejected
or identified. This is correctness evidence, not GPU performance simulation.

The exact normal CIA completed serial New and Old headless checks in32.18 and
32.07 host seconds. Both reached the cockpit through ordinary Start input,
exited with Select, drained logs and exited the owned emulator. Both saved
288,083-byte images with matching log stamps. Both images were inspected:
Fox/cockpit/title/menu are present, correctly oriented and colored. No special
guest test hooks, desktop input automation or visible window was used.

| Sampled game resource | New | Old |
|---|---:|---:|
| Heap peak / reserved, B |46,277,472 /71,303,168|38,740,832 /50,331,648|
| Minimum free linear / reserved, B |18,518,656 /25,165,824|3,838,592 /10,485,760|
| Minimum free VRAM, B |4,822,016|4,822,016|
| Process commitment / limit, B |103,190,528 /130,023,424|67,538,944 /83,886,080|
| Measured main stack / reserved, B |72,956 /524,288|72,956 /524,288|

The288,000-byte shutdown allocation is additional to these sampled game metrics,
after renderer resources are released. It remains well within the measured
normal-heap headroom in these scenes. No universal area/memory guarantee follows.

343 native sources, the game archive and exact main511/fur214/material304 shader
programs match between normal/replay. CIA profiles remain Old80/New124. The prior
active-light optimization is included, with LIGHT MAPS still opt-in and its CPU
budget still unmet. There is no new shader/effect or physical FPS claim here.
No redundant replay boot was run for a capture that automatic replay does not
request; its unchanged renderer reuses the previous serial replay evidence.

Evidence: `artifacts/research/pica-scene-rewrite/last-frame-test/` contains host,
runtime and shared-build proofs. `artifacts/native/last-frame-normal-v2-{new,old}`
contains captures and isolated-run summaries. Builders preserve source snapshots
and restore all six unrelated VRAM work files and the previous archive in finally.
The first local build lacked the capture/log clock stamp and was superseded before
runtime testing or deployment. It was not a console failure.
