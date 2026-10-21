# C-menu GPU isolation probe

**Latest result:** V7 completed on hardware (`20261011-091330-results`). Case 6,
direct ordinary UV arithmetic, restores the full panel and is byte-identical to
the minimal skinning control. Standalone shader and removal of Q output do not
repair it. A normal game candidate now uses a small native shader for unlit UI
models; see [implementation and evidence](NATIVE_UI_MODEL_SHADER.md). The next
check is the normal game, not another probe run.

Latest installed candidate: UV-isolation probe `1e6dff12b342`, source
`d4dde6ecc398cd96` (same path/instructions below). Physical evidence from
`console-cmenu-probe/20261011-090209-results` confirms V6 completed all eight
cases. The production shader reproduces the missing panels: 256 visible pixels
versus 2,801 in the oracle. Disabling culling, float inputs and uniformly scaling
clip coordinates did not repair it. Solid geometry is complete, original
texture rectangles are present, and minimal GPU skinning retains 2,800 pixels
(99.964% mask overlap). This isolates the textured full-shader path, not an
absent asset, but does not yet distinguish UV computation from output routing.
The probe crash correction has now passed this physical run. Latest Luma dump
is still the previously collected dump 193, not a new crash report.

V7 replaces three redundant controls in the eight-case batch:

| Case | New control | What changes |
|---|---|---|
| 1 | Standalone full scene shader | Same source/math/output declarations, without other linked entries |
| 4 | Full shared shader without Q output | Only removes TEXCOORD0W metadata; shader code/uniforms unchanged |
| 6 | Full shared shader with direct ordinary UV arithmetic | Uses vertex UV and constant 1 directly, bypassing the temporary used for fur/specular |

Cases 0/2/3/5/7 remain production/solid/oracle/textures/minimal-skin controls.
The direct-UV case is diagnostic only; it is not a valid fur/specular replacement.
Reassembling all eight archived entries must exactly reproduce the installed
scene binary before building variants. The existing linked ARM null-sampler
gate also remains required. Exact-binary V7 headless run passes all eight cases
in 4.88 host seconds; captures inspected. This does not predict its hardware
result. Main game, assets, saves and graphics preferences remain unchanged.

The output-mapping investigation also checked the SDK's
[DVLE output-map generator](https://github.com/devkitPro/libctru/blob/master/libctru/source/gpu/shbin.c)
and the documented [PICA output semantics](https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_SH_OUTMAP_Oi).
Neither source proves a hardware fault by itself. All publication remains through
the SDK; this probe adds no direct GPU-register writes.

11 October crash correction: probe `21b3b7eb3717` aborted before case 0
completed. Luma dump 193 identifies a CPU null-pointer read at address `0xC`,
PC `0x103ecc`, in stock SDK `C3D_TexBind(1, NULL)`. Its captured instruction
bytes match the archived executable. This is not evidence of a GPU failure.
The isolated probe had omitted the SDK null-texture correction already used
by the game and morph probe. V6 links that same narrowly patched texture
module; its build now runs 18 actual linked ARM cases with page zero unmapped.
`tools/test_cmenu_null_texture.py` reproduces the old exact hardware fault and
checks the corrected probe and current game. Azahar alone missed this bug.
Corrected probe `9663c9b7de4e`, source `65aba52ff2049afc`, passes all eight
headless cases in 5.41 host seconds. The physical rerun above completed; this correction
does not establish a fix for the game's missing menu background.

Launch `SD:/3ds/foxhollow-cmenu-probe/foxhollow-cmenu-probe.3dsx` through
Homebrew Launcher. It runs eight cases automatically and exits. No buttons,
save loading or FUR setting are needed. Reopen ftpd afterward. This is a
diagnostic application, not an updated game or a performance benchmark.

If it reports a GPU completion timeout, it retains GPU-referenced resources
rather than freeing them under an outstanding draw. Recover by rebooting.
There is no benefit in waiting after that failure message.

Each case draws the three missing panels using private prepared fixtures. The
40-byte geometry and tiled texture fingerprints match physical run
`console-visual/20261011-081146-223`. Pose/camera uniforms and vertex inputs come
from the earlier Azahar open-menu command capture. Geometry and texture bytes
are checked against the physical fingerprints. The physical shutdown observer
records a later, edge-on pose for two panels; that is not substituted into the
open-menu fixture. The differing pose rows are explicitly recorded in build.json.
The original linked 511-instruction scene shader is extracted from the installed
normal executable. This does not reproduce preceding gameplay draws, complete
live fragment state, asynchronous loading or GPU-visible memory contents.

Cases:

| Case | Variant | Question |
|---|---|---|
| 0 | Original scene shader, compact inputs, culling | Does the isolated draw render? |
| 1 | Culling disabled | Does face rejection remove it? |
| 2 | Solid material, no culling | Does disappearance precede texturing/blending? |
| 3 | Offline shader outputs through passthrough shader | Does the reference projected geometry rasterize? |
| 4 | Float input attributes with original scene shader | Is compact attribute decoding implicated? |
| 5 | Texture rectangles | Do the original tiled images sample correctly? |
| 6 | Homogeneous projection scaled by 1/256 | Does equivalent clip-coordinate magnitude matter? |
| 7 | Minimal GPU skinning shader | Is the larger shared scene program implicated? |

Cases 3 and 4 use **offline-prepared diagnostic data**, not a proposed CPU
gameplay renderer. Case 7 still transforms/skins on PICA. None changes game assets.

The probe uses an 8 MiB normal heap, 4 MiB linear heap, 512 KiB stack and one
240x400 RGBA8/D24S8 target. The command buffer is 256 KiB. All immutable GPU data
is flushed once after initialization, following the pinned Melee explicit-cache
ownership pattern. Color readback happens only after completion, using SDK
display transfer and cache invalidation; no raw depth readback or raw register
writes. Each queue wait is bounded at four seconds. In-flight resources are
retired before reuse/free. RGB readback is copied into a 288,000-byte ordinary
heap staging buffer before file IPC. Eight images consume 2,304,000 SD bytes.

Build/run with `tools/build_cmenu_gpu_probe.py` and `tools/run_cmenu_gpu_probe.py`
inside the project's Linux toolchain. `tools/analyze_cmenu_gpu_probe.py` creates
a contact sheet, pixel-coverage counts and reference differences. Installation
requires a passing exact-binary headless report and staged/final readback.

The first local test correctly rejected blank scene cases caused by an incorrect
probe boolean-uniform location. V2 fixes `0x68 + bit` and checks the resulting
mask is 1. That failure does not establish a gameplay defect or hardware cause.
V2/V3 image inspection rejected striped captures despite matching shader cases.
The initial explanation that shutdown poses caused the stripes was wrong. V3
keeps the earlier open-menu pose, but did not repair the capture. V4's RGB
transfer still produced stripes when file IPC read the linear GPU allocation
directly. V5 copies retired readback through ordinary CPU heap storage before
file IPC; complete panels and correct clear colours then appear. The precise
emulator/IPC cause is not yet established. This is a diagnostic capture repair,
not a gameplay menu fix. Bounds/clear-colour assertions now reject these earlier
misleading images. Exact source identity is recorded in the probe log.
