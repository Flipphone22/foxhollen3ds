# Cloud GPU isolation

This is a diagnostic, not the pending sky fix or a new gameplay build.
The regular applications, graphics preferences and saves are untouched.

On New 3DS, launch `SD:/3ds/foxhollow-sky-probe/foxhollow-sky-probe.3dsx`
through Homebrew Launcher. It runs nine cases without buttons and exits
automatically. Reopen ftpd afterward. The top display need not show the cases;
the captured images are written to the probe's own directory. If a timeout
message explicitly says resources are retained, reboot before reopening ftpd.

The fixture is original cloud model 1196, texture 0x88d, and the actual linked
511-instruction scene shader, with captured live inputs from intro frame 5200.
This is a nearby camera angle, not the console's exact animation/weather state.
No simulation, game asset modifications or save access occur.

| Case | Single changed condition |
| --- | --- |
| 0 | Captured cloud baseline |
| 1 | Disable culling |
| 2 | Disable depth test |
| 3 | Opaque constant white material |
| 4 | Use captured interpreter outputs through a minimal passthrough shader |
| 5 | Scale all four projection rows by 1/1024, preserving projected coordinates |
| 6 | Sample the same texture on a rectangle with W=1 |
| 7 | Same rectangle with all clip components multiplied by 4096 |
| 8 | Same cloud using equivalent RGBA8 texture storage instead of L8 |

The SDK NULL-sampler regression checks the actual linked ARM binder with page
zero unmapped. Texture conversion checks all 16,384 source pixels independently.
All nine images are read only after GPU retirement, using the already verified
400x240 diagnostic transfer. GPU timeout retains resources; it never frees
in-flight allocations. This follows the pinned Melee binding/cache discipline
and the existing verified C-menu probe's submission and lifetime handling.

Headless Azahar run `sky-gpu-isolation-v1-new` completed in 3.45 host seconds.
All nine images were inspected. Baseline, no-culling, scaled-clip and RGBA8
images match exactly. Depth-off and passthrough differ by at most 2 byte levels
in 2/4 pixels. Both rectangle scales match exactly. This confirms the isolation
is coherent in Azahar, not that the physical-console failure is fixed.

The draw and texture are private game-derived fixtures. Do not publish the
probe binary, fixture header or image payloads as public source artifacts.
