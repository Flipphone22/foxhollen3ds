Performance acceptance, 17 October 2026

This supplements the full goal; it does not redefine completion. Latest physical
Old80 baseline is source 7d87ef925cd7, palace case, 19.567 FPS / 51.106 ms.
Evidence: console-test-app-results/20261016-140836-229-direct/hardware-comparison.json.
The user subsequently asks for Old30; keep New60/steady30 and full visual/gameplay
requirements. Existing physical result is a five-second benchmark, not sustained
playability. A missing effect remains missing even when timing passes.

Provisional exclusive Old30 CPU wall budgets (targets, not predictions): native
submission 7 ms; presentation prep/copy/publication 4 ms; scene+actor visual
preparation 8.5 ms; simulation and remaining work 13 ms; total 32.5 ms. New60
requires a separately measured 16.67 ms frame, not a CPU frequency multiplier.
Overlapping GPU observations never add to exclusive wall buckets. GPU hardware
throughput still needs appropriate hardware evidence; no emulator clock changes.

Before changing a major path, name the existing repeated work, its current
evidence, the operation being removed, and the correctness dependencies. A
cache only helps when its lifetime actually spans reuse. Count publication,
hit/miss, command bytes, source reads/copies, draws and shader uploads where
relevant; existing instrumentation is preferred over new permanent overhead.

Keep candidate versus accepted state explicit. Acceptance of a performance
claim requires complete matched frame windows and unchanged settings, saves,
recipe, visibility and essential effects. Record differing animation/workload
counters and host contention; never convert ARM instruction counts to FPS.
Verify code scope and actual linked sources, inspect captures and exercise
the relevant known-failure fixtures, including the Old229 ThornTail save.
Local structural improvements can be retained pending hardware, but are not
an established hardware speedup. Do not reject useful incremental wins; do
not let them replace the main architectural work needed to close the gap.

Memory must include current process grant/commit, normal and linear usage,
VRAM, stack and commands. Latest physical normal peak plus conservative stack
is close to the existing 85% admission ceiling: new caches need offsets from
removed storage or explicit measured room, not another blanket allocation.
Retain the original assets/saves and normal application startup menu.

Validate new hardware mechanisms early, before building features on them.
Use matching offline evidence first and request hardware only at substantive
milestones or for unresolved hardware questions. Reproduce console failures
locally with the exact save/options whenever possible. Pin a passing build and
failure evidence; do not silently move the baseline. Remain on the safe inline
command transport until any different transport is independently established.

Current work: determine which portion of native submission remains dominant.
The whole-run 6,867 actor publications / 7,094 actor stream draws does not justify
assuming high cross-draw material-cache reuse. An exact-binary short ARM profile
will guide the rewrite. The known redundant world-template flush is separate;
fixing it alone must not be represented as the remaining 17.77 ms solution.

The fresh exact-binary eight-frame palace profile now exists in
artifacts/native/unified-save-heap-v1-benchmark-old-dsp-profile-v2. Packet update
accounts for only 0.71% of ARM instruction encounters; optimizing it cannot close
the whole-frame gap. The single-state change is justified as removal of duplicate
state and recovery of scarce normal RAM, not the main FPS solution. The next
substantial performance experiment must address larger combined preparation and
submission costs. Do not keep tuning this small kernel after its bounded checks.
Material publication counts are not distinct-value counts: do not conclude that
every publication invalidates a value-based cache without checking actual values.

17 October admission correction: successful execution was not sufficient.
The 7d87ef925cd7 physical Old palace log contains 2064 site-36 coverage failures
over the full run. The corresponding local profile contains 5653. Site 36 is
an attempted actor draw without its native scene asset, not merely an unused
resource load. The physical 19.567 FPS remains an observed timing result, but
must not be described as complete-rendering performance. Counts span the run,
not specifically the five-second timing window. Preserve the original records.

tools/audit_scene_admission.py now audits exact source identities against both
private native inventories and separately checks actual draw coverage. A load
rejection alone does not prove visible omission; recorded site-36 failures do
prove attempted missing draws. Remaining unknown admission cases prevent a full
rendering claim pending visibility evidence. The shared startup runner records
execution_status separately and marks failed-render-coverage when such draws
occur. A state checkpoint, nonblank image, clean exit, or zero vertex-layout
faults cannot override this result. Comparison tools also retain admitted asset
sets; differing coverage cannot support like-for-like whole-frame speedups.

The v5 memory candidate admits 61 scene assets versus the baseline's 49 and
reduces missing actor attempts to 661, but still fails rendering coverage.
Its full ARM window rises 20086230 -> 21104088 encounters, while the small packet
update kernel falls 142800 -> 124656. Do not extrapolate either to physical FPS.
Its packaging is explicitly rejected; passing lifecycle/memory checks remain
valid within their scope. Repair admission before using this as a new baseline.

17 October Old Fast compiler candidate: unified-native-lto-v3 recorded a 4.721%
reduction in full eight-frame ARM encounters versus coverage-correct V7, but
audio/particle work differs. This is NOT an attributed frame-time or physical
FPS gain. Both admit the same65 assets with zero attempted missing actor draws.
Native LTO must exclude retired compatibility code and audio/game enum ABI
boundaries: broader v1/v2 candidates were rejected before runtime. Preserve
strict shader inventory and fatal LTO type-mismatch checks. Do not spend another
round refining this modest compiler experiment instead of larger Old Fast costs.
The unresolved ThornTail bottom controls issue still prevents deployment.
18 October current-versus-hardware palace gate:

The latest collected Old hardware sample is 26.40484 FPS / 37.87184 ms from
88915fc75528 (132 frames over 4.999 seconds), not the older 23.0011 FPS run.
It is a short benchmark, not sustained 30 FPS.

The new exact-binary comparison caught an Old80 regression before timing:
36c4aaae18ff aborts with `PICA actor material plan` while loading Krystal's
material set. Preserve current-hardware-palace-v1-current-old-profile as a
failure, not a performance sample. Its prepared deployment package is held.

Candidate d478192e76be moves CPU material/sampler metadata to the existing
bounded linear allocation owner, alongside command plans. The shared 512 KiB
metadata ceiling and 15% linear reserve remain; no larger grant, shader, draw,
GPU input or per-frame allocation is introduced. Only platform_pica_material.c
changes; game archive and shaders are unchanged. Actual allocator functions
pass 560 host cases including rounded-allocation reserve rollback and failure.
The Old80 palace run completes, zero recorded missing actor draws, saves and
settings unchanged, owned exit/drain complete. Both palace captures inspected.
This is local repair evidence, not physical or SnowHorn-crash acceptance.

Full eight-frame ARM encounter windows 1301..1308: baseline core0 17,334,132,
candidate 15,126,800 (-12.734%); core1 5,479,222 -> 6,152,440 (+12.287%).
Recorded scene/draw fields differ, including lightning/particle work. Therefore
these are whole-window work observations, not an attributed optimization gain,
frame-time ratio or FPS prediction. Foreign emulators were observed; wall-time
measurement was rejected before launch. All profile timing remains excluded.
Same recipe, graphics mask, private saves, scene inventories and admitted asset
IDs verified. Some load-only admission cases remain unresolved; zero recorded
missing draws does not prove full rendering parity.

Current core0 leading shares: particle update 9.48%, drawGlow 5.98%, collision
triangle construction 3.89%, uniform updates 3.81%, memcpy 3.27%. Actor command
writer 2.65%; do not return to tuning that small kernel as the main solution.
Preserve simulation/RNG/collision behavior when reducing visual preparation.
Memory: heap 41,366,136/49,283,072, linear minimum free 3,642,112/12,582,912,
VRAM free 4,723,712, main stack 73,008/524,288, process 70,709,248/83,886,080.
Do not claim a peak RAM reduction: cache admission redistributes recovered room.
Evidence: material-metadata-pool-test/runtime-regression.json and
current-hardware-palace-v1-repair-comparison.json under research/pica-scene-rewrite.
Normal build only; matching replay and New runtime not yet validated/deployed.
