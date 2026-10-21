# Fur comparison at native resolution

11 October: the wind candidate remains local and unaccepted. No console files,
assets, settings or saves were changed during this comparison.

`build_desktop_native_reference.py` compiles an isolated desktop entrypoint and
links the original game archive and renderer libraries unchanged. It sets an
800x480 initial window, stretch presentation and the same 5:3 initial/standard
camera aspect as the native port. No simulation or material code is changed.
The original executable, objects and vendor source are retained byte-for-byte.
The reference executable is `986526ae361f`; its source/command/archive record
is `artifacts/research/pica-scene-rewrite/desktop-native-reference-v2-build`.

The runner resizes only this process's window on its private Xvfb server to
400x240. A read-only GDB observation verifies both presentation and actual GX
framebuffer dimensions as 400x240; another records camera and standard aspect
as 1.666667. This matters because upstream render scale is relative to the
configured GameCube framebuffer, not the desktop window. The output is captured
directly at 400x240, with no screenshot downsampling or desktop input automation.

The isolated copied save has the same starting hash as the native ON/OFF runs.
Normal Start input reaches the cockpit; subsequent menu input is suppressed by
the runner's `--cockpit-only` mode. Game state itself is not modified. The final
reference run `artifacts/desktop/fur-native-reference-v4` completed in 37.32 host
seconds with title ready, captures 2/3/final, and owned-process cleanup. It is
visual evidence, not a performance benchmark or progression validation.

The earlier v3 reference verified 400x240 but retained the original 4:3 camera;
that image was not accepted as a matched projection comparison. v1 failed while
allocating the old 1GB input runway before launching a process. v2 exited cleanly
after a missing `xwininfo` dependency; the runner now uses libX11 directly. Its
optional smaller input capacity avoids a 1GB allocation for a short reference;
the original default capacity and growth limit are unchanged. Completed input
files were NTFS-compressed with before/after SHA checks, retaining all evidence.

`fur-native-image-comparison/cockpit-comparison.png` presents the original and
native wind OFF/ON at their unscaled 400x240 output. Native images are crops of
the existing Azahar captures, not new runs. The original still has clear fur at
this resolution. The wind candidate alters native fur pixels, but the comparison
does **not demonstrate that it repairs the user's reported appearance**. Its
327-instruction shader should not be accepted or deployed merely because it
changes pixels or has small CPU submission cost.

Source and native animation/movie/noise phases are not synchronized. The report's
ON/OFF head/arm/tail pixel differences also include those possible differences;
they are not a fidelity score or isolated causal measurement. No GPU-cost claim
can be inferred from this reference run. Existing candidate GPU interpolation
limitations and missing projected lighting remain explicit.

Next implementation work must address those source material differences with
bounded native resources. In particular, the current masked fur already uses
three samplers for noise, base colour and coverage. Adding a projected light or
source diffuse map needs a deliberate material/resource design, not reconnecting
the original GX path or silently dropping mask/fog/lighting stages. Full fur
acceptance, rendering parity and sustained hardware performance remain open.
