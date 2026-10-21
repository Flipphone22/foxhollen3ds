15 October: Old Fast CPU candidates and their limits

The 30 FPS Old Fast target remains unmet. Last physical result is9.925FPS;
none of this turn's experiments establish a console speedup. Lighting, ENV and
specular remain enabled. Optional visual development is deferred.

The resident texture-binding candidate removes redundant SDK requests using the
publisher's copied descriptor and resource-lifetime invalidation. Unlike the
old retained-register writer, this can avoid requesting another cache clear.
It never clears another owner's pending dirty/flush bit. ARM tests pass20539
checks plus three rejected fault mutants. The completed shortened ThornTail
replay retains168229 of292833 requests (57.45%). This is NOT57% faster rendering,
and other dirty units can still force a cache clear. Candidate8605b206e5f6 is
unpromoted. Game archive,18GPU programs, geometry and rendering order are unchanged.

The complete eight-frame instruction window from the longer (timed-out) run
attributes13.35% of observed instruction encounters to resampleVoice,5.33% to
mixRampChannel,4.48% to salCtrlDsp,3.92% to uniform publication and2.64% to reverb.
All run on Old core0. These are not CPU cycles or measured physical percentages.
The full timed-out replay is not accepted; only its completed profile window
is used. The shorter route subsequently passed normal state/save/drain/exit.

An independent audio-core1 experiment requests30% of the shared system core.
The actual mixer, queues, mutex, buffers and quality remain unchanged. Lifecycle
and core placement now pass, but late mix reports exceed the5ms slice being
produced. No net performance improvement is established. It stays test-only:
sharing the synth lock with a quota-limited worker may stall the main thread.
Do not promote it just because the worker starts and exits. The physical quota
and scheduling behaviour are unvalidated, and the captured replay is unsuitable
for timing comparisons. Source063938e54a66 excludes the sampler candidate.

The first placement check correctly failed because Azahar did not implement
svcGetProcessorID/SVC11. The custom audio-core-id emulator now returns the actual
running core and observes1. It also includes the current scripted-HID helper
and silent DSP capture; it is not claimed to differ from the older capture
binary solely by SVC11. Clocks, quota policy and scheduler were not modified.
The original emulator source/binary were restored. Parent memory telemetry's
requested_core is hardcoded to2 even onOld; ignore that field until corrected.

Audio v2 completed in108.47hostsec, with captures inspected and saved state,
shutdown drain and owned exit verified. Numeric stereo PCM checks passed; audio
was not listened to. Memory: heap42,115,784/50,331,648B, linear free3,547,648 of
12,582,912B, VRAM free4,723,712B, stack73,044/524,288B, process69,898,240/83,886,080B.
No new console requests, installations, save edits or asset changes were made.

Reproduce evidence with tools/analyze_resident_texture_bind.py and
 tools/analyze_old_audio_syscore.py; raw runs/tests/build identities are under
resident-texture-bind-test,old-audio-syscore-test and the named native run folders.
Keep both experiments separate from the accepted actor-fog-retention-v1 pair.
Next work remains structural PREP/DRAW reduction, not further optional fog work
or repeated unchanged collision/cache benchmarks. Approximately100ms must become
33.33ms; successful local correctness alone does not meet that target.

15 October indexed follow-up completed locally. Replay source3fa32c3a85d8,
3dsxb8d91db28367/CIA6122292e39ba; NOTDEPLOYED/NOTPROMOTED. Isolated build
changes only track_dolphin.o plus a shutdown report;685source inventory and18
GPUprograms preserved. Exact parent/candidate generated-file MD5 identities
prove the shadow-query implementation is unchanged. The shared mask index's
old shadow-named hit counters now include gameplay lookups; use the new Native
collision mask counters to separate them. No new index allocation.

Old80 ThornTail Fast replay passes save/state/log drain/owned exit; capture1770
inspected with Fox, world textures, HUD and actors.9950indexed block lookups,
0unindexed,237687candidate triangles.2000 source-vs-indexed mutable-map queries
pass ASan/UBSan,810603returned triangles,224capacity cases; topology reload and
unavailable-index paths covered. Existing load/retirement/index ownership reused.

No end-to-end gain established: single-run guest phase total19.076->19.673ms,
objectUpdate2.917->2.902ms, submission3.207->3.342ms. Draws171.66->175.66/frame,
indices51273->52823/frame and dynamic phases differ. These are not physical FPS
or a controlled causal regression claim. Keep the candidate unpromoted; do not
build/deploy a normal variant merely because correctness passes. Baseline/case/
inputs/assets are preserved in collision-mask-index-test/runtime.json.

Sampled indexed Old heap42132440/50331648,linear minfree3741184/12582912,
VRAMfree4723712,stack73044/524288,process69902336/83886080. Static diagnostics
add12B; aligned code/data changes raise measured process commitment4096B.
Full opening/endurance/Newhardware and physical performance are not validated.

The next priority is shared native draw preparation/submission and publication,
not more optional fog or collision-cache tuning. Physical PREP31.140/DRAW37.978/
PRES13.595ms remain far over the33.33ms TOTAL OldFast target. The cave instruction
profile is useful discovery evidence but cannot identify ThornTail's dominant
function on its own. Do not repeat these unchanged routes without a new question.

15 October priority update: the user now targets **30 FPS on Old 3DS Fast**,
replacing the earlier approximately20 FPS acceptance target for that preset.
This is33.33ms/frame; the previous physical result remains9.925FPS, not improved.
Preserve Lighting, ENV and Specular. Defer optional-effect development while
reducing Fast's active CPU work. New3DS targets and full parity/gameplay remain.

The checksummed latest physical settings slot from20261014-192331-229 has
mask2113537, geometry0, textures0, effects0: exact OldFast. FPS/PERF/GPU displays
were enabled. The captured final image is ThornTail Hollow. A new isolated
OldFast cave eight-frame instruction profile confirms height-fog draws0 and
identifies collision scanning, uniform updates, particles and audio. It is NOT
physical timing or a matching-camera decomposition of the hardware112ms frame.

The first bounded collision-plane cache is an unpromoted experiment. Its18KiB
cache passed8000 mutable-map/repeat query comparisons and reused73.19% of its
calculations in the real ThornTail route, but single-run Azahar total19.076->
19.776ms did not establish a gain. Workload differs171.66->175.60draws/frame;
objectUpdate2.917->2.892ms is too small to support a hardware speedup claim.
Do not deploy it or carry it into the stable builder automatically.

The existing profile was reused to inspect the source lines. Within the
1,864,188 collision-function instruction encounters,789480 belong to the
triangle-loop/cell-mask lines alone.485796 have no source-line assignment and
remain unassigned; out-of-line vector math is not included. A better candidate
reuses the existing load-time immutable cell-mask index for gameplay collision,
retaining live vertex/flag tests and original order. No second index allocation.
Its runtime acceptance and timing must be recorded before promotion.

Evidence: old-fast-profile/physical-preset-proof.json;
artifacts/native/old-fast-cpu-v1-old; collision-geometry-cache-test/{result,
build-verification,runtime,line-profile}.json; collision-mask-index-test/.
No new hardware claim or deployment. Larger render preparation/submission costs
remain the primary structural target after this bounded collision follow-up.

The regular application on Old 3DS displays a main-thread timing breakdown below
READ MAX when GRAPHICS → PERF is ON and the graphics menu is closed. It refreshes
once a second. Values are average milliseconds per rendered frame over the last
sample interval, not the last frame, CPU utilization, or hardware cycle counts.

| Label | Exclusive main-thread elapsed time |
| --- | --- |
| SIM | Object updates, environment update and collision |
| PREP | Scene/object rendering preparation, source checks, model refresh, visibility and bone palettes |
| DRAW | Native PICA submission scopes, including effects submitted through them |
| TEX | Texture resolution |
| AUD | Audio update, pumping, observation and sound-effect updates |
| LOAD | Game load-files service and DVD callbacks |
| FX | Remaining reflection, distortion and particle work outside nested native scopes |
| UI | Game UI start/end/draw and text work outside nested native scopes |
| GPUW | Instrumented GPU acquisition wait |
| WRKW | Render-worker wait |
| VBL | VBlank/display pacing |
| OTHER | Work outside all instrumented scopes |
| SET | Frame setup (`waitNextFrame`), excluding nested scopes |
| PRES | Presentation and APT/CPU service polling, excluding instrumented waits |
| IN | Controller sampling and the bottom-screen controls/telemetry drawing |
| MET | Periodic log, memory and SD telemetry |

MAIN is the complete sampled main-thread interval divided by completed render
frames. The sixteen exclusive groups sum to MAIN before display rounding; nested
draws are subtracted from PREP/FX/UI rather than counted twice. MAIN includes
blocking and scheduling. SD IPC elsewhere in the panel can overlap these groups
and includes worker activity: do not add it to MAIN. GPU Q also overlaps MAIN.
These labels identify the instrumented owners, not every possible reason an
owner blocks. For example, a synchronous asset read inside texture resolution
stays in TEX, and GPU waits outside the acquisition scope stay with their caller.

PERF OFF disables the new clock reads/snapshots. PERF ON adds scope timestamp
overhead; these are diagnostic numbers, not an uninstrumented FPS claim. There
are no new worker threads, per-frame allocations, file writes, GPU counters or
GPU fences. The existing replay exclusive phase definitions and function
signatures are reused. Worker calls cannot update the main-thread clock.
The first hardware panel combined SET/PRES/IN/MET/OTHER into REST. The next
revision splits them without adding any timing calls. On clean exit it records
all 35 raw phase totals, sampled frame count and the last displayed window in
the existing buffered native log. There are no extra gameplay-time SD writes.
The pinned Melee renderer's separation of acquisition wait from GPU drawing
time was consulted; its render/cache/resource ownership is unchanged.

This panel is currently enabled in regular Old3DS builds. New3DS keeps its
recording button in the same bottom-left area; replay keeps its existing CSV
timing system. The shared Old80/New124 binaries, saved settings and prepared
asset recipes are retained. Lighting, Environment and Specular remain ON in
all four named presets.

Validation: `tools/test_cpu_panel.py` checks all 35 phases, interval conservation,
owner exclusion, zero-frame samples, toggle resets and disabled clock reads under
ASan/UBSan. `tools/test_cpu_panel_lcd.py` checks the actual painter, Old Items and
New recording-area preservation, clearing and 10,000 unchanged no-flush paints.
`tools/build_cpu_panel.py` builds from the preserved lighting/palace heap archive,
checks unchanged shaders/game archive/recipes and restores unrelated root work.
`tools/run_cpu_panel.py` runs the exact normal Old80 binary headlessly with an
isolated PERF-on profile. Physical measurement/overhead remains to be checked.

The detail revision uses tools/build_cpu_panel_detail.py against the preserved
cpu-panel-v2 source archive. Tests additionally verify that REST separates into
five exclusive groups and that the report appears only on explicit clean exit.
Runtime startup evidence is reused from the unchanged v2 game and phase sites;
new physical accounting results are pending.

15 October first detailed Old3DS hardware receipt

`artifacts/console-test/20261014-173701-old229/cpu-analysis.json`
contains the actual saved report from the user run. Last sampled window:
107.186 ms/frame; PREP31.212, DRAW30.569, OTHER18.232, SIM16.286,
IN8.263, UI1.253, AUD0.560, MET0.356, SET0.309, remaining groups0.146ms
plus display truncation. Explicit GPU wait0.022ms/VBlank0.007ms/worker0.
GPU queue occupation is separate and overlaps these elapsed main-thread scopes.
This is not an exclusive CPU-cycle profile or matched benchmark. Full-session
averages mix startup/movie/load/gameplay and must not be presented as this scene.
The final two pre-exit SD rows have unchanged I/O counts/durations; that rules
out fresh measured FS operations in that narrow interval, not earlier stalls.
OTHER=0 would still leave88.954ms/frame; PREP+DRAW61.781ms is the larger target.
IN includes panel sampling/painting/cache publication, not just HID sampling.
The source still has uninstrumented game callbacks (e.g. pending map/water/UI
interface work); their costs cannot be guessed from OTHER. No new console
probe requested, settings changed or speedup established by this receipt.

Local LCD publication follow-up: tools/lcd_cache_batch_patch.py delays each
column-span flush until all widgets in that span finish writing. One left and
one right flush replace up to five repeated calls on the initial panel; updates
retain exact span bounds and the caller's subsequent swap. Actual painter test
pixels are byte-identical (153600bytes), Old Items/New record bounds retained,
and10000unchanged calls perform no flush. No hardware gain established; not yet
built/deployed. Isolated builder tools/build_lcd_cache_batch.py uses the deployed
CPU-detail archive; future actor-fog installer builder carries the patch too.

15 October follow-up: local LCD build `20034d910ea9` is now built, not deployed.
Its sole changed source is the LCD painter; game archive, shaders, conversion
recipes and Old80/New124 profiles match the deployed CPU-detail parent. The
original root test included an unrelated graphics-menu table, so acceptance now
uses **both exact archived painters** (`lcd-cache-archive-test/result.json`).
Both pass ASan/UBSan; all 153600 framebuffer bytes match, initial publication
calls fall from five to two, and 10000 idle calls produce no flush. This is a
call-count reduction, not a measured console FPS gain.

The presentation accounting has a confirmed coverage gap. `GXCopyDisp` calls
`fh_renderer_present` before `VIWaitForRetrace` enters `fh_platform_frame` and
its PRESENT scope. The latter calls present again, usually with no pending
frame. Consequently the real submission, command publication and clear outside
nested scopes can remain in OTHER while PRES looks negligible. The pure
`present_owner_timing_patch.py` scopes the entire real GXCopyDisp owner; existing
GPU/input/audio/VBlank scopes still subtract their exclusive time. It does not
move, remove or duplicate game/GPU work. Sixteen actual-body parent/candidate
cases pass ASan/UBSan: identical calls/order, caller restored, exact accounting,
and PERF OFF performs no added clock reads. This does **not** establish how much
of the physical 18.232 ms belongs to presentation.

Eight Old3DS-emulated cave frames of the later actor-fog replay were captured in
`artifacts/native/actor-fog-cpu-profile-v1-old`. The full run timed out at its
150-second bound and owned cleanup completed; it is **not** an accepted replay
or timing benchmark. The complete 1000..1007 scheduler window is usable only
as instruction-encounter evidence, with exact ELF/binary identity checks. It
observes eight GXCopyDisp and FrameEnd calls, but sixteen renderer-present calls,
consistent with that source path. Capture1410 was inspected: cave, Fox, textures,
HUD and haze are present. It does not establish full visual parity.

Largest individual encounter shares include resampleVoice6.75%, uniform
publication5.95%, collision-triangle construction5.83%, particle pool update3.92%,
memcmp3.46%, memcpy3.40%, GPUCMD_Add3.27%, material binding2.81%. Old core0 mixes
main and audio-worker execution; these are not main-thread percentages, retired
instructions, CPU cycles, or a decomposition of the physical OTHER bucket.
The case also enables height fog and is not the user's matching OldFast scene.
Do not convert these shares into predicted FPS or remove simulation work.

The combined local normal build is `59ee77d69f72` (3dsx `3e18fb854ccb`,
CIA `85ca65639515`), archived under `present-owner-timing-v1-console-setup-builds`.
It is not deployed. Only the painter and GXCopyDisp timing owner differ from
installed CPU-detail; all GPU programs and the game library remain unchanged.
The exact Old80 CXI passes ordinary setup/intro/cockpit/Select shutdown in
`artifacts/native/present-owner-timing-v1-old/summary.json`, with drained logs,
unchanged fixture/source and owned process cleanup. Cockpit capture inspected.
This is correctness evidence, not a matched performance comparison. In its final
emulated window OTHER is0.124ms and PRES20.204ms; this verifies that the newly
covered path can be substantial, not that physical OTHER was all presentation.
PRES still combines queue publication, copies and retrace callbacks; it is not
pure CPU work or pure GPU waiting. Further attribution belongs inside this owner.
Observed normal RAM39051680/50331648, minimum free linear5930368/12582912,
initialized free VRAM4723712, stack72936/524288 and process70995968/83886080.
Memory samples cover this startup/cockpit route only, not worst-case gameplay.

15 October presentation components follow-up

The normal application now has39 exclusive phases, still grouped into the same
16 LCD cells. Four appended phases all belong to PRES:

| Saved phase | Exclusive scope |
| --- | --- |
| present_prep | Remaining native presentation work, excluding nested draws, waits, copies and publication |
| present_copy | Native surface split and queued copy into the display target |
| present_publish | Early-queue deferral and final C3D_FrameEnd publication |
| retrace_callbacks | GX completion processing and original pre/post retrace callbacks |

The timing sites preserve original call order, resources, shaders and game work.
`test_present_components.py` runs the exact archived sampler under ASan/UBSan:
39-phase conservation, five phases grouped into PRES, worker exclusion and OFF
zero clocks. Its second, deliberately different window proves that saved last
phase values are the last window, not the whole-session mean. All39 last-window
tick counts/frame counts/microseconds are emitted only during clean shutdown.
No per-frame SD writes were added.

V1 normal `671f866461d4` completed the exact Old80 setup/intro/cockpit/Select run
in `artifacts/native/present-components-v1-old` (42.26host seconds, not matched
performance evidence). The cockpit capture was inspected; saves/fixture/source
were preserved and logs drained. Whole-run exclusive means were present0.018,
prep0.019, copy0.041, publication1.114, callbacks5.597ms/frame. Those means mix
startup/movie/menu; **they cannot decompose the final LCD window**. The final
LCD window was MAIN27.209, PRES15.325, OTHER0.120ms. The prepared movie decoder
continued through the cockpit window, recording576 frames at9.449ms mean each.
This identifies a callback workload worth investigating, not hardware timing or
proof of a GPU fence problem. The earlier physical movie portion logged roughly
20ms/decoded frame but stopped before the user's final hardware window.

V2 normal `c12ba39fda77` adds exact last-window retention/reporting over V1, with
no other source change. It reuses that runtime evidence and uses the new exact
sampler tests for its sole reporting delta. 3dsx`0e0347a84b05`, CIA`b85251128ce0`;
package`c208b32ccce313d9`. GPU programs, game archive and asset recipes unchanged,
Old80/New124 retained. Compared with59ee, ELF text grows640B and BSS416B; no new
normal heap/linear/VRAM allocations. V1 sampled memory was identical to59ee for
this route. A replay variant is not built; adding four phase ticks to its4096
rows would add131072B and must be included in that variant's memory checks.
No physical speedup established. Deployment status is recorded separately.

V2 uploaded to Old229, six payloads staged/readback-verified and published in
`artifacts/console-test/20261014-182853-old229/transfer.json`. Install
`SD:/3ds/foxhollow/foxhollow.cia` in FBI to update the HOME icon; the3dsx is ready
in the same folder. Saves/settings/assets/logs were not replaced. New223 unchanged.
