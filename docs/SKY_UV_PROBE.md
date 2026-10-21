# Cloud shader UV repair check

The completed first probe reproduces the console failure: the full shader
produces a uniform colour, while the same geometry with interpreter outputs
shows the clouds. Culling, depth, uniform clip scaling and RGBA8 storage do not
restore detail. Both texture rectangle controls work, including large clip W.
Evidence: `artifacts/console-sky-probe/results-20261012-010103-223/`.

This second probe tests candidate arithmetic changes on that exact cloud draw.
It is not a normal-game release or performance benchmark. Launch
`SD:/3ds/foxhollow-sky-uv/foxhollow-sky-uv.3dsx` through Homebrew Launcher;
it runs automatically, captures nine cases and exits. Reopen ftpd afterward.
No game assets, saves or preferences are changed.

The launcher and result folder now have the distinct name `foxhollow-sky-uv`.
The bottom screen identifies `SKY UV v2`, build `6e9d1627a598ed11`.
This addresses ambiguity after repeated collections returned the first probe's
unchanged log; the reason those results remained unchanged is not established.
Failure to open the result log is now displayed instead of silently exiting.
The GPU cases are unchanged: their fixture bytes and all nine emulator capture
hashes match the preceding UV probe. The new binary passed the bounded check in
3.38 seconds and exited normally. This is not additional hardware evidence.
Collect with `collect_sky_gpu_probe.py --host new223 --app foxhollow-sky-uv`.

Cases: original shader; direct ordinary UV arithmetic; specular UV write
removed; fur UV write removed; precomputed-output control; UV initialization
moved beside its consumers; specular/light7 boolean locations swapped;
standalone original shader; original shader again after all handoffs.
The disabled-write cases are diagnostics for this ordinary cloud fixture;
they must not replace real fur or specular material handling in gameplay.

All nine cases complete in bounded headless Azahar and their captured images
were inspected. This does not prove that the hardware fault is repaired.
The late-initialization variant is standalone because adding its world-path
initialization exceeds the existing shared program's strict instruction limit;
the limit was retained. Existing capture size/cache/retirement protections and
the linked ARM null-sampler regression remain in place.

## Physical UV result, 12 October 10:52 UTC

The New 3DS completed the correct `SKY UV v2` build
`6e9d1627a598ed11`. Read-only collection
`artifacts/console-sky-probe/results-20261012-105216-223` contains its log,
build record, all nine original captures and the inspected contact sheet.
`verify_physical_sky_uv.py` validates their identities and pixel comparisons.

| Case | Physical result |
|---|---|
| Original, repeated original | Identical flat colour |
| Direct ordinary UV arithmetic | Detailed clouds |
| Specular UV write removed | Identical to direct UV |
| Fur UV write removed | Identical flat failure |
| Precomputed-output reference | Detailed clouds |
| UV initialized near consumers | Identical flat failure |
| Specular/light7 boolean slots swapped | Identical to direct UV |
| Original shader compiled alone | Identical flat failure |

The three successful variants differ from the reference by at most two byte
levels per channel; mean absolute error is 0.004302083 byte, with 819 of 96,000
pixels differing. This supports direct UVs for ordinary cloud materials and
implicates the original specular UV branch/control. It does **not** establish a
universal b15 defect, prove why the branch fails, or justify moving another live
feature into b15. Pinned citro3d `source/uniforms.c` publishes the full 16-bit
boolean mask with `0x7FFF0000`; no SDK publication defect has been established.

The accepted local gameplay candidate already contains a separate direct-UV
cloud shader. This physical fixture validates the repair mechanism, not that
whole application's handoffs, full intro or performance. No further sky probe
is requested for this fault. No executable, asset, preference or save changed
on the console during collection. No new normal build was deployed this turn.
