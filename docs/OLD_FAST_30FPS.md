16 October shared resident hardware replay88d497806a81 is built and passes Old palace/cave checks with inspected images and >15% sampled memory reserves. Uses established CPU audio; includes sparse resident state plus deployed sample-lifetime/post-capture repairs. Old229 transfer20261015-233614 COMPLETE, staged-byte verified; CIA requires FBI installation. Normal app/saves/assets remain unchanged. Separate DSP branch remains experimental. See RESIDENT_HARDWARE_20261016.md for scope, evidence, unresolved audio work and hardware instructions. No physical FPS gain established.

16 October combined renderer/audio candidate3ac5355ad37b now passes the short Old ThornTail replay. V1/v2 caught normal-heap admission failures; v3 keeps the original audio queue and repartitions existing Old48/12MiB heaps to48.5/11.5, unchanged total and15%reserve gates. Sampled headroom normal-after-stack15.50%,linear18.12%,process15.97%. Fullframe18.771->18.662ms versus preceding localDSPcandidate is only+0.587%rawFPS, not an established causal/hardware gain or comparison to the installed build. Exact matching inputs verified, absolute animation phase and diagnostic timing limits retained. WSL recovered; no deployment. See renderer-audio-v3-console-replay-builds/integration.json and native-renderer-audio-v3-old/renderer-audio-review.json. Audio lifecycle/fidelity and full performance goals remain incomplete.

16 October WSL recovery and combined renderer/audio integration

The user authorized restarting WSL. Normal shutdown hung; Windows denied a
service restart. Terminating the specifically identified WSL VM through HCS
succeeded, and Ubuntu/toolchain checks now pass. The three previously stuck
commands terminated; the pending opening replay had not started. No console
files, private assets or saves changed.

Combined replay v1 source450d74d00220 builds with 751 exact merged sources,
unchanged 18 GPU programs/game archive, and Old80/New124 profiles. Its Old
ThornTail check caught a real failure at loading Fox: actor command admission
stage2, request1576B, normal heap42256952/50331648, linear free3007104/12582912.
Normal heap plus stack/request exceeded the existing 85% admission ceiling.
The failed run is preserved as native-renderer-audio-v1-old (owned exit).

Candidate v2 keeps all memory reserve rules and moves only the 81920B stereo
raw AUX sample queue to cached linear RAM. Capacity, source timing and sample
processing are unchanged. It is CPU-only, so no new DSP/GPU flush is added.
Matching linearFree follows the existing joined shutdown. Pinned Melee audio.c
allocation/lifetime/publication rules were reviewed.139 actual-helper boundary,
failure and cleanup cases pass ASanUBSan. See audio-aux-storage-test/result.json
and adaptation.json. V2 builds/restores successfully; runtime acceptance pending.
Do not deploy this audio experiment: timing, resets and auditory parity remain
open. No new physical FPS claim; installed build and Old11.22FPS unchanged.

16 October renderer/audio branch integration prepared (NOT built)

The audio experiment retained the actor-fog-retention renderer, while the later
world-stream/actor-stream-v6 work was on a separate branch. Do not describe these
as already combined, or add their separate performance observations together.

merge_native_renderer_audio.py now composes the exact archived renderer91e9ef3f2fe2
and shared audio59e5490c6bed against common source1a732a666be0. It validates every
source record and pinned parent ID.20renderer files transfer unchanged; the only
overlap is build_arm_support.py's compiler-selection line. The merged condition
retains audio module SDK handling and adds the three resident draw units. Checks
over247actual source filenames prove the union; two unreviewed compiler changes
are rejected. Every audio-only and renderer input is retained byte-for-byte.
All three game libraries have the same SHA2562f219edec50e. This is source-merge
evidence, not a combined binary/runtime/performance acceptance.

Prepared bundle: artifacts/research/pica-scene-rewrite/renderer-audio-merge-v1/
selected-source.zip with merge.json and compiler-policy.json. Use isolated
build_renderer_audio.py console-replay --revision v1 when the toolchain recovers.
It requires1GiB free on both workspace and Linux build storage before touching
root files, and retains existing restoration checks. Both audio replay runners
now accept --build-family renderer-audio, retaining audio-cost as their default.
Do not build unrelated root WIP or deploy this unbuilt combination.

The C: drive reached zero free bytes during WSL failure. Removed only ten
completed/reproducible host test executables, preserving their source, fixtures,
logs and results. Then transparently NTFS-compressed218archived game libraries
and256archived ELF files, checking each SHA256 before/after. Recovered roughly
5.77GB; logical archive bytes, assets, disc dumps, saves and live compiler outputs
are unchanged. Reproduction/records: tools/compress_archived_builds.py and
artifacts/storage-maintenance/{20261016-host-test-cleanup.json,
compression-20261016-011358.jsonl,compression-20261016-011748.jsonl}.

WSL still does not answer after space recovery. HCS reports its VM RUNNING;
missing vmmem in a process listing is not proof it stopped. Existing opening
launch93963 and read-only queries61967/37739 remain live, with no opening run
directory. No duplicate startup or merged build was launched. User approval to
restart WSL is pending because that terminates all Linux jobs, including other
projects. No VM/service was restarted, no deployment or new performance claim.

16 October shared AUX connected to the actual game (LOCAL, not deployed)

Latest candidate is audio-cost-shared-aux-v3-console-replay-builds, source
59e5490c6bed,3dsx805132a157b3/CIA1694fd274ebd. Isolated parent830ca0e570e1;
unchanged game archive/private reverb kernels/GPU programs. Old80/New124 profiles
and complete root/archive restoration verified. No ordinary app build or console
files changed. Installed normal2fba94b84350 and physical11.22FPS remain unchanged.

The actual source studio loop now copies CURRENT raw software AUX after voice,
depop and studio-input mixing, before legacy callback processing. For the admitted
studio/bus this replaces the original effect call and wet-return loop. It leaves
software dry mixing, declined voices, simulation/control and source buffers intact.
CPU-reference mode retains the original callback. Capture precedes PCM stamping/
publication; no callback wait or silence substitution is introduced. A pending,
misaligned or full raw queue is still an explicit experimental fault. Do not
deploy this as production recovery or reset support.

The real source loop passed6912captured bus cycles in STD/DPL2 fixtures, with
995580nonzero sample observations, preserved voice state and bit-identical dry
PCM against the zero-effect reference. Legacy previous-cycle timing is checked;
wrong input rotation, wrong source position and double processing mutants reject.
Together with the previous queue/shared-original-kernel tests, this establishes
the connection and processing order, not end-to-end auditory acceptance.

One Old80/LLE short replay native-audio-shared-aux-v3-old passed in74.56hostsec,
ending normally at step4 with save preservation/log drain/owned exit. Its first
movement segment is180ticks, NOT the full ThornTail movement route. Screenshot
1000 inspected: Fox at the textured ThornTail entrance, world/vegetation/HUD and
Old controls present.370native voices/155876segments; source/callback faults0,
curve shortages0.5282software raw blocks captured; peak35/64slots,28retained at
shutdown. Reported consumed841170 is converter source_frames INCLUDING unknown
spans, not raw software sample count; actual raw consumption lies840640..840799
from slot retirement. Recorder explicitly qualifies that legacy telemetry label.

Added normal allocations81920bytes plus2204bytes metadata; no extra linear/VRAM
allocation. Measured heappeak42194760/50331648,linearfree2708736/12582912,
VRAMfree4723712,process70451200/83886080,mainstack73044/524288. Sampled aggregate
heap differences are not the exact allocation delta. PCM2318336scalars,RMS1242.68,
no clipped samples; these statistics are not listening/source-fidelity approval.

Reuse of the parent's first matching movement window gives full-frame raw guest
timing19.0204->18.7711ms,52.575->53.273FPS(+1.328%).106frames each; draws/frame
214.858->215.009,indices70839.11->70963.42. Save/input/assets/emulator/firmware/
settings match; scenario differs only by shortening this leg and removing later
legs. Absolute simulation windows/animation differ. Both diagnostic timings remain
EXCLUDED; this is not a causal gain or physical FPS prediction. It is plainly not
the required multi-fold improvement. Producer mean.50965ms,SDK-active.27194ms;
combined176.66elapsed-ms per produced second includes scheduling/preemption and
cannot be interpreted as CPU utilization or added to inclusive child buckets.

Earlier firmware AUX evidence already measured roughly4.8ms extra native wet
delay (audio-aux-analysis/result.json). The shared path now also sends software
wet audio through that buffered route. No delay compensation or original gate
acceptance is claimed. Native depop, mixed clipping, resets and general audio
fidelity remain open; do not spend another unchanged probe rediscovering the
already-known two-DSP-block versus one-source-block delay.

Build failures retained: v1 ARM format warning fixed; v2 manifest correctly caught
new imported audio_timeline_patch.py dependency. V3 explicitly selects it and
passes. Host capture test and build checks are in audio-aux-capture-test/result.json
and candidate/integration.json. Runtime records: audio-shared-review.json,
audio-cost-analysis.json,audio-frame-window-comparison.json,audio-window-input-proof.json.

An additional exact50-second opening recording check was requested with
run_audio_gameplay.py --revision shared-aux-v3 --proof integration.json, label
native-audio-shared-opening-v3-old. Runner now explicitly selects either proof
schema while still requiring all payload hashes and Old80/New124 grants. This
launch is PENDING: WSL dispatch and even read-only ps requests stopped returning;
no run directory exists as of the last observation. Do not call it failed/passed
or restart it without checking its live handle. Existing owned sessions93963
(opening),61967(wrapper ps),37739(direct ps) remain live; do not terminate shared
WSL or foreign work. No new hardware request. Goal remains active/incomplete.

16 October shared raw AUX consumer (local, not game-integrated)

The user's roughly11FPS Old3DS observation agrees with the already-collected
11.2195FPS/89.1303ms physical window. This is not a matched emulator speedup.
Both consoles still have normal2fba94b84350; experimental replay830ca0e570e1
remains local. There is no new physical measurement or deployment this turn.

Implemented audio_aux_input.c: a bounded single-producer/single-consumer queue
of copied signed32 raw software effect input, tagged with exact source frame,
generation and effect. CPU-only normal RAM; no DSP/GPU pointer, flush, wait or
filesystem operation. Pinned Melee audio.c publication/lifetime rules reviewed;
actual NDSP-buffer flush and shutdown ownership are unchanged. Caller must drain
both CPU owners before freeing storage. One continuous generation per queue.
Storage is81920B stereo or122880B three-channel, plus1064B metadata on ARM.
This is capacity, not additional latency. No app allocation has been added.

audio_aux_shared.c now combines this input with the converted DSP sends BEFORE
one original effects-history pass. The existing source-position/effect-span
mapping, converter and native processing kernels remain authoritative. Preflight
covers the whole returned block, including later spans, before converter/history
mutation. Missing input reports PENDING without waiting or substituting silence;
the eventual callback owner must retain and retry that exact DSP block. Arithmetic
overflow during application is a fault, not invented clipping or rollback.

Queue sanitizer tests exercise3.84million source frames across real producer/
consumer threads, serial wrap, partial slots, full pressure, metadata mismatch,
source-buffer reuse and late-channel overflow preserving all output/cursors.
Shared consumer agrees byte-for-byte with original fixed160-frame reverb applied
to the combined canonical inputs:1536returned bus blocks,12split blocks and48
lookahead blocks, stereo and three-channel.15344checks pass; four mutants reject
omitted software samples, source offset, swapped channels and skipped preflight.
Includes pending on a later span with existing valid input: no history advances.
Both modules compile strictly for ARM11; reported individual frame stacks are
96B maximum for the queue routines and256B for the shared consumer (not cumulative
call-chain stack). No additional app/emulator boot for these unconnected modules.

Not yet integrated: current raw source-cycle capture, callback retry scheduling,
source reset handling and wet/dry delay reconciliation. Original software effects
consume the PREVIOUS160-sample cycle; native AUX returns through a different
buffered path. Matching canonical processing does not prove end-to-end timing,
source waveform clipping, native voice depop or auditory parity. The game still
uses separate effect processing until those integration requirements are handled.
Do not remove the source callback or claim saved CPU time from these module tests.

Evidence: audio-aux-input-test/result.json and audio-aux-shared-test/result.json
under artifacts/research/pica-scene-rewrite. Reproduce with
tools/test_audio_aux_input.py and tools/test_audio_aux_shared.py in the Linux
toolchain wrapper. The latter retains its exact generated fixture and original/
native kernel identities. Goal remains active; no performance gain claimed.

15 October ordinary-world command integration (LOCAL, not a performance acceptance)

Ordinary world drawing now uses resident native command plans compiled when assets
load. The live path selects prepared raster/sampler descriptions and publishes
camera, tint, texture-frame and scrolling parameters; it no longer invokes the
ordinary per-draw SDK material binder or rebuilds fetch/raster descriptions.
Culling, draw order, hidden flags, images and geometry are unchanged. Actors,
specialized native materials and active height fog still use their existing native
owners and remain architectural replacement work. No GX interpreter was added.

Pinned Melee command-cache and raster ownership informed the handoff. Existing
resource fences/cache publication remain authoritative; the raw world lane owns
only depth/cull/alpha/blending, preserves inherited stencil/early-depth/fragment
state, and restores affected SDK caches and uniform rows7..9 on departure. Actual
ARM publisher tests pass92683 assertions and8 rejected fault variants; compiler
checks pass370323 assertions and9 rejected faults. GPU output was checked separately
through real Old80 replay runs; this is not physical-console acceptance.

Latest archived replay: world-stream-v3-console-replay-builds, sourcebc59c33e60a3,
3dsx63797dcc6a85/CIA345c49a80091. Build_world_stream.py isolates the accepted
actor-fog-retention source/game archive and restores root WIP. All embedded GPU
programs/game archive remain unchanged; shared CIA requests Old80/New124. No normal
variant build, console deployment, private asset/save/preference change occurred.

V2 cave replay routes all86669 world draws through the new lane; former ordinary
binder calls are zero. The matching FULL-FRAME observation is16.8233->16.7244ms,
only+0.59% raw virtual FPS, while submission is4.5997->4.1061ms and VBlank waiting
increases. Both are near the emulated60Hz pacing limit. Foreign emulator activity
excludes causal timing attribution; these numbers are not console FPS predictions.
The physical9.925FPS baseline still needs about3.02x throughput for30FPS. This
world-only step does not meet that requirement; no substantial gain is claimed.

The larger ThornTail fixture exposed residency pressure. Sharing one geometry
descriptor across three raster alternatives removes140bytes perdraw (248->108).
V3 prepared-world storage peaks at170452bytes versus221972 inV2; total material
pool408692/524288bytes, all freed on exit. It uses the existing material allocation
cap/85% normal-heap gate, not a new unbounded allocation. The separate256KiB
geometry-plan cap remains. V3 Old measured normalheap42244744/50331648bytes,
linearfree3746304/12582912,VRAMfree4723712,stack73044/524288,process69910528/83886080.
No new linear/VRAM allocation or shader/pass/worker was introduced. Actor/texture
loads vary between runs, so aggregate heap deltas are not pure storage benchmarks.

Cave and ThornTail captures inspected: scene geometry/textures, Fox and HUD remain.
Mist and animation phases differ; full visual parity is not claimed. Both variants
complete state/save/drain/owned-exit checks; V3 ThornTail49849 worlddraws use the
new lane with zero ordinary binder calls and zero material errors. No New hardware,
physical throughput, normal-app startup, long progression or endurance acceptance.
Evidence: artifacts/research/pica-scene-rewrite/world-stream-test/{runtime,integration}.json.
Reproduce with build_world_stream.py, run_world_stream.py, analyze_world_stream.py
and record_world_stream.py. Next is actor/material/palette command ownership and
remaining render preparation; do not repeat unchanged world diagnostics.

# Old 3DS Fast: 30 FPS work

The current target is 33.33 ms per rendered frame, with lighting, environment
lighting and specular enabled. The last physical sample remains 9.925 FPS.
None of the local candidates below establishes a console speedup.

## Prepared whole-draw stream (15 October, local prototype)

The next renderer layer now has an implemented load-time command compiler and
resident-state publisher in `pica_draw_stream.c` and `pica_draw_resident.c`.
It is **not connected to the game yet**, and establishes no FPS improvement.
Do not confuse this with another material-only packet or with completion of
the renderer replacement. It must replace the ordinary world's binding loop,
not run alongside that loop and duplicate its work.

The compiler consumes native attribute, geometry, raster and four-stage material
descriptions. It retains the pinned SDK's exact command ordering and draw side
effects. Texture-frame descriptors are compiled separately. Colour constants,
texture selection, scrolling/scales in uniforms 7..9, fog configuration and
environment colours remain live. Shader/camera/bool state, render target,
lighting/fog LUTs and TEV4/5 remain explicit surrounding owners. This first lane
does not admit geometry shaders, cube maps or special actor/effect materials.

The resident compiler interns immutable command blocks with hash **and exact
byte comparison at load time**. The runtime never hashes, searches the arena,
validates source GX data, calls SDK setters or reconstructs these descriptions.
It compares resident identities, emits changed blocks and patches changed buffer
addresses. Full layout transitions restore all twelve SDK streams, including
inactive ones. It compares the small live colour/uniform values and publishes
changes. Texture refresh is explicit even when the address/descriptor is stable.
Every draw still includes all thirty original draw-command words.

Storage is bounded by caller-provided normal RAM; there is no hidden allocation,
worker, linear-RAM/VRAM allocation or resource copy. ARM sizes are 76 bytes per
resident draw, 160 bytes per active state cache, 268 bytes per arena descriptor
and 32 bytes per immutable sampler. Interned words consume additional arena
space. The 784-byte full template is load-time scratch/reference output, not a
required persistent allocation for every draw. These sizes are not the actual
game resident total; that still needs inventory and memory admission gates.

Pinned Melee `citro3d_fix.c` and `command_cache.c` informed this design. Unlike
Melee's per-draw packet construction, compilation happens at asset admission.
The adaptation preserves primitive restart, draw start/end, geometry-stage
toggles and both primitive-cache writes. Existing exact-range cache publication
and GPU fence retirement must remain owners of the finished command segments
and referenced resources. Neither append function submits or flushes anything.

The SDK handoff is now implemented in the isolated `draw_stream_owner_patch.py`
adaptation. Begin publishes pending SDK/program state, including the established
GS to VSH boundary. Successful raw draws set DrawUsed; capacity failures leave
the cursor/state unchanged and must be retried after the caller's usual reserve.
Explicit end, SDK context updates, command splits, APT restoration and teardown
close the batch. They invalidate affected SDK flags and retained caches, including
the three float32 material rows7..9. Viewport/scissor, TEV4/5 and sampler2 register
cache entries remain valid; no unrelated camera/bone/light uniforms are dirtied.
No register submission, cache flush, wait or resource retirement is added by end.
The owner adds8 bytes of static metadata plus the existing caller-owned160-byte
batch state. Game callers and build integration are still absent.

`test_pica_draw_owner.py` executes the actual archived retained SDK publishers
and actual libctru serializer on ARM11:91116 checks pass across124 draws and21
splits, including the GS boundary. Eight faulty variants reject. The test checks
warm buffer provenance, uniform/raster cache restoration, failed-capacity retry,
nested/invalid admission, untouched raster state, target/APT changes and teardown.
Shader upload and GPU queue consumption are mocked. This is CPU ownership/command
evidence, not GPU pixels, physical cache coherence, retirement timing or FPS.
Earlier test gaps (buffer provenance lost at initial framebuffer publication and
split closure only observed after a later context update) are recorded in
`draw-owner-test/missing-handoff-coverage.json`; both now reject their faults.

Callers must reserve the current segment and bind the actual world program,
camera/scissor and common effect owners before begin. SDK setters and external
raw writers are forbidden inside the batch; close first to change those values.
The per-draw guard rejects pending SDK raster/program/target or bool changes,
but does not scan every float uniform for an out-of-contract writer. Immutable
resources remain resident until the existing GPU retirement point. Reset the
resident cache before arena/sampler address reuse. No fallback to the retired GX
interpreter is permitted. This ownership implementation must now be connected
to the real world loader and draw loop before any performance claim.

Game admission must compile the bounded raster/texture-option variants at load
time, retain material boundaries and existing culling, and read actual live
game parameters. Do not move the compiler into the frame loop to handle flag
changes. Special native effect owners remain separate until their own command
plans are implemented. The next acceptance gate is isolated game integration,
actual GPU output and a matching whole-frame comparison. Synthetic
command counts against a full SDK bind do not establish improvement against the
game's already-retained SDK path or establish physical performance.

## Full-frame priority and bounded effect ownership (15 October)

The user explicitly reiterates that the raw12.9% Azahar comparison is far below
the required improvement. The physical20-second sample is9.925168FPS, or
100.753959ms/frame. Reaching30FPS requires3.022619 times its throughput and a
66.9161% reduction in elapsed frame time. No experimental candidate has achieved
that. The separate final9-frame window is112.047ms, with31.140ms preparation,
37.978ms native submission and13.594ms presentation (sum of rounded phase values).
These three owners total82.712ms; everything else still totals29.335ms. Simply
improving a small function cannot close this gap. These scopes include scheduling
and blocking; audio-worker preemption can appear inside rendering scopes. They
must not be interpreted as exclusive CPU cycles or mixed with the20-second FPS.

`old-fast-full-frame-budget.json` binds the physical input hash and records these
calculations and limitations. The next priority is broader persistent native
command preparation/publication replacing per-draw SDK reconstruction, together
with live parameter updates and correct resource retirement. Prior material-only
packets, shader-bank tweaks and palette scheduling did not establish the needed
gain. Do not repeat those experiments or continue audio cleanup as the sole route.

The in-progress audio ownership change is now checked and preserved separately.
`audio_effect_epoch.c` provides two caller-owned payload slots. The producer
publishes prepared state; the SDK callback brackets processing with begin/end,
then advances a retirement watermark only when no older input, queued bank, bus
callback or necessary tail can reference that token. The producer collects
retired payloads with acquire/release ordering. Unknown/backwards/nested/busy
operations and duplicate live payload pointers are rejected. Busy never discards
an effect update or migrates an existing native voice into stale software state.
There are no allocations, locks, waits, SDK calls or GPU operations in the owner.
ARM metadata is36B; this does not reserve or budget the future effect buffers.

Effect tokens must follow actual source effect lifecycle, not just source output
clock generation. The original `ReverbSTDModify` frees/recreates delay lines even
with identical parameters. A clock reset alone does not prove that operation
occurred. The bounded owner does not implement those source hooks, mixed output
blocks or game retirement policy; current reset/route guards remain in place.

`test_audio_effect_epoch.py` exercises actual original STD reverb state through
80blocks/38400samples and582 assertions under ASan/UBSan, including old-tail work
after newer state is prepared, identical-parameter recreation and safe reuse.
A two-thread SPSC test publishes/processes/reclaims20000 payloads. Four compiled
fault variants fail assertions. Strict ARM11 compilation verifies36B metadata,
barrier instructions and no external calls; ARM code was not executed and host
threads are not weak-memory hardware proof. Initial mutation selection matched
two functions; it was narrowed before compiling. The final run includes duplicate
payload guards. Evidence is `audio-effect-epoch-test/result.json`.

No game build, emulator run, deployment, save/asset change or new performance
measurement occurred in this stage. The local v7 experiment remains incomplete
and undeployable. Preserve it while returning to the larger rendering costs.

## Returned effect-input identity (15 October)

The SDK calls the ordinary audio callback before its AUX hook. That callback
prepares **upcoming** channel controls; the hook reads an **older** AUX return.
Using the producer's or upcoming playback generation to select effect state is
therefore unsafe. Private-firmware LLE tests with two independently marked lanes
on both buses identify the return as two publications earlier. A deliberately
delayed callback produces five dropped DSP frames without changing that bank
association. This is measured transport behavior for the pinned firmware, not
proof of physical timing, all sleep/resume paths, or complete source fidelity.

`audio_dsp_aux.c` now provides an optional tagged callback and SDK-callback-only
`fh_audio_dsp_aux_tag`. The tag follows the actual SDK bank. Both callbacks read
the returned identity before it is replaced with the new publication's token.
Frame age/order, missing tags, invalid mapping, reinstallation and per-bus
enable history are checked. Unknown provenance is zero; it never silently uses
the latest producer generation. Tokens are opaque and do not themselves release
effect buffers. The caller must keep any referenced state alive. Native owner
size increases from2588 to2628 bytes; no sample storage, allocation or thread is
added. Existing game callbacks are still untagged and the reset panic remains.

`test_audio_aux_sdk.py` executes the native adjunct with the pinned actual SDK
enable-state implementation on ARM11. **165234 assertions pass**, including
unchanged planar PCM/bounds, repeated/skipped/wrapped counters, missing tags,
disabled origin, malformed bank and reinstallation. Eight compiled faulty
variants fail assertions. Source/header/test hashes are checked before compiling
and after execution. This fixture simulates shared RAM and locks, so it is not
concurrency or firmware proof. The separate exact-source firmware run supplies
the transport check.

Reproduce that run with `test_audio_aux_epoch_device.py --delay --gates` through
the existing Linux wrapper. It is silent, isolated and bounded45seconds, with
owned-process cleanup. Final run `20261015-071154` completed in6.85 hostseconds:
292 observations,282 identifiable older returns,6 deliberately unavailable bus
rows and4 startup rows. All292 expected metadata values match. AUX B is disabled
for four publications; AUX A continues. No extra hardware probe is requested.

An initial constant-marker interpretation found zero matches because firmware
ramps mix gain even for constant PCM. The corrected independent marker analyzer
identifies the two-lane end values within less than one marker step, verifies
the preceding publication at block start and checks the min/max envelope. Both
original raw captures remain. This is not a reverb waveform comparison or a
claim that a whole transition block contains only one generation's contribution.
The SDK fixture initially hit its pinned unused-argument warning; only that SDK
unit gets a warning exception. A faulty variant also needed to keep its variable
used so the mutation could execute; the final result has eight executed failures.

Pinned Melee `port/3ds/audio.c` exact publication and `ndspExit` resource-lifetime
rules were reviewed. The adjunct continues to use SDK-mapped shared RAM and the
existing guarded hook, without new cache/DMA operations. No game archive or
deployed app changed. Remaining work is effect-state publication/retirement and
mixed-generation PCM regions before connecting the source reset API; the current
experimental app is not deployable. No additional FPS gain is established.

Evidence: `audio-aux-sdk-test/result.json` and
`audio-aux-epoch-device/20261015-071154/{summary,identity-analysis}.json` under
`artifacts/research/pica-scene-rewrite/`. Earlier marker captures070052/070125
and tagged no-gate capture070732 retain their narrower scopes.

## Source-side audio reset ownership (15 October)

`fh_audio_native_voices_reset` now advances the producer's source generation and
clears its voice mappings without changing any DSP command, descriptor, sample
lease or SDK channel. The source producer can be ahead of buffered playback, so
immediately cancelling old DSP voices would truncate still-valid buffered sound.
The existing callback instead retires old generations when playback reaches the
new generation, keeping its sync/sequence acknowledgement requirement intact.
New source starts use free channels, or remain software-owned from their first
sample when channels are still occupied. Existing software voices are never
silently promoted. No heap, buffer, worker, GPU work or owner-size change is added.

The reset invalidates AUX admission and clears old source-effect identities.
Effects need an independent, generation-safe handoff before rebinding. In
particular, the game's current reset panic is STILL present: this API is tested
but deliberately not connected to `platform_audio_game_dsp_prepare` yet. It does
not reset effect buffers or solve mixed-generation regions inside one DSP block.
This is implementation progress, not a completed game reset fix or FPS gain.

Pinned Melee cache-publication and SDK-join resource lifetime rules were reviewed;
those paths remain unchanged. `test_audio_native_reset.py` executes the actual
original source-control/mixer and pinned libctru channel owner on ARM11. Its744
assertions cover playing/queued old generations, source position/ADSR equivalence
after reset, repeated resets before playback catches up, all22 channels occupied,
fresh software starts, AUX invalidation, monotonic rejection, and retirement
with stale versus matching SDK replies. Complete DSP owner, sample-pool and
3168-byte SDK channel snapshots remain byte-identical across the producer reset.
Only acknowledged samples can be reused; new AUX admission succeeds after drain.
Five faulty variants reject stale mappings, early playback mutation, stale AUX
binding or generation rollback. These are synthetic replies, not real DSP audio
or weak-memory/concurrency acceptance. Test input hashes are bound before
compilation and verified unchanged afterward.

The initial563-check fixture was extended to744 with acknowledgement/reuse
coverage, then rerun after the final fresh-start setup and input-hash guards.
All checks pass; no unchanged game boot, tone capture or full DSP protocol suite
was repeated. No new app build/deployment, asset/save/settings changes or hardware
request. Latest archived replay remainsv7 and remains experimental. Next integrate
effect generation ownership and mapped output regions before removing the game
reset guard; do not use this source-only result as proof that transitions work.

## Software reverb span integration (15 October)

The original software STD reverb remains necessary for voices not admitted to
native DSP playback and for their effect tails. Its callback was still using the
older cursor kernel. The new adapter replaces that kernel with contiguous spans
of the four independent delay rings, preserving the public callback, constructor,
parameter modification, allocation/free behavior and complete signal state.
The high-quality reverb route is unchanged. This also benefits the CPU-reference
mode; it does not require completing the experimental DSP migration.

`audio_source_reverb_span_patch.py` generates a self-contained change to the
archived `arm_overrides.py`. Pinned Melee audio publication, cache flush and SDK
join/lifetime handling were reviewed; none changes. There is no extra heap,
linear RAM, worker, GPU work, or removed effect. Clang stack reports show the
channel kernel increases from144 to156bytes, with the16-byte callback unchanged.

`test_audio_source_reverb.py` executes the actual fixed160-sample callback against
the archived optimized cursor callback:61,440 blocks /29,491,200 samples pass
ASan/UBSan, with exact samples and complete delay/filter state, silent decay,
ring wraps and live parameter modification. Three mutations reject altered ring
wrap, lost tail state and wrong all-pass arithmetic. Constructor, modification,
callback and free function bodies are verified unchanged from pinned source.
Twenty ARM11 cases compiled with the game compiler preserve full state, reducing
instruction encounters877,940 ->606,893 (-30.87%). The original cursor kernel
was already optimized, so the earlier65% result against a different reference
must not be applied here. Counts are not cycles or whole-frame savings.

Isolated replayv7 source52796dbe4af0 /3dsx859ae4bd414f /CIAf5564d4b790a changes
only the archived override generator fromv6. The actual linked old/new software
callbacks pass20 full-state cases with persistent pre-translation hooks;
instruction counts877,960 ->606,913. Game archive,18GPU programs and Old80/New124
profiles remain unchanged; root/archive restoration passes. Allocated text grows
448bytes, with no data/BSS growth. Evidence: `audio-source-reverb-test/result.json`
and `build-verification.json`. The surrounding DSP migration remains experimental;
this is not a normal-build deployment or full audio-fidelity acceptance.

One exactv7 Old Fast ThornTail run `old-fast-audio-source-reverb-v7-old` completes
in131.81hostsec with status1, unchanged saves, drained logs and owned exit.
The1000 capture retains Fox/world textures/HUD; the34.54s captured waveform and
spectrum were inspected.122models load without rejection,345voices admitted,
9declined,146447segments,116696retained controls; zero source/callback/curve faults.
No clipping or reported PCM underruns, but the previously observed interior
silent interval remains (26.16..29.57s,3.41s), unattributed rather than accepted.
Producer elapsed average705.48 ->635.67us; SDK248.31 ->246.98us. These intervals
remain inclusive/preemptible and cannot be summed into exclusive CPU utilization.

Sampled heap42230800/50331648, linear minimum free2708736/12582912, VRAM4723712,
main stack73044/524288 bytes are unchanged fromv6; audio worker upper stack4096,
core0. Process commit grows by one page to70434816/83886080 with the added code.
No new allocation/buffer or full memory/endurance claim. Runtime data lives in
the run's `retained-runtime.json` and `audio-cost-analysis.json`.

## Full-frame diagnostic comparison (15 October)

The user requested full frames rather than isolated audio speedups. New
`compare_audio_frame_windows.py` differences the replay's complete boundary tick
counter over the movement step,60..180 simulation ticks after its first measured
sample. It includes waits and does not add nested component timers. Original
CSV hashes, frame counts, executable/emulator identities and differing draw/index
workloads are recorded in `audio-full-frame-estimate.json`.

Earlier experimentalv2 ->v6, on the same capture emulator, gives19.8469 ->19.2156ms
over101/104 frames: raw3.18% shorter frames,3.29% more FPS. Software-referencev4
->v6 gives21.4550 ->19.2156ms: raw10.44% shorter frames,11.65% more FPS. The latter
uses DIFFERENT profiling/capture executables. All existing timing exclusions
remain: neither ratio is accepted causal speedup or physical-console evidence.
The rough conversation estimate of about10% overall is a conditional diagnostic
projection, replacing an earlier loosely grounded5% estimate; it is NOT a claim
that the installed9.925FPS build has become10.9FPS. These local reference builds
are not the last deployed executable. Simulation/animation/audio workloads differ,
and host elapsed throughput is separately excluded. No evidence supports30FPS.
The initial report script incorrectly expected a top-level emulator hash; fixed
to use the recorded `emulator.sha256`, without changing the underlying data.

The subsequently completedv7 run gives19.0003ms over106frames in the same relative
movement window. Raw full-frame FPS ratios are+1.13% versusv6,+4.46% versusv2,
and+12.92% versus the DIFFERENT-emulator software reference. Recorded separately
in `audio-full-frame-v7.json`; earlier estimate data is preserved. All causal
comparison exclusions remain. The131.81 versus193.11hostsec run durations are
NOT a32% game performance improvement. No physical speedup is established.

## Retained native audio controls (15 October)

The producer now retains a contiguous identical constant source curve instead
of submitting the same pitch/pause/interpolation/routing/gain state every1ms.
Its last published slot is immutable: only this producer writes command slots,
and it never edits a command already visible to the callback. Retention requires
the same lease, matching constant coefficients and an exactly contiguous source
interval. All normal admission checks still execute, and the producer deadline
advances even when no queue slot is needed. Ramps, changed controls, gaps and
stops remain explicit. The callback can hold constant gain without reporting a
missing ramp segment. Source simulation, ADSR, callbacks and sample clocks still
advance. Curve mode also avoids computing unused endpoint-only gains.

Pinned Melee publication and SDK-join lifetimes were reviewed; neither changes.
The DSP owner grows459104 ->459280bytes (+176 normal bytes), with no new buffer,
linear allocation, worker or wait. Original source fallback reverb is retained.

`test_audio_dsp_epoch.py --retained` passes23487 actual-SDK protocol assertions;
adding `--game-capacity` passes24243. Both reject17 faulty variants. Coverage
includes10000 retained segments, every changing property, untouched future
deadlines, repeated ramps, gaps, full queues, late stops and resource retirement.
These use synthetic DSP replies, not hardware or weak-memory stress.

`test_audio_native_voice.py --out-name audio-native-retained-test` passes the
original source-control/SDK checks:123113 for the ordinary route, plus four
AUX/DPL2 variants and eight rejected faults. Source states and output gains
remain checked. Its first run failed an obsolete251-command expectation:
the new result is248 queued entries plus3 retained segments, still250 source
segments and one START. The corrected accounting remains coupled to source
state and SDK-output checks. Added telemetry was initially garbage-collected
from the non-AUX fixture, causing a reader KeyError; explicit initialization
keeps it linked. These were test-harness failures, not silently accepted runs.

Private-firmware LLE paired delay37 test `audio-native-voice-device/20261015-055941`
finishes in10.71hostsec:420 source segments,339 retained, zero underruns/clips/
prefix faults. Both128000-byte PCM captures are byte-identical to the prior
051528 capture; the full envelope plot was inspected. Original/native level
and release differences remain, so the broader analyzer retains accepted=false.
This is not live game reset/AUX or physical-console validation.

Isolated v6 source9a232418915a,3dsx86d8f9e4d6d2,CIAc0ea59837f33 builds and restores
successfully. Exactly four sources differ fromv5: DSP header/owner, native voice
adapter, shutdown telemetry. Actual archive sources match the tested hashes;
the callback's generation-handling body is unchanged. Original game archive,
all18GPU programs and sharedOld80/New124 profiles pass. Allocated ELF deltas:
text+624B,rodata+64B,BSS+192B (owner176B plus alignment).

One short Old Fast ThornTail run `old-fast-audio-retained-v6-old` completes in
193.11hostsec with status1, unchanged saves, drained logs and owned exit.
It loads122models with0rejects, and captures1000 retain Fox/world textures/HUD.
367voices are admitted,8 declined;125626 of157029 source segments retain their
controls (80.0018%),31314 controls are consumed. The remaining89 are not claimed
as consumed; exit may retain or discard queued output. Source/callback faults
and curve shortages are zero. The35.29s audio envelope/spectrum was inspected:
zero clipping and reported PCM underruns, but exact silence includes13.73s at
startup and a3.065s interval at27.20..30.26s. The latter remains unattributed;
zero underruns does not establish uninterrupted audio or original-game parity.

Observed inclusive/preemptible averages are705.48us per producer block,
248.31us per SDK callback and177.38us for its AUX child. These are not a matched
comparison, exclusive CPU utilization or an FPS multiplier. The80% command
reduction is not an80% reduction in audio CPU work or frame time. No new physical
speedup is established. Sampled heap42230800/50331648, linear minimum free
2708736/12582912, VRAM free4723712, stack73044/524288 and process70430720/83886080
bytes leave the existing measured headroom intact for this route.

Evidence: `audio-dsp-retained-{test,game-test}/result.json`, game-test build
verification, `audio-native-retained-test/result.json`, paired-tone
`retained-comparison.json`, and game `retained-runtime.json`/`audio-cost-analysis.json`.
No normal pair/deployment/assets/save/preferences changes. Source reset, mixed
epochs, live AUX changes, full audio fidelity and sustained performance remain
open. Continue reducing remaining native costs without repeating unchanged
protocol/tone tests or treating this short route as final playability validation.

## Dedicated native stereo AUX conversion (15 October)

The v4 profile attributed983,524 instruction encounters to generic native
rate conversion in its eight-update window. The new DPL2 stereo route performs
paired L/R interpolation using one fraction, avoiding generic channel traversal
and unused surround arithmetic. It retains the same32kHz effect clock, floating
operation order, saturation, causal delay, history and phase. The separate
three-channel route remains available; no effect is disabled and no allocation,
thread or wait is added. Pinned Melee's audio publication and SDK-join lifetime
handling was reviewed; neither changes here.

`test_audio_aux_stereo_pass.py` compares the exact archived v4 adapter with the
candidate under ASan/UBSan:16,392 cases/10,490,880 sample outputs plus complete
state equality, including consecutive blocks, extreme signed values, partial
initial silence and invalid output cursors. Three mutations reject wrong phase,
history or channel selection. The existing actual-original-reverb comparison
also passes10,485,760 samples. These are not complete game audio-fidelity tests.

Actual ARM11 execution passes24 state comparisons. Stereo instruction encounters
fall395,668 ->280,046 (-29.22%); the unchanged generic three-channel algorithm
compiles slightly differently after the shared helper extraction and falls
497,614 ->472,258 (-5.10%). This includes the fixture's identical effect callback.
The first count attempt added hooks after warm-up and missed cached code; its
`invalid-count-v1.json` is retained and excluded. One persistent hook now precedes
all translation. The earlier50-case reverb test was rechecked with that same
discipline: its counts and output/state conclusions are unchanged, including
833,154 ->290,640 for normal blocks. Original report retained as
`audio-reverb-span-test/arm/pre-persistent-hook-result.json`.

Isolated v5 replay source6c60e474db1d,3dsx8fd2fbd668e8,CIA38b922e26d46 is built.
Only `audio_aux_rate.c` differs fromv4. Actual linked adapter output/state matches
the archived reference in24 cases. Original game archive and18GPU programs,
shared Old80/New124 profiles and root restoration pass. Allocated section delta
is744 additional text bytes; other sizes are unchanged, no extra data buffers.
Evidence: `audio-aux-stereo-pass-test/{result,build-verification}.json`.
Build with `build_audio_reverb_span.py console-replay --revision UNIQUE`.

No new long emulator boot was repeated for this exact arithmetic replacement.
Previousv4 runtime evidence covers only unchanged surrounding behavior; no new
end-to-end timing, physical FPS, complete fidelity or normal application
acceptance is claimed. This remains an experimental, undeployed replay. Old
physical9.925FPS and the30FPS goal are unchanged. Redundant per-segment control
publication and remaining audio transition integration still need work; retained
software reverb cannot be dropped while software voices and their tails use it.

## v4 sampled CPU-work comparison (15 October)

The CPU-reference and native-DSP runs of the same v4 executable both completed
with normal log drain, unchanged saves and owned-process exit. The isolated
Old Fast route, input/save/assets, emulator and firmware match. Captures1000
were inspected: textured scene, Fox and HUD remain; animation poses differ.
Elapsed218.07/226.42hostsec is excluded: instrumentation runs outside the active
window too, and unrelated emulator processes were present.

Window1041..1048 has35,248,151 ->34,246,914 ARM instruction encounters (-2.84%).
Resampling plus mixing drops7,257,804 ->1,092,691, but native control submission,
callback servicing and AUX work consume much of that saving. The original
HandleReverbChannel still executes1,082,016 encounters in both modes, alongside
517,064 encounters in the native kernel. Retained software voices/state must be
understood before removing that work.

The strict comparison script rejects the frame denominator:15 versus16 present
entries, not its expected8. Both windows contain8 particle-pool updates and24
source audio cycles, while rendering/collision/voice histories differ. Its gate
is unchanged; audio-arm-diagnostic.json explicitly records a rejected matched
comparison and raw-window counts. Counts include condition failures and are not
hardware cycles. At most this suggests a small net gain; it provides no evidence
for20-30FPS from the9.925FPS console baseline. No deployment or audio-fidelity
acceptance. Both processes are terminal; do not repeat unchanged boots to obtain
a more favorable number.

## Buffered audio-generation handoff (15 October, experimental)

The native DSP owner previously cancelled a future-generation START while older
PCM was still reaching output: it treated every epoch mismatch as stale. It now
keeps future commands queued, holds live timed voices when output provenance is
unknown, and retires older voices only after playback advances. Explicit aborts
still bypass deadlines; sample leases still require the sync-matched DSP sentinel
acknowledgement. A leading silence prefix for a new generation no longer reports
an old generation's retiring voice as a prefix fault. Immutable voice descriptors
are read only after acquiring a queued START or while that voice is live.

The game callback now retains its last observed playback generation through
unmapped gaps. The producer's generation can already be ahead, so using it during
those gaps could prematurely retire buffered voices. This adds one4-byte callback
field, with no new buffer, allocation, worker or wait. Pinned Melee's cache and
NDSP-join resource lifetime rules were reviewed; those mechanisms are unchanged.

`test_audio_dsp_epoch.py` executes actual native queue/retirement code and pinned
libctru channel operations on ARM11 with synthetic DSP replies. It passes3385
checks with four command slots and3889 with the game's256 slots. The archived v3
owner fails the future-generation regression; eleven injected faults also fail,
including premature future rejection and incorrect prefix attribution. These are
serialized SDK protocol checks, not firmware/concurrent-memory validation.
`test_audio_game_epoch.py` executes the exact game callback body under ASan/UBSan:
3997 assertions pass; the old producer-generation gap behavior fails.

Short silent LLE check `audio-native-voice-device/20261015-051528` completes in
17.85hostsec and cleans up its owned process. Binary1577d04d7d0f, private firmware
identity retained. Original source and native DSP tones share a capture with a
37-source-sample prefix:420 native segments, CPU resampling420 ->0, no underruns,
no clipping or unexpected silent gaps, one source-end notification on each side.
Native onset differs by-0.214ms and end by+3.086ms; envelope RMS error0.01913,
steady level ratio0.96730. Full envelope plot inspected. This checks the existing
delayed-start path after the queue change, NOT a live multi-generation game reset,
complete audio fidelity or physical performance. The analyzer keeps accepted=false
for that broader scope.

Producer reset admission, two audible generations inside one DSP block, live AUX
parameter/route changes, depop/other audio fidelity and total CPU benefit remain
open. In particular, the mixed-generation block still fails explicitly; the game
reset panic has not been removed prematurely. No console deployment or FPS claim.

Isolated `audio-cost-v4-console-replay-builds` is complete/restored:
source8a48b956e621,3dsxd9fd4f546814,CIA6ed682cedad5,ELF94df2152f676.
Only the DSP voice header/owner and game callback owner differ from v3; the tested
source hashes bind to the archived build (including its accounting transform).
Original game archive and18 GPU programs are unchanged, Old80/New124 verified.
Allocated ELF sections differ only by168 additional text bytes; the4-byte callback
field does not increase their overall BSS size. No new game runtime/FPS claim.
Evidence: `audio-game-epoch-test/{result,build-verification}.json` and
`audio-dsp-epoch-game-test/result.json`.

The test-runner output-directory replacement initially failed to match the full
path, so both SDK runs wrote to `audio-dsp-voice-test`. The latest3889-check,
256-slot report and binaries were verified and copied byte-identically to
`audio-dsp-epoch-game-test`; the runner now asserts and replaces the actual suffix.
The earlier3385-check/four-slot result is retained in tool output, not claimed as
a separate surviving result.json. This bookkeeping error did not affect executed
tests or production code. No emulator boot was repeated to repair the record.

## Contiguous reverb rings (15 October, local experimental replay)

The source STD reverb kernel now processes contiguous spans of its existing
delay rings. It retains the original floating-point operation order, coefficients,
predelay, all-pass/comb filters and sample conversion. Ring positions are updated
at span boundaries rather than for every sample. Very short predelays advance
separately, so a one-sample delay does not force one-sample spans. No effect is
removed; no heap allocation, worker, buffer or GPU work is added. The compiled
kernel's own stack frame is112 bytes, not a measurement of total SDK stack use.
Pinned Melee audio cache/SDK-join lifetime handling was reviewed and is unchanged.

Host ASan/UBSan comparison against the original variable-partition kernel passes
14,211,315 samples /61,440 calls, including zero/tiny/full partitions, predelays
0/1/32/1056/3200 samples, noise and decay tails, with complete filter/delay state
equality. Wrap, state and arithmetic fault mutants are rejected. The first wrap
mutant produced a zero-length infinite span and hit the45s test bound; the revised
wrong-offset mutant fails immediately. This was a mutant harness failure, not a
candidate runtime failure. Strict ARM compilation passes without suppressing
unused-function warnings; the replaced scalar helper definitions are removed.

ARM11 execution of both kernels passes50 synthetic cases. Instruction encounters
fall1,551,210 ->585,332 across that mixture; normal157/160-frame blocks with no,
1056- or3200-sample predelay fall833,154 ->290,640 (-65.12%). Short one-sample
calls retain span-setup overhead; these counts are not a captured game workload,
hardware cycles or FPS. The first span-only implementation regressed the legal
one-sample predelay; the separate short-delay cursor removes that regression.

Isolated `audio-cost-v3-console-replay-builds` is built/restored:
sourcef97cba508f08,3dsx27551e3a7fb1,CIAd41006eedb9c,ELFbe7f3a03e8ee.
Exactly one generated source differs from v2: `platform_audio_game_reverb.c`.
The original game archive and all18 GPU programs are unchanged; shared Old80/
New124 profiles pass. The actual linked ELF kernel also passes60 ARM11 cases
(including156-frame blocks), with complete output/state equality to the original.
Evidence: `audio-reverb-span-test/{result,build-verification}.json` and
`audio-reverb-span-test/arm/result.json`. Build command:
`python3 tools/build_audio_reverb_span.py console-replay --revision v3`.

This optimizes the experimental native AUX owner only. The original CPU fallback
reverb is unchanged. There is no normal-app pair or deployment, and no new live
game/physical performance acceptance. Prior v2 gameplay evidence is reused only
for unchanged surrounding code, not presented as a v3 runtime. Source epochs,
live AUX changes and complete audio fidelity still block production promotion.

The attempted same-binary CPU-reference ARM profile
`old-fast-audio-work-v2-reference` timed out after305.54hostsec at HID retrace916,
before its1041..1048 window. No ARM profile or screenshot was captured. The owned
process exited, timing is excluded, and seven foreign SMG emulators were observed
and left untouched. Do not repeat that unchanged boot or claim a combined CPU
win from it. Next gate is exclusive/captured CPU accounting when feasible, plus
the remaining native audio transition/fidelity work; physical Old Fast remains
9.925FPS, with30FPS unmet.

## Source-mixing and SDK accounting (15 October, experimental)

The v1 paired runs finished: `old-fast-audio-cost-v1-{reference,native}`,
same sourceb099a63c5d79/3dsx34e80e619e67/CIA7e687310a8b9 and private firmware.
Both pass normal shutdown, save preservation,123 loaded models/0 rejects and
owned cleanup. Captures1200 and full audio envelopes inspected; neither is a
source-matched fidelity acceptance. Their RAM observations are identical:
normal42230800/50331648, linear minimum free2695040/12582912, VRAM free4723712,
process70430720/83886080. Native admits499 voices/231430 segments with28
pre-admission CPU fallbacks and zero source/callback faults.

Reference producer1232.92us/block, SDK44.63us/callback. Native producer780.88us,
SDK527.21us, including AUX423.70us, PCM/native callback70.95us and publication
23.84us. Summing only producer plus inclusive SDK and normalizing by produced
audio gives258.93 ->302.72 elapsed milliseconds per produced second (+16.91%).
Thus the producer saving alone does NOT establish a beneficial replacement.
These include preemption, startup and different voice histories. SDK work can
overlap the producer's elapsed interval when it preempts that producer, so the
sum and its17% increase are NOT an exclusive CPU-time regression. Foreign host
jobs/audio capture invalidate host-throughput/FPS claims. They also exclude
producer-side movie/PCM copying, queue publication and SD reads outside
render_mix; this is source-mixing plus SDK accounting, not total audio CPU usage.

The largest new cost is the native AUX owner. The next candidate removes the
surround reverb/rate work discarded by the immutable original DPL2 stereo route.
Original reverb channels use separate delay/filter state. Both stereo-only and
full-surround adapters pass10485760 sample comparisons against the archived
adapter and actual source kernel, with matching retained channel state. The
independent original reverb test still passes384000 samples/2154 partitions;
4096-block rate tests and strict ARM compile pass. No delay storage is removed,
no effect disabled, and no memory/worker added.

v2 sourcea5a86523133d/3dsx0365d05d90c0/CIA0b67624127fb builds with only four
audio-source differences from v1, unchanged game archive/GPU programs and
restored root/archive. The600-tick run exceeded its300s bound (305.92s observed,
lastHID1213, empty buffered log, no capture); owned cleanup and save hashes pass,
but that run is not runtime acceptance. Five foreign emulators were observed.

The focused180-tick movement check `old-fast-audio-cost-v2-micro` completes in
283.05hostsec at1164, status1/logsdrained/savesunchanged/ownedexit. Capture1000
shows Fox/textured ThornTail entrance/HUD;35.79s audio envelope inspected,
zero clipped samples. Source-fidelity acceptance remains separate. Native358
voices/153271 segments,7 pre-admission CPU fallbacks, zero source/callback faults.
122 models load without rejection; the shorter route does not load v1's123rd.

Observed AUX316.31us/callback versus v1's423.70us (-25.35%); inclusive SDK408.37
versus527.21us. Producer691.54us/block. The new summed elapsed metric is253.54ms
per produced second, but do not compare it to the reference258.93 as a net win:
the route/startup fraction/voice history differ and cross-owner elapsed intervals
can overlap under preemption. LLE/capture/foreign host work also exclude an FPS
claim. A bounded instruction-work or exclusive-CPU comparison is the next gate.
Normal peak42230816/50331648, linear minimum free2708736/12582912, VRAM free4723712,
process70430720/83886080; audio worker stack upper4096/98304. No deploy or physical
speedup claim. All owned runs/builds/tests finished; no replay remains running.

`build_audio_cost.py console-replay --revision v1` creates one shared Old80/New124
executable containing both the native DSP route and a CPU reference. The latter
is selected only by the exact diagnostic fixture marker
`replays/cases/audio-reference.txt` containing `FH_AUDIO_CPU_REFERENCE_V1` plus
a newline. It leaves all voices on the original CPU mixer. Both modes retain
native sample preparation, the same pool, effect arena reservation, SDK and PCM
transport, so this compares steady voice delivery rather than allocation savings.
This selector is confined to the isolated experiment, not normal installations.

The two serialized audio owners now report accumulated source-producer and SDK
active elapsed ticks after worker/SDK join. SDK state handling, PCM/native
callback, AUX processing and publication are separate child buckets; only
producer plus inclusive SDK active may be combined. DSP IRQ waiting is excluded.
Ticks use the SDK ARM11 clock and include preemption; their sum is not CPU
utilization and must not be called physical CPU cycles. Counter storage is144
normal bytes plus one SDK-local timestamp; no audio buffer or thread is added.

`test_audio_cost.py` executes the actual patched SDK loop and cost owner under
ASan/UBSan with scripted IRQ, sleep, cancellation and not-ready paths. Original
and instrumented submission event order matches; five million synthetic wait
ticks are excluded. A missing-callback-charge mutation fails the accounting
check. Hardware scheduling/cache behavior is outside this test's scope.

`run_audio_cost.py` runs the same short ThornTail route with either mode and
private DSP firmware, retaining normal renderer/saves/exit gates. Optional
instruction profiling is separate from audio capture. `analyze_audio_cost.py`
checks executable, firmware, emulator, graphics and model-admission identities,
normalizes by produced audio, and refuses to turn the elapsed ratio into an FPS
claim. The first pair is recorded above; no console deployment. Keep the v1
unproven performance case as the baseline for the stereo-return repair.

## v10 integration result (15 October)

`old-fast-game-dsp-v10-old` completes the short ThornTail route and normal
shutdown at retrace1539, with saves unchanged and owned emulator exit. Source
03ee953dd983, binary595eff517b74. All123 native models load, none rejected:
allocating only the used96KiB effect arena restores the two v9 rejected models
without weakening the memory reserve. 519 native voices/238539 segments,
33 pre-admission software fallbacks, zero source/callback faults.

Normal peak42230800/50331648 bytes; linear minimum free2695040/12582912;
VRAM free4723712; process70430720/83886080. The1200 capture and full captured
audio envelope were inspected. Textured world/Fox/HUD are present;42.80s audio
contains no clipped samples, but is not a source-fidelity comparison.

The observer started late, recording52 rows and three continuous pause/resume
boundaries with no anomalies. This does **not** establish a whole-run reduction
from v9's224 pauses. Its shutdown heap/read failures are retained. The211.29s
host duration is excluded from performance comparison because of LLE audio
capture, observer overhead and concurrent foreign emulators. No deployment,
normal-build promotion, overall FPS gain or complete audio acceptance follows.
Next account for SDK callback/AUX cost as well as producer cost with matching
audio emulation modes; do not translate1.49->0.90ms producer readings into FPS.

## Native audio queue failure and coordinated refill (15 October)

**v9 game outcome:** source79931026bea5 /3dsx22697f0100f0 /CIA05721af8c7c0
reaches ThornTail, executes the short movement route and shuts down at1514.
511 native voices,228,980 segments,17 pre-admission software fallbacks and zero
source/callback faults. All GPU jobs complete and saves remain identical.
The inspected1200 capture contains Fox, textured terrain/vegetation and HUD.
However, overall acceptance correctly FAILS:13 renderer coverage faults(site36),
with models698/699 rejected during loading. Parent admitted both. Normal RAM
was at its85% admission boundary; the prototype's unnecessary second96KiB
effect arena crowds model admission. The following candidate allocates only
the effect buses actually used; it does not loosen rendering/memory gates.

v9 measurements: normal peak42,252,456/50,331,648; linear free minimum2,763,136/
12,582,912; VRAM free4,723,712; process70,430,720/83,886,080; main stack73,044/
524,288. Native metadata485,804 normal bytes, effect arenas196,608 normal bytes,
sample/silence1,050,624 linear bytes. These are this short route's observations,
not worst-case/endurance bounds. The captured42.68s audio has no clipped samples;
waveform/RMS plot inspected, not source-matched quality acceptance.

The observer captured974 rows and224 continuous refill handoffs with zero
mapping anomalies, but215 handoffs occurred after loading. Those frequent
short pauses are NOT acceptable as a finished audio backend. The revised gate
uses four actual PCM packets of refill headroom (2,560 input frames here),
matching the existing startup queue's margin, instead of four DSP blocks
(about628 input frames). It adds no buffer or discarded samples. The revised
host375,997/SDK1,453 checks pass. Private-firmware starvation fixture040208,
binary5bcf2235cc5f, resumes both channels after127.11ms with zero underruns/clips;
end difference0.3055ms, envelope error0.01614. Actual game validation is pending.

The game integration now admits actual DPL2 voices. v6 admitted154 voices and
61,483 control segments before failing on its bounded control queue. Its raw
producer readings around0.90ms versus the parent's1.49ms exclude callback/AUX
work, use different audio emulation modes and voice phases, and ran alongside
foreign host jobs. They are NOT a40% gameplay speedup or console FPS estimate.

The v8 run exceeded its185.78hostsec deadline at retrace993 without flushing the
buffered native log. Its emulator exited; do not restart it as a supposedly live
job. A subsequent run, `old-fast-game-dsp-v8-observed-old`, captured310 bounded
journal records through a read-only observer and reproduced the queue failure
at retrace819. It admitted196 voices/77,484 segments. Saves remain identical.

The concrete defect is a prediction/queue race, not DSP dropped frames. At SDK
tick4445 the upcoming window clipped to queued PCM frame453760 and invalidated
the prediction. New PCM arrived before the returned hardware state reported
an empty queue. Hardware playback continued to456263 at tick4464, with the same
underrun count7 and dropped count0, while the native clock remained invalid at
453760. Voice commands stopped draining. `native-audio-analysis.json` records
both observations; no guessed emulator-to-console multiplier is involved.

The new experimental output guard co-pauses PCM and native voices before the
queued tail, retaining the input position and fractional phase. It leaves two
DSP blocks available before holding and resumes with four blocks buffered.
Explicit terminal drains bypass the guard. Only the native synth channel opts
in before callback registration; ordinary streaming PCM retains its behavior.
This prevents the observed clipping race; unknown dropped-clock recovery,
source epoch resets, live effect changes and complete audio fidelity remain
separate requirements. No commands/samples are discarded and the ring remains
256 entries. No new thread, waveform resampling or linear buffer is added.

`audio-output-gate-test` passes375,997 source-window/phase checks under ASan and
UBSan, including the captured boundary and50,000 bursty producer steps. The
actual SDK ARM harness passes1,453 checks. Six host mutants and three SDK
mutants reject, including missing coordinated pause. An initial all-channel
guard changed unrelated PCM behavior; its SDK test failed at line91. The guard
is now explicitly opt-in, and the full suite passes. This failed design was
not deployed.

Private-firmware paired starvation fixture `audio-native-voice-device/20261015-034701`
uses binary8274789896a3. Deliberately withholding producer data creates a shared
122.2195ms pause in original-left/native-right output. Both resume; no underrun
or clipping, source end notifications1/1, CPU resample420->0. Onset differs
-0.0917ms and end0.3055ms; envelope error0.01621, RMS ratio0.96691. The complete
envelope plot was inspected. This validates one fixed-pitch dry voice, not AUX
tail pause equivalence, arbitrary sample fidelity or physical scheduling.
The earlier034503 fixture had the same waveform result before opt-in scoping.

The observer checks the exact archived ELF, two executable anchors, process
identity and normal heap; each32-row journal snapshot is read twice. It writes
no guest state, pauses nothing, and invalidates throughput comparisons. v8's
journal predates the strengthened ARM odd-marker acquire/release ordering;
double-reading guards its observed records. The next build includes that
diagnostic ordering fix. No new console build or hardware run is requested.

## Actual game DSP integration and DPL2 correction (15 October)

`build_audio_game_dsp.py` now connects the original source voice/control hooks,
sample registration lifetimes, mapped DMA output window, native DSP voice owner
and separately owned reverb to an isolated **game replay**, based on the
audio-timeline-v1 archive. This is an experimental backend, not a deployable
audio replacement. The original game archive and all GPU programs remain
unchanged; the replacement is in the platform/MusyX support library. Shared
Old80/New124 packaging is retained. No console files or saves were changed.

The first real-game v4 run reached asset loading at retrace907, then correctly
failed the renderer's existing15% linear reserve: only1,859,200 of12,582,912
linear bytes remained when an8KiB HUD image was needed. The prototype's2MiB
sample arena was too large for this workload. The following candidate uses a
bounded1MiB arena, retaining software playback for cache/admission misses;
the renderer reserve and texture fidelity are not weakened. Native metadata,
two96KiB effect arenas and2KiB immutable DSP silence also remain budgeted.
The reverb constructor uses those bounded normal-memory arenas and no game
effect pointers escape to the SDK callback. Effect parameters/identities,
source resets and command backpressure still need complete transition handling:
unsupported live changes fail the experiment explicitly. They are not hidden
by resuming stale software sample histories.

**Correction to the earlier voice snapshot interpretation:** captured studio
type1 is `SND_STUDIO_TYPE_DPL2`; STD is0. All eight retained Old Fast observations
use DPL2, with `sndAuxCallbackReverbSTD` on AUX A and no AUX B callback. The effect
name does not imply a STD studio. `audit_audio_game_routes.py` verifies the
snapshot ELF hash, resolves the actual callback symbol and checks the original
enum declaration. Evidence: `audio-game-route-audit/result.json`. Consequently,
the prior STD-only admission would offload none of these game voices. The v5
memory adjustment alone was not rerun as a performance experiment.

Native mode4 implements the original two-channel DPL2 routing: main S is not
mixed, AUX B L/R feed the direct stereo output, AUX B effects and B-surround are
ignored, and AUX A's returned S is discarded. STD mode3 retains its previous
three-channel effect/downmix behavior. The routing selector occupies former
curve padding; commands remain80B, the256-command DSP owner459,104B and source
adapter6,488B. No extra sample copies or CPU waveform loop were added.

`audio-native-dpl2-test/result.json` executes the original sample gain loop,
`mixStudioOutput` and `foldStereoToOutput`, plus the real SDK channel state.
Native123,113 checks and STD/DPL2 source-state tests pass; DPL2 with either
present or absent B handler passes16,524 checks each. The lost-DPL2-route mutant
and existing control/identity/send/lookahead mutants reject. These are source
arithmetic/ABI checks, not physical audio or FPS measurements.

Private-firmware LLE capture `audio-native-voice-device/20261015-031150` adds
nonzero main S, AUX A S and direct B sends. Original/native source cycles match,
CPU resampling calls420->0, source end notifications1/1, no underruns or clips.
The full tail is captured and the envelope plot inspected. Dry onset differs
-0.0611ms; tail ends5.0416ms later; normalized envelope error0.01919 and steady
RMS ratio1.09193. These residual differences remain explicit; this is not full
fidelity acceptance. Earlier STD capture023754 has ratio1.15846, not unity.
The short023445 capture censored both tails; its apparent zero end difference
must not be used as end-time evidence.

The headless game runner now has `--dsp-lle` and verifies both persisted config
and the emulator's actual LLE log. v4's96.53hostsec failure retained saves and
cleaned up its owned process. Its timing is excluded for audio capture and
concurrent foreign emulator work. No relative gameplay speedup or console FPS
estimate is established. Further game runs must demonstrate nonzero native
admission, normal completion, remaining RAM headroom and usable captured output
before a matched performance comparison is meaningful.

## Native AUX routing and original reverb (15 October)

The new experimental DSP handoff now carries effects buses rather than only
dry voices. This is needed because the captured game workload has12–23 voices
but only3–8 qualify for the earlier dry-only route. **The game still uses its
full CPU mixer; this work is not yet a game speedup.**

The installed libctru2.7.0 stores public AUX callbacks but never invokes them.
Its actual ARM thread object and the versioned upstream source agree on that
missing dispatch. `audio_aux_sdk_patch.py` inserts one call at the SDK's explicit
TODO, after its sleep/cancel/readiness guards and master-state publication, before
channel publication/semaphore. It generates a separate unit; the installed SDK
archive stays untouched. An initial global channel-update-wrapper design was
discarded before positive runtime testing because that function also runs during
wake initialization. The scoped hook does not run there.

`audio_dsp_aux.c` snapshots the SDK-mapped PCM32 effect bus, invokes an exclusively
owned callback and publishes its return. No fixed DSP addresses, new linear
buffer, per-callback allocation or CPU sample decoding. Both bus and returned/
submitted bank selection are checked against the real SDK enable state:
165,002 ARM checks pass, and wrong-bank, wrong-bus and disabled-callback mutants
fail. These tests simulate shared RAM and locks; they are not sleep/wake or
physical shared-memory coherence acceptance. Pinned Melee's publication and
`ndspExit` lifetime rules were consulted; callbacks/contexts survive `ndspExit`.

`audio_aux_rate.c` bridges the actual SOC/512 DSP clock to32kHz and back, retaining
fractional phase and two processed history samples. The original STD reverb
kernel accepts156/157 samples per DSP block without changing its32kHz parameters
or sample arithmetic. An independently compiled, unmodified original callback
agrees exactly across384,000 samples,2,154 partitions, four configurations,
pre-delay wrap and final filter/delay state. ASan/UBSan also passes4,096 rate
blocks/640,772 source samples plus signed/extreme-value and surround checks.
Linear interpolation remains an approximation; the host ramp errors are at
most1.88 input and2 output sample units, not a general frequency-response bound.

One bus adds2,588 bytes of SDK-handoff state plus2,000 rate-adapter bytes
(4,588 normal bytes), excluding the existing reverb's delay allocation. Two
buses share the handoff scratch and require a second2,000-byte rate owner.
No new thread or linear allocation. ARM compilation verifies the SDK clock.

Private-firmware LLE results, with captured waveforms/envelopes inspected:

- `audio-aux-device/20261015-021214`: stock negative control, zero public AUX
  calls and silent intended RIGHT return. Unprocessed AUX LEFT contaminates the
  reference LEFT, so this run is **excluded from audio-quality comparisons**;
  it does not show that the firmware mixer itself is silent.
- `20261015-021352`: actual source reverb via bus0,248 callbacks, no drops/faults,
  RMS ratio0.998858 and latency-aligned waveform error0.021653. **Fails the
  original <1ms onset gate**: native starts4.827597ms later. The failure is retained.
- `20261015-021614`: bus1 raw handoff with no rate/effect processing,248 callbacks,
  no drops/faults, RMS ratio0.999932, onset4.797043ms later. This is a routing/
  latency measurement only. Two160-sample DSP blocks versus one160-sample source
  cycle predicts4.777412ms, within one DSP sample of the raw observation.

This establishes a firmware transport delay, not permission to hide it by
aligning captures. `audio-aux-analysis/result.json` retains both absolute onset
and separately aligned waveform metrics. The inspected plot shows the reverb
tail retained; it is one tone/configuration, not general game audio parity.
The captured rate source85e816f9e634 differs from currentca88f0b4bc38 only by an
ARM compile-time SDK-clock assertion. Current host/strict ARM checks cover that
guard. No repeated firmware capture for an assertion-only change.

Next: connect source gain/send curves and controlled effect-state ownership,
account for transport latency and source pause/reset epochs, then measure the
actual game's CPU reduction. ITD, depop, studio graph/mixed clipping and dynamic
parameter lifetimes remain open. Do not re-run the same dry-tone checks or treat
an unused backend as30FPS progress on hardware. Latest game99c62 and installed
console builds remain unchanged; no deployment or new hardware request.

## Native starts within a DSP block (15 October)

The experimental voice owner can now start a new voice inside an upcoming
32 kHz source region. It submits an immutable zero-sample prefix before the
prepared sample and loop descriptors. Prefix length uses the initial source
curve's pitch; future pitch events stay queued. A missing initial curve waits
for publication rather than guessing from the muted START command. Capacity
failure retains the command and sample lease for explicit cancellation.

The prefix shares the existing permanently retained silence allocation. An
optional 2,048-byte allocation covers high-pitch prefixes, compared with the
old 320 bytes: **+1,728 linear bytes**. It is cleared/flushed before publication
and retained through `ndspExit`, following the pinned Melee/SDK publication and
shutdown lifetime rules. Each voice gains a separate delay descriptor/history;
total owner metadata increases **720 bytes**, to 15,056 bytes with four commands
per channel (458,576 bytes with the experimental256-command configuration).
There is no new thread or per-frame allocation/PCM waveform generation.

The firmware-level simultaneous CPU-left/native-right test now reproduces
480 and640 leading unknown PCM frames, the two delays found in the real DMA
pipeline. These require 11 and15 native prefix samples respectively. Both
captures complete with one prefix, no prefix faults, zero underruns, one
source-end notification per phase, and420 CPU resampler calls reduced to zero
in the native phase. At480 frames, onset difference is -0.2444 ms and end
difference +0.0306 ms, normalized envelope RMS error0.02018. At640 frames with
a shared53.7766 ms pause, onset difference is -0.2139 ms and end difference
-0.0611 ms, error0.01854. The latter full capture was inspected. Native steady
RMS remains about3–4% below the CPU reference; this is not full audio parity.

The actual SDK ARM owner passes3,317 checks and nine faulty variants fail.
New cases cover a future start inside a block, initial48 kHz pitch, distinct
prefix/sample descriptors, insufficient retained silence at192 kHz, and
cancellation without early sample release. The first-curve publication guard
was added after firmware capture and tested separately against the SDK. In
the captured cases all source events are published before PCM READY, so that
guard takes the unchanged branch. The verifier reconstructs both captured
source versions and records the difference, rather than claiming the newest
binary itself was captured.

Evidence: `audio-subblock-test/result.json` and saved source versions;
`audio-native-voice-device/20261015-015046` (480) and`015315` (640+pause);
`audio-dsp-voice-test/result.json`. First fixture015016 rejected its own
reference phase's generation11 before playback; this test bug was corrected
and its failure retained. A SDK-only link initially lacked `ceilf`; bounded
positive rounding now avoids that dependency. An obsolete deadline-mutant
replacement was also corrected after the command-loop refactor.

Still unimplemented: resuming an already active voice across an interior
source gap, general pitch/short-envelope fidelity, source depop tails,
AUX/reverb/ITD/studio routing, drop recovery and game integration. The new
region API explicitly faults/holds an unsupported active-voice interior gap;
it must not be installed as a complete audio backend. No game build, deployment
or new physical FPS result was produced. Continue with effects-bus routing and
actual game integration; do not repeat the same dry-tone proof.

## Upcoming mapped audio blocks and pause/resume (15 October)

`audio_output_clock.c` now tracks submitted DSP blocks, retaining fractional PCM
input frames across pauses. It uses the SDK submission counter and rational
output rate, not wall time. Duplicate callbacks do not advance it; a dropped
frame, missed callback, unexplained rate/epoch change or exhausted input makes
the prediction unavailable until an explicit paired start. That invalidates
metadata only: it does not drop software PCM, invent a successful recovery or
authorize native voice admission.

`audio_output_window_patch.py` extends the already mapped PCM owner to publish
the upcoming block's source spans. The walker handles ring order, packet
boundaries, leading silence, unknown/movie-only spans and source-generation
changes. It retains unknown spans rather than treating them as synth time.
Incomplete/overlapping/unrepresentably fragmented coverage is rejected. No
producer-owned pointer escapes the callback. Queue storage grows from 2,216
to 2,472 bytes: **256 normal bytes, zero linear bytes**, no worker or additional
cache flush. Original PCM copying, SDK queue submission and retirement fences
are unchanged. This is an isolated source patch, not a new game package.

Host ASan/UBSan passes 325,840 checks, including long fractional accumulation,
submission counter wrap, pauses, resets, dropped frames and absolute source
mapping across differently ordered slots. Four mutants fail. The actual
installed SDK ARM owner passes 1,438 checks; stale-window reset and stuck-clock
mutants fail. This does not prove hardware feedback/quantization or concurrent
weak-memory behavior.

The native source/DSP fixture now uses that **actual mapped PCM owner**, with
source generations distinct from PCM queue epochs. In private-firmware LLE,
the paired CPU-left/native-right capture includes a real shared 53.7766 ms
pause. Both channels resume together; onset difference remains -0.0917 ms and
end difference +0.3055 ms. Full-envelope normalized RMS error is 0.01733,
steady native/source RMS is 0.96624, no clipping or underruns. CPU resampler
calls remain **420 to zero** and both source-end notifications occur. The full
capture plot was inspected, including the pause and final release. Source
cycles are 176 versus 168 because the second phase pauses; this is not lost
simulation or a throughput comparison.

Evidence: `audio-output-window-test/{result,firmware-verification}.json`,
`audio-output-window-sdk-test/result.json`, and
`audio-native-voice-device/20261015-013925/{summary,paired-analysis}.json` plus
`paired-inspection.png`. The newer analysis includes the full release after
the pause; the original summary's fixed 460 ms analysis window ended early.
The original capture and summary are retained. The verifier checks exact
compiled queue/header/source identities without relaunching the emulator.

Still missing: native starts or gaps inside a DSP block (the game begins with
480/640 unmapped PCM frames), recovery after dropped/underrun blocks, feedback
validation, general pitch/envelope families, AUX/reverb/ITD/depop routing and
actual game admission. The fixture consumes one contiguous known source span;
it does **not** claim to handle every span exposed by the new metadata API.
Do not install a diagnostic that merely pauses through unhandled transitions.
No new game build or hardware request was warranted by this component change.
Physical Old Fast remains 9.925 FPS, with 30 FPS still required.

## DSP source curves and firmware execution (15 October)

The next audio prototype keeps original volume and ADSR endpoints in a compact
command payload, evaluates their product at the upcoming DSP block boundary,
and asks the DSP to ramp its gain. Future gain curves can be inspected without
consuming their pitch/pause events. This eliminates the previous assumption
that sending an endpoint at a returned playback cursor reproduces the original
envelope. Curve commands fit inside the previous 60-byte controls payload;
retained per-voice curve state adds 1,064 bytes to the owner. With 256 commands
per channel the owner is 457,856 bytes, plus the unchanged 6,456-byte source
adapter. The default four-command owner is 14,336 bytes. No additional linear
sample storage or worker is added by the curve implementation.

This is still **experimental and not connected to gameplay**. Mode 2 of the
source adapter selects curves; mode 1 retains the endpoint-only comparison.
The callback API explicitly requires the upcoming block's source interval.
The fixture derives that interval from paired submission and actual DSP block
callbacks, using the SDK's exact `SYSCLOCK_SOC/512` output rate. This proves
only uninterrupted playback from a paired start, not resets, underruns, future
voice starts, DMA/movie handoffs or general pitch changes. Those cases must be
integrated with the game's provenance before enabling this backend.

The previous near-exact steady tone was **HLE evidence**, not firmware evidence.
The runner now also executes the existing private DSP firmware through Azahar
LLE, checks the saved configuration and runtime LLE log, and records firmware
and actual emulator hashes. It does not distribute the firmware. The verified
custom executable is `21de0e6dbecf`; the earlier stock AppImage executable is
`9ead802cdbcd`. A LLE steady-tone comparison shows about 3.33% lower native RMS,
despite matching frequency and no clipping. Do not reuse the HLE 0.04% figure
as a firmware claim. Carrier/interpolation differences remain open.

The DSP format's bit 5 is documented as Fade in the
[3dbrew DSP memory description](https://3dbrew.org/wiki/DSP_Memory_Region).
The pinned SDK has no public named constant for it. Actual private-firmware
execution was therefore used to check it; inspected emulator source alone was
not treated as proof. Pinned Melee audio publication/flush/exit lifetimes and
the installed SDK's callback ordering were reviewed; these owners remain in
place. No raw DSP-memory ownership bypass was introduced.

For the original 40 ms attack, 20 ms decay and 20 ms release fixture, the old
endpoint path has normalized envelope RMS error 0.09256 after independent
onset alignment. Enabling Fade alone gives 0.08466. The new curve/window path
gives 0.01820. The final comparison submits the independent original waveform
on the left channel and native playback on the right **in the same capture**:
onset difference -0.0917 ms, final audible sample difference +0.3055 ms,
normalized envelope RMS error 0.01766, steady RMS ratio 0.96735, no clipping.
Its plot was inspected. Both phases run 176 source cycles; native control
handles 420 active segments with **420 to zero CPU resampler calls**, one
sample-end notification on each path, and zero underruns. The fixture's
reference waveform storage is test-only, not a proposed game allocation.

These measurements do not accept all envelopes: short attacks, discontinuous
gain/pitch changes, source depop tails, source polyphase coefficient families,
AUX/reverb/ITD/studio routing and mixed clipping remain unresolved. In
particular a DSP-block ramp cannot reproduce every arbitrary 1 ms source
curve. The analysis records `accepted=false` for full audio parity. No physical
timing, game FPS gain or complete native audio claim follows from this test.

The actual original gain loop and installed SDK ARM test passes 24,801 checks,
including signed moving volume/envelope products, future gain lookahead that
does not consume future pitch/pause, retirement and unsupported-route handling.
Four mutants reject lost controls, lost AUX routing, endpoint-only envelope
evaluation and disabled gain lookahead.
The existing four-command DSP owner passes 3,290 checks and seven failure
mutants. No new sanitizer or weak-memory concurrency proof is claimed.

Evidence: `audio-native-voice-device/20261015-011432` (LLE steady), `011756`
(endpoint envelope), `012038` (Fade only), `012551` (curves), and `012746`
(simultaneous paired capture), all under the same `20261015-` directory prefix.
The ARM reports are `audio-native-voice-test/result.json` and
`audio-dsp-voice-test/result.json`. Reproduce the final capture with
`test_audio_native_voice_device.py --backend lle --envelope --curves --paired`.
Inspect without another run using `analyze_audio_native_voice.py RUN --paired`.

No game build, conversion, FTP transfer, save/preference change or new hardware
request was made. Latest local game replay remains `99c62cd16efc`, with its
software synthesis still running. Old Fast's physical baseline remains
**9.925 FPS**; the target remains **30 FPS**, with lighting/ENV/specular on.

## Prepared material publication

`material-packet-v1-console-replay-builds` compiles four immutable TEV stage
packets per resident material. Its publisher keeps the SDK context and copied
raster cache coherent, including foreign SDK writes and context restoration.
It adds 12,312 bytes of static CPU storage, no linear/VRAM allocation, shader,
pass, geometry or worker. The pinned Melee command-cache ownership approach and
the actual SDK register layout informed this implementation.

The actual ARM publisher passes 47,431 checks across 1,800 transitions. Four
incorrect-state mutants fail. The synthetic colour-heavy workload reduces
command words from 33,728 to 20,832; that is **not representative of gameplay**.
The Old Fast ThornTail prefix records only 2,498 colour-only publications versus
105,306 full publications. The synthetic percentage must not be called an FPS
gain. Candidate native submission is 3.238 ms versus 3.207 ms in the reused local
baseline, with different draw counts (177.15 versus 171.66 per frame). No net
gain is established. Keep this candidate unpromoted.

The runtime completes normally, preserves saves, drains GPU work and exits.
Capture 1200 was inspected: Fox, ThornTail geometry/textures, HUD and particles
remain visible. This is not full image parity. Heap peak is 42,132,416/50,331,648
bytes, minimum linear free 3,746,304/12,582,912, VRAM free 4,723,712, measured
stack 73,044/524,288 and process 69,914,624/83,886,080. The process reservation is
16 KiB larger after alignment. Evidence: `material-packet-test/` and
`artifacts/native/old-fast-material-packet-v2-old/`.

The first runtime attempt failed storage preflight before launching. Identical
immutable archived game libraries were hardlinked after full hash comparison;
all paths and bytes remain available. No assets, saves or captures were deleted.

## Shared native shader bank

The next candidate combines ordinary actors, ambient actors, UI models, clouds,
layered actors, diffuse maps, height fog and specular maps in one DVLP. Identical
skinning/lighting helper bodies are shared. Eight original entries total 830
instructions in the offline intermediate; coalescing identical helpers leaves
495 instructions, with 17 of the PICA's 512 slots spare. The oversized
intermediate is never sent to the GPU.

The live material owner holds the shared allocation. Scene instances borrow it
while holding the actor entry, and release borrowed descriptors before releasing
that ownership. Specular/diffuse/height-fog owners retain their existing
acquire/release lifecycle. Uniform constants, boolean masks, output tables,
fragment/raster restoration, resource fences and texture publication remain
entry-specific. Sharing code does not authorize retaining incompatible state.
The layered entry adds a counted helper call and retirement NOP; existing
ordinary/ambient/UI/cloud entries already used these helpers.

This follows the pinned Melee renderer's shared-DVLP ownership and explicit
program/ABI checks. It extends the approach to Foxhollow's native material
families; it does not reconnect GX interpretation or CPU geometry rebuilding.
Both the CPU-side linker and final runtime enforce the hardware code limit.
The linker ignores only unused second-source descriptor bits for unary FLR/MOV
when identifying identical helpers; all meaningful operands remain checked.

All eight entries pass 65,536 original-versus-linked float24 vertex comparisons
each, with exact output lanes. Invalid and missing helper-call mutants fail.
This is numerical/control-flow evidence, not physical raster or timing proof.
The isolated shared-bank replay build is source `7e3688585208`, 3dsx
`3a45e41f41c6`, CIA `8f304b2cc60c`. All 686 source records match the expected
eight-file change, the game archive is unchanged, Old80/New124 remain, and the
linked bank bytes exactly match the float24 test. The old ambient and specular
blobs disappear; all other embedded shader blobs except the material bank match
the parent. Root and game archive restoration passed.

The Old Fast ThornTail prefix completed in 79.86 host seconds, with saves
unchanged, logs drained and owned emulator exit. Capture1200 was inspected:
Fox, textured world/vegetation, HUD and particles remain visible. The measured
window contains 418 frames at 172.85 draws/frame, versus the baseline's 420 at
171.66. Native submission is 3.207 ms in both; presentation publication changes
6.352 to 6.306 ms. **No end-to-end gain is established.** This is emulator
evidence and does not establish physical performance.

Against the earlier material candidate's identical shortened input prefix,
whole-run VSH upload requests fall 8,003 to 4,507, and program words fall
4,366,100 to 3,771,614. Those runs have different animation/frame counts and the
material candidate also changes TEV publication. These are supporting traffic
observations, not an isolated timing comparison. Particle transitions still
change banks, and returning to the larger bank offsets some upload savings.
Do not claim savings from fewer banks alone. Keep the candidate unpromoted.

Heap peak is 42,132,104/50,331,648 bytes, linear minimum free
3,746,304/12,582,912, VRAM free 4,723,712, measured stack 73,044/524,288 and
process 69,894,144/83,886,080. There is no new linear/VRAM allocation or worker.
Evidence is in `hot-bank-test/` and `artifacts/native/old-fast-hot-bank-v1-old/`.

The material-packet, resident-texture and Old audio-core experiments are **not**
included in the shared-bank candidate. Its parent is the accepted
`actor-fog-retention-v1` snapshot; unrelated root work is excluded.

## Load-time bone palette scheduling (15 October)

The next candidate prepares persistent bone slot assignments at asset admission.
It permutes palette order and the corresponding two compact vertex slot bytes;
positions, UVs, weights, bone identities, triangles, materials and draw order
remain unchanged. Two deterministic schedules favor ordinary or fur continuity.
A schedule is accepted only when neither static publication count increases.
All derived plans are compiled afterward, before the existing initial GPU flush.
There is no per-frame scheduler, new shader, pass, worker or resident GPU buffer.
Temporary scratch is capped at 11,264 bytes; this inventory needs at most 1,364.
Two diagnostic counters add eight static CPU bytes.

Across 835 models, 75 models/216 groups change. Static ordinary bone publications
fall 6,027 to 4,801 and fur publications 320 to 237. These are unweighted asset
counts, excluding context resets, not predicted frame savings. The actual C
reader/compiler/scheduler passed ASan/UBSan with all 313,746 vertices checked;
bone identities and all unrelated bytes remain exact. Two slot/publication
fault mutants fail, and ARM11 compilation passes. Pinned Melee publication and
uniform invalidation rules remain in force.

Replay source `ed7a1aa3efd4`, 3dsx `81f9a3841f43`, CIA `a400041e07ab` is
**unpromoted and not deployed**. Its parent is accepted actor-fog-retention-v1,
without the shader-bank, material-packet, sampler or audio-core experiments.
The isolated build verifies exactly three changed files, 686 source records,
all 18 GPU programs and the game archive unchanged, Old80/New124 profiles, and
root/live-archive restoration.

The Old Fast ThornTail prefix passes in 70.13 host seconds, with unchanged
saves, drained logs and owned exit. Capture1200 was inspected: Fox, world
textures, vegetation, HUD and particles remain visible. Thirteen loaded models
and 29 groups use the schedule. The matching simulation window contains 422
frames at 171.53 draws/frame versus baseline 420 at 171.66. Emulator native
submission changes 3.207 to 3.193 ms, object rendering 2.421 to 2.370 ms and
publication 6.352 to 6.341 ms. **No end-to-end or physical speedup is established.**
These small differences do not close the gap to 30 FPS; timing buckets overlap.
Whole-run traffic totals have different durations and must not be compared as
an isolated saving. The static layout improvement is real, its timing benefit
has not been established.

Sampled heap is 42,132,416/50,331,648 bytes, linear free minimum
3,746,304/12,582,912, VRAM free 4,723,712, stack 73,044/524,288 and process
69,902,336/83,886,080. No normal build, New run, conversion, deployment or
hardware request was made. Evidence: `palette-schedule-test/{audit,native,
build-verification,runtime}.json`; runtime is reproduced by
`tools/analyze_palette_schedule.py`. Continue with the larger audio and CPU
render-preparation costs rather than treating this as the required 3x gain.

## Sparse audio stop-tail processing (15 October)

The native mixer previously consumed every stop-tail ring entry in all eleven
channels, even when no voice had published a tail. The isolated sparse-tail
candidate keeps a 32-bit occupied-region mask for each of eight studios and
eleven channels (352 bytes). Producers mark the existing ring after writing;
consumers skip only regions proven zero. Partial consumption retains its bit,
and global/studio reset clears the corresponding mask with the ring. Dormant
studios, wrapped offsets, overlapping tails and voice reuse retain their state.
There is no new sample buffer, worker, lock, DSP service call or audio-quality
change. The pinned Melee CPU mixer and NDSP ownership were consulted; existing
synth locking, output publication and cache coherence remain unchanged.

The actual adapted producer/drain passes 330,000 channel comparisons against
the original tail equation under ASan/UBSan. Missing publication and premature
partial-region retirement mutants fail. A synthetic ARM11 exercise produces
identical hashes and changes 23,906,953 to 21,153,648 instruction encounters.
Those numbers include the test harness and are not a measured gameplay saving.

The existing completed eight-frame Old profile was attributed to its exact
DWARF5-identified mixer source (MD5 `98d134734131c477a0f920df31888a78`). The old
drain loop accounts for 423,192 of 40,915,804 instruction encounters, or 1.0343%.
This bounds its importance in that window: it cannot provide the required 3x
improvement. Resampling remains the larger audio cost, spread across input
decoding, history, interpolation and control flow. The parent full profiling
run timed out; only the completed bounded window is accepted, not its runtime.

Replay source `73bfa8c70fa1`, 3dsx `958541f8c6ce`, CIA `61bd93d2b06e` is
**unpromoted/not deployed**. Exactly the new header and archived adapter change;
686 source records, unchanged game archive and all 18 GPU programs, Old80/New124
and restored root/live archive pass. Linked DWARF verifies the exact tested
mixer and header; the linked dirty-state object is 352 bytes. This candidate
does not combine the other unpromoted CPU experiments.

Old Fast ThornTail completes in 94.46 host seconds with state checks, unchanged
saves, drained logs and owned exit. Capture1200 was inspected and retains Fox,
world textures/vegetation, HUD and particles. The audio worker remains on core0.
No fresh listening or whole-stream PCM comparison was performed. Concurrent
foreign Azahar runs and a build make timing comparisons invalid; they were left
untouched. `host_timing_context.py` now detects competing emulators as well as
compilers. No FPS gain is claimed.

Sampled heap 42,132,416/50,331,648 bytes; linear free 3,746,304/12,582,912;
VRAM free 4,723,712; stack 73,044/524,288; process 69,902,336/83,886,080.
There was no normal build, New run, conversion, console transfer or probe request.
Evidence and reproducible analyzers live in `audio-sparse-tail-test/` and
`tools/{test_audio_sparse_tail,analyze_audio_tail_profile,verify_audio_sparse_tail,
analyze_audio_sparse_runtime}.py`. The larger performance gap remains open.

## Decoded ADPCM block cache: integration rejected (15 October)

A separate prototype tested avoiding repeated static-sample decoding across
voices and loops. It uses 64 content-keyed entries (3,072 bytes plus 12 bytes
of counters), covering encoded block bytes, predictor coefficients, initial
history and scale. Entries fill only as samples are consumed; there is no
speculative decoding. Virtual and streaming samples remain on the original
reader. Each span belongs to one synchronous resample call under the existing
synth owner; the cache does not retain source pointers between calls.

The ARM test ran the actual accepted resampler adapter (source SHA-256
`1debecbc71b154a2b94c6633cfde7c33c641e3c9f1fb2af07fe8f6a81e763b78`),
changing only its span helper. Across 43,200 samples, six formats, three SRC
modes, five pitches and mutable loops, sample/state rolling checksums match.
This is a checksum comparison, not exhaustive byte comparison or live audio
acceptance. Despite 892 hits in 920 lookups (96.96%) and 11,106 cached samples,
instruction encounters increase from 6,075,594 to 7,043,443 (15.93%). Hash lookup,
additional branches and compiler register allocation offset the saved decoding.
These are synthetic ARM instructions, not hardware timings.

The prototype fails the integration gate and is **not connected to the app**.
No game build, repeated emulator boot, deployment or hardware request follows
this failed check. The root experimental header and benchmark wrapper remain
for reproducibility; existing isolated builders exclude the unused header.
Evidence: `artifacts/audio-block-cache-arm-v1/{summary,acceptance}.json`, frozen
`tested-block-cache.h`, complete generated C and ARM executables. Reproduce
with `tools/benchmark_audio_block_cache.py --block-experiment --label <new-label>`.
Do not infer a gameplay speedup from the high cache hit rate.

The renderer review also confirmed that persistent world/model plans, prepared
buffer layouts and packed draw commands are already present in the accepted
snapshot. Recreating those wrappers would not constitute an architectural gain.
The 30 FPS target remains open; avoid repeating this rejected cache design or
the previous small packet experiments without a materially different cost model.

## Retained resource tables (15 October)

The isolated candidate retains the original seven merged resource tables until
their inputs change. Its key includes output and both source addresses. Engine
read submission, endian conversion, table allocation/unload/relocation and reset
revoke the entries. Pending DVD work prevents both reuse and publication, even
when a worker has finished writing but its callback/replay gate has not retired.
All cache access is on the engine thread; workers never touch it. The original
merge equations and output storage remain unchanged. No source pointers are
dereferenced by the cache or held as ownership references. It adds 92 static
CPU bytes, with no allocation, GPU work, worker or asset format change.

Pinned Melee `file_io.c` publication/async-read ownership was consulted. The
adaptation keeps Foxhollow's existing asynchronous reader and callback ownership;
invalidation runs before submission rather than adding writes from an I/O worker.
The source audit locates merged-output writes in `pi_dolphin.c`; all source-table
mutation entry points and native DVD reads are conservatively invalidated. Any
new in-place table writer must join this invalidation contract.

The actual original seven merge branches pass 72,000 complete output-array
comparisons under ASan/UBSan. Missing invalidation, accepting pending reads and
ignoring source replacement mutants fail. Original null-pointer walking is not
sanitizer-executed; null cache keys are tested separately. The test initially
caught the adapter's early-return admission gate excluding the two target units;
that was fixed before the passing run and build.

Replay source `406e478f550b`, 3dsx `9c11a79f295e`, CIA `5ad4c0afac71` is
**unpromoted and not deployed**. The accepted actor-fog-retention-v1 parent
excludes other experimental CPU changes. Exactly four source records change,
687 records are verified, and only `pi_dolphin.o` changes in the game archive.
DWARF verifies the tested loader, endian conversion, DVD owner and cache source.
All 18 GPU programs remain unchanged; Old80/New124 and root/live-archive
restoration pass. Do not blind-build unrelated root work.

Old Fast ThornTail completes in 90.49 host seconds with state assertions, saves
unchanged, drained logs and owned exit. Capture1200 was inspected: Fox, textured
world/vegetation, HUD and particles retained. It avoids 4,124 of 4,185 merges
(98.54%). This is a reduction in this operation, **not an FPS gain**. The previous
completed Old instruction window attributes only 1.81% of encounters to table
merging, not cycles. Concurrent foreign emulators exclude timing comparisons;
no new physical speedup is established and the 30 FPS target remains unmet.

Sampled heap 42,132,416/50,331,648 bytes, linear minimum free
3,746,304/12,582,912, VRAM free 4,723,712, stack 73,044/524,288, process
69,898,240/83,886,080. No normal/New/conversion boot or console probe was needed.
All assets, preferences and saves remain unchanged. Two identical immutable
archived libraries now share storage after full SHA-256 checks; every path and
byte is retained (`archive-storage.json`). Evidence: `resource-tables-test/` and
`artifacts/native/old-fast-resource-tables-v1-old/`. Reproduce with the
`test_resource_tables`, `build_resource_tables`, `verify_resource_tables`,
`run_old_fast_profile --resource-tables --short --scene thorntail`, and
`analyze_resource_tables` tools. Keep the larger CPU preparation/submission and
audio budget as the priority; this cache does not close that gap.

## Exact rolling resampler window (15 October)

Old Fast remains the priority. This candidate replaces repeated history-ring
address calculations with a local packed four-sample window. It still writes the
original history, consumes the same samples at the same time, uses the original
32-bit filter coefficients and 64-bit accumulation, and preserves all decoder,
loop, virtual-source and voice state. It adds no resident storage or worker;
the window is eight local bytes. This is a limited cost reduction, not a route
to the required threefold whole-game improvement by itself.

The actual resampler passes 30,000 calls / 779,868 output samples with complete
output-buffer, DSP voice and resampler-state byte comparisons. Coverage includes
six source formats, three interpolation modes, variable and zero pitch/length,
mutable loop bytes and history-index wrap. ASan/UBSan pass; the shift sanitizer
is excluded because the original decoder uses signed left shifts. Reversed-shift
and missing-update mutants fail. The separate ARM11 fixture uses 43,200 samples
and reports 6,075,594 -> 5,912,483 instruction encounters (-2.68469%), with matching
sample/state checksums. That is synthetic instruction cost, not physical cycles
or an FPS estimate. Pinned Melee audio publication/cache ownership was consulted;
the existing SDK buffers and fences remain unchanged.

Replay source `5d22c6c42c79`, 3dsx `f72f6c591b26`, CIA `b79a375d3c14` is
**unpromoted and not deployed**. It changes only the mixer adapter against the
accepted actor-fog-retention archive, not the other experimental candidates.
All 685 source records, 18 GPU programs, unchanged game archive, Old80/New124,
root restoration and exact linked mixer identity are checked. No normal build,
New runtime, conversion boot or console probe was required.

Old Fast ThornTail completes in 129.09 host seconds with state/save/drain/exit
checks passing. Capture1200 was inspected: Fox, textured world, vegetation, HUD
and touch controls remain. Timing comparisons are excluded: this run includes
a read-only voice observer and silent DSP capture alongside foreign emulators.
Sampled heap 42,132,416/50,331,648 bytes, linear free 3,746,304/12,582,912,
VRAM free 4,723,712, stack 73,044/524,288, process 69,898,240/83,886,080.
Captured output contains 39.17 seconds at 32,728 Hz, no clipped samples and
approximately -27.1 dBFS RMS. Its waveform and spectrogram were inspected;
there are silent intervals as well as active output. The capture includes
loading/menu, and does not establish uninterrupted audio or original-game
audio parity. The exact resampler comparison is the stronger local correctness
evidence. No physical gain is established; the last measured baseline is still
9.925 FPS.

### Larger audio replacement: observed workload, not an admission claim

The physical log reports recent 160-sample/5-ms mix blocks taking about 3.5 ms
elapsed on the shared Old core. That includes preemption; it is not 70% CPU
utilization. Eight coherent, read-only runtime snapshots contain 141 active
voice observations, all static ADPCM, using linear or polyphase interpolation.
There are 12..23 active voices per snapshot in one master studio with AUX A;
its captured handler address `0x3e2c90` resolves in the exact linked ELF to
`sndAuxCallbackReverbSTD`.
Only 3..8 meet necessary dry/no-ITD conditions. These are observations, not
unique voices, time-weighted costs or a correct DSP admission policy.

The SDK's 24 ADPCM-capable channels do not alone solve this workload. A larger
DSP replacement must preserve AUX effects, envelope and voice timing, filter
semantics, sample-rate behavior, channel reclamation and resource lifetime.
Routing only dry voices is incomplete; dropping AUX work is not an optimization
accepted by this task. Full AUX-capable offload is not ruled out, but has not
been implemented. A decoded-sample residency design would also need an explicit
bounded memory/admission contract; the previously rejected per-block hash cache
must not be reintroduced as if it were a successful result.

Reproduce with `test_audio_window.py`, `benchmark_audio_window.py`,
`build_audio_window.py`, `verify_audio_window.py`,
`run_old_fast_profile.py --audio-window --scene thorntail --short`, and
`analyze_audio_window.py`. Evidence is in `audio-window-test/`,
`artifacts/audio-window-arm-v1/`, and
`artifacts/native/old-fast-audio-window-v1-old/`. The observer verifies the exact
ELF, DWARF layouts, owned process/heap mapping and double-read state stability;
it never writes guest memory, pauses the game or changes inputs. No deployment,
private asset, save or preference changed.

## Native DSP sample preparation (15 October)

The next architectural candidate moves immutable sample preparation out of the
frame loop. `audio_sample_plan.c` / `fh_audio_sample_plan.h` now compile static
GC-ADPCM into bounded, caller-owned playback ranges. This is **unconnected**:
there is no new app build, DSP scheduler, runtime allocation or claimed FPS gain.
It does not ship any game-derived sample data. Source banks remain private and
unchanged; conversion runs over the supplied bytes.

The compiler preserves the first traversal and the loop's independent predictor
history. A loop starting inside a 14-sample ADPCM block cannot be represented by
merely pointing NDSP at that block. Such samples get exact separate PCM ranges,
with a 32-byte aligned loop start. Frame-aligned loops and ordinary one-shots
retain compressed bytes when their initial/loop predictor headers agree. The
compressed loop range shares the immutable first-traversal storage and retains
its separate history. Streaming/virtual sources are outside this interface;
no-extra-data loops with carried rather than reset history must also be excluded
by a future admission adapter. Descriptor format IDs are local enums, not SDK
format values. Publication, cache cleaning, pins and completion fences are the
future owner's responsibility; no borrowed source pointer survives compilation.

All 1,472 samples in the two actual banks are static ADPCM. Original-reader
comparisons cover 41,512,064 sample outputs and repeated loops, plus 4,096
synthetic cases / 2,724,107 outputs covering arbitrary predictor/header/history
values and block boundaries. The mixed representation retains compressed data
for 1,174 samples and uses PCM for 298 unaligned loops. Full-bank PCM would use
63,031,686 bytes; the mixed inventory still uses 34,298,090 bytes. **Neither is
an acceptable resident allocation.** Runtime residency must be bounded and
admitted at sample/bank load, not rebuild or hash sample data every frame.

The eight existing read-only snapshots have matching length/loop metadata in
this catalog. Assuming separate storage for every live voice, the maximum
metadata-derived bound is 964,160 bytes, including 32-byte allocation rounding.
This is not a proof of peak memory: it excludes queued/retiring generations,
fragmentation, descriptors and transition preloads, and metadata alone does not
prove sample identity. `residency.json` explicitly retains these limitations.
The current sampled Old linear free budget was 3,746,304 bytes before this work;
no new pool has been allocated. A proposed pool must leave explicit headroom
and preserve complete audio when capacity is exceeded.

Pinned Melee audio range cleaning, wave-buffer retirement and queued publication
were reviewed, along with the installed SDK channel/AUX interfaces. The captured
studio uses `sndAuxCallbackReverbSTD`; replacing decoder/resampler/mixing work
must retain that effect and voice/ADSR/stop timing. The local Azahar source
`audio_core/hle/source.cpp` maps polyphase to linear interpolation, and its ADPCM
codec allocates/decodes a second sample for odd-length input. These are emulator
comparison limitations to resolve explicitly, not reasons to silently change
the game's sample counts or claim physical fidelity from an HLE run.

Validation status is deliberately split. PCM-only v1 passed strict ARM11 compile
and ASan/UBSan differential checks (`pcm-only-v1-result.json`). The expanded
compiler and first three incorrect-state mutants completed Linux sanitizer
checks. C: then reached zero free space, WSL became read-only, and compilation
of the fourth mutant failed. All four mutants plus the exact native compiler
subsequently passed Windows GCC checks without sanitizers: incorrect loop
history, rounded loop start, ignored initial predictor and lost DSP loop history
all fail sample comparisons. Expanded ARM compilation remains **unverified**.
`result.json` does not reuse the earlier ARM result for the changed source.

Space recovery replaced 17 duplicate immutable archive copies with hardlinks
after complete SHA-256 verification. Every path and byte remains available;
no live archive, executable, asset, save, preference or other project changed.
Never modify these archived libraries in place. WSL subsequently reported all
distributions stopped and Ubuntu startup error6/E_FAIL; no distro was terminated
or reset by this work. Windows testing remained usable, so this is not a full
goal blocker. All owned test/startup processes ended.

Evidence: `audio-sample-plan-test/{result,residency,archive-storage*,windows/}`.
Reproduce with `tools/test_audio_sample_plan.py` (Linux sanitizer/ARM) or
`--windows` (host GCC only), then `tools/audit_audio_dsp_residency.py`.
Next: bounded resource ownership and actual DSP voice scheduling/AUX integration,
with independent audio/timing comparisons. The 9.925 FPS physical baseline and
30 FPS Old Fast target are unchanged; lighting/ENV/specular remain enabled.

## Bounded DSP sample ownership (15 October)

`audio_sample_pool.c` / `fh_audio_sample_pool.h` now own prepared sample extents
within a caller-provided, 32-byte-aligned arena. The module is still unconnected
to gameplay. It allocates no arena itself; the maximum accepted capacity is
2 MiB, not a chosen Old 3DS budget. Actual Old/New budgets require the runtime
memory gate. ARM metadata is 8,760 bytes (64 sample records, 64 immutable bank
incarnations and 128 independent playback leases). Samples compile once on an
admission miss; retained bank/sample keys do not rehash or revalidate payloads.
These keys require the bank adapter to maintain immutable incarnations.

Preparation and publication are separate. A failed synchronous range-flush
callback leaves the entry unavailable for playback. Published samples receive
independent, generation-checked leases. Closing a bank prevents new acquisition
but retains storage pinned by queued/playing/retiring voices. Only unpinned
extents are evictable; first-fit allocation never compacts or moves live bytes.
Duplicate releases and stale bank/sample/lease generations are rejected. A
shutdown with outstanding leases returns BUSY and retains the arena. Serial
identity survives drained shutdown/reinitialization. The caller must serialize
all operations and release leases only after authoritative DSP completion;
there is no hidden thread, cache operation or device fence in this module.

The test executes the real sample compiler and pool through 25,000 operations:
4,847 compilations, 2,014 evictions, 9,976 capacity refusals and 41,456 complete
held-payload byte comparisons. Peak occupancy is 32,576/32,768 test bytes.
Additional cases fill all 64 bank and 128 lease slots, retain data through failed
shutdowns, fail publication, reload a bank with changed bytes, release leases
twice, and reuse the pool after shutdown. Evict-pinned, close-pinned,
publish-after-failed-flush and reset-generation mutants all fail. The final
suite passes ASan/UBSan and strict ARM11 compilation. Earlier Windows checks
used hash comparisons and a less varied random sequence; the final Linux suite
uses full byte copies and upper RNG bits. No timing/FPS inference comes from
this synthetic lifecycle test.

WSL started successfully on this turn after the earlier space recovery; no
distro reset or foreign-process termination was issued. The expanded sample
compiler also now passes its entire Linux sanitizer suite, all four mutants and
ARM compile. The earlier partial failure is preserved as
`audio-sample-plan-test/partial-wsl-failure-result.json`; the current `result.json`
supersedes its pending status. Source compiler identities and sample counts are
unchanged. All owned test processes completed.

The installed SDK's `ndsp-channel.o` was also inspected directly, retaining its
archive/object SHA and disassembly in `audio-dsp-retirement-audit/`. In
`ndspChnWaveBufClear`, the CPU immediately writes DONE to every queued wave buffer,
clears cached sequence/playing fields, increments its local synchronization
counter and marks configuration dirty. `ndspChnIsPlaying` and
`ndspChnGetWaveBufSeq` read those cached fields. Therefore clear + DONE, false
playing, or sequence zero is **not a DSP retirement acknowledgement**. A timeout
is not one either. The future voice adapter must observe a genuine DSP response
before returning a lease; this inspection is not yet a completed device fence.
Pinned Melee's queue/cache/whole-engine shutdown pattern remains the integration
reference, without assuming its whole-engine exit is a per-voice fence.

Evidence: `audio-sample-pool-test/linux/result.json`,
`audio-sample-plan-test/result.json`, `audio-dsp-retirement-audit/`.
Reproduce with `tools/test_audio_sample_pool.py`, `tools/test_audio_sample_plan.py`
and `tools/inspect_ndsp_retirement.py`. There is no new game executable, emulator
boot, allocation in gameplay, hardware request or deployment. Physical Old Fast
remains 9.925 FPS. Actual DSP scheduling/AUX/reverb/voice timing integration and
the remaining CPU-rendering reductions still stand between this work and 30 FPS.

## DSP cancellation mailbox and acknowledgement (15 October)

`audio_dsp_retire.c` / `fh_audio_dsp_retire.h` implement the next unconnected
piece of the DSP voice owner. One producer, serialized with the sample pool,
requests cancellation through a per-channel acquire/release mailbox. The NDSP
frame callback consumes requests, clears the channel and queues a permanently
retained, looping silent buffer. Only when the SDK's synchronization-checked
status reader returns that buffer's sequence does the callback publish an
acknowledgement. The producer drains it and releases the old sample lease.
The callback performs no pool, allocation, file I/O or application-lock work;
SDK channel locking remains. No timeout, guessed frame delay or unacknowledged
stop releases data. Channels 0/1 are excluded for existing stream/software audio;
the candidate owns only 2..23. This is cancellation, not a depop/envelope
implementation or a complete voice scheduler.

The installed SDK exposed two distinctions that the adapter must preserve:

* Clear immediately marks old buffers DONE, clears cached playing/sequence and
  increments local sync. DONE or false playing is not device retirement.
* `ndspiUpdateChn` can mark a newly submitted buffer PLAYING before a DSP reply.
  PLAYING alone is also insufficient. The returned nonzero sequence is required.

The first test incorrectly expected QUEUED after command publication; the real
SDK ARM execution rejected that expectation. Its source/driver are preserved as
`first-status-assumption.c` and `first-status-driver.c`. The corrected test
explicitly demonstrates early PLAYING with cached sequence zero, and rejects
release at that point. This did not require a console probe.

There is also a threading requirement: the SDK compares sync and writes its
cached sequence before taking the channel lock. Therefore a clear from an
unrelated audio/game thread could race an in-progress status read. The new
mailbox puts all channel mutations and acknowledgement observation in NDSP's own
callback. The installed `ndspThreadMain` disassembly places this callback after
`ndspiReadChnState` and before `ndspiUpdateChn`. Producer-side pool operations
never run in that callback. The integration must preserve this ownership;
calling the service function from another worker would invalidate the argument.
This replaces the earlier prototype's direct callback-to-pool operations.

The standalone ARM11 test links the actual installed `ndsp-channel.o`, with its
exact SHA pinned. Only its private state symbol is globalized in the **test copy**;
no SDK or application archive changes. Locks, virtual-to-physical translation
and DSP replies are simulated. It runs 2,041 checks over 88 retirements, all 22
channels, both SDK shared buffers, 16-bit sync wrap, stale replies with a matching
sequence, early PLAYING, invalid aliases, failed initial flush, and 22 concurrent
retirements sharing one sample. Producer drain is required even after callback
acknowledgement. Release-on-request and PLAYING-only mutants fail. Strict ARM
compilation passes. This validates the serialized CPU protocol, **not concurrent
stress, firmware/DMA/cache-coherence behavior or a physical device fence**.

ARM owner state is 904 bytes; the shared mono silence requires 320 linear bytes.
Both must remain alive until `ndspExit` returns, even after every old sample lease
has drained: clearing/pausing the silence is not a fence for its own data.
No new memory has been allocated in the running game. No executable, asset,
preference or save was changed, and no emulator boot or deployment was needed.
The initial compile caught the SDK's mutable mix-array signature; the test link
also needed the real `ndspFrameId` symbol. Both were corrected before passing.

Evidence: `audio-dsp-retire-test/result.json`, the frozen SDK objects and
`audio-dsp-retirement-audit/{identity,thread-disassembly,channel-disassembly}`.
Reproduce with `tools/test_audio_dsp_retire.py`. Next integrate actual voice
submission/control and the callback mailbox with bank ownership, while preserving
ADSR/depop, AUX/reverb, source timing and audio capture. No FPS improvement is
established; Old Fast remains physically measured at 9.925 FPS with a 30 FPS target.

## Accounting constraints

The material candidate's whole-run command traffic includes approximately
21.84 MB of uniform-row payload and 17.46 MB of shader-program publication out
of 81.46 MB of command bytes. These are traffic counters, not elapsed CPU/GPU
time. They motivate reducing repeated publication; they do not predict 30 FPS.
Old audio also consumes substantial CPU work. The core-1 experiment remains
unpromoted: scheduling/quota and shared synth ownership prevent assuming that
moving its worker removes that cost.

Use the exact Fast mask 2113537, effects zero, matching save/input prefix and
100% emulated CPU clock. Compare work counts alongside timing. Keep physical
measurements separate. Full parity, sustained 30 FPS Old Fast, New performance
targets, progression/save/endurance validation remain incomplete.

## Native DSP voice playback and controls (15 October)

Old Fast remains the priority: **30 FPS / 33.33 ms**, against the unchanged
physical **9.925 FPS** baseline. Lighting, environment lighting and specular
remain enabled. This work targets an active Old Fast CPU cost, not an optional
visual effect. It does not establish a game speedup or replace the renderer work
still needed to close the full gap.

`audio_dsp_voice.c` now consumes the prepared sample pool through channels 2..23.
It submits an immutable first range and optional looping range, sets native
ADPCM coefficients and independent loop histories, and handles rate, all twelve
mix gains, interpolation, pause/resume and cancellation. Channels 0/1 remain
reserved for existing stream/software output. A four-entry SPSC queue per
channel preserves command order; a full queue returns BUSY without dropping a
command. The caller must retain and retry it. Requests contain generational voice
IDs, so a retired voice cannot control a reused channel. Queue indices survive
wrap and are never reset while NDSP is live.

The producer owns sample acquisition and immutable descriptors; SDK mutations
run only in the NDSP frame callback. A newly acquired lease is ARMED without
cancelling playback. An ordered STOP or natural end begins the existing silent
sentinel acknowledgement protocol. DONE never releases a sample directly.
Callback work has no decoding, pool access, allocation, file I/O or application
lock. The final callback operation publishes retirement acknowledgements; it
must not subsequently read descriptors the producer may reuse. The owner and
silence remain alive until `ndspExit` returns, following the pinned Melee global
shutdown/publication rules. The SDK internal locks remain intact.

The actual installed SDK ARM test passes **3,229 checks**, all 22 channels,
ADPCM/PCM, first and loop histories, both shared buffers, natural endings,
queue-full backpressure, index wrap, controls, and stop-before-first-device-
publication. Three mutants fail: missing loop history, lost pause and treating
first-buffer completion as the end of a looping voice. The extended retirement
API also retains its **2,041-check / 88-retirement** regression and both rejected
unsafe-release mutants. Locks, address translation and DSP replies in these ARM
protocol tests are simulated, so this is not a physical firmware/DMA or weak-
memory concurrency proof.

An asset-free, silent Old-mode Azahar run then executed the actual owner and SDK
against HLE DSP. Compressed ADPCM on the left and the independently decoded PCM
loop on the right produced **identical 8,000-sample steady captures** at both
rates and after resume. Measured peaks were 2000.53 Hz and 998.22 Hz (FFT-bin
resolution), with RMS about 1468; pause and cancellation captures were all zero.
Both leases retired and the capture guard remained intact. The plotted waveforms
were inspected. This covers synthetic even-length loops with nonzero histories,
not game music, transient alignment, polyphase equivalence, source envelopes,
AUX/reverb, hardware precision or performance. The fixture is compile-time
forbidden outside emulator-test/HLE builds and is not a console package.

ARM voice-owner state is **10,616 normal bytes**, including retirement, plus
8,760 bytes for pool metadata and 320 linear bytes for shared silence. The sample
arena is separately bounded and has **not** been allocated in gameplay. No game
binary, accepted source snapshot, assets, saves, preferences or console files
changed. Accepted actor-fog-retention and installed builds remain unchanged.

The first device runner passed guest and waveform checks, then failed because
matplotlib was absent in the Linux container. That failed summary is preserved;
the same captures were analyzed and plotted on Windows without rerunning the
emulator. Analysis is now separate from optional plotting. The first ARM test
also incorrectly treated `ndspChnGetRate` as Hz: the pinned SDK implementation
returns the stored ratio to NDSP_SAMPLE_RATE. The test now verifies the setter's
physical-rate ratio. Strict compilation caught and resolved the float/double
comparison warning. These were test assumptions, not claimed gameplay fixes.

Evidence: `audio-dsp-voice-test/{result,validation}.json`,
`audio-dsp-retire-test/result.json`, and
`audio-dsp-voice-device/20261014-233231/{summary,audio-analysis}.json` plus PCM/WAV
captures and `waveforms.png`. Reproduce protocol checks with
`test_audio_dsp_voice.py`; `test_audio_dsp_voice_device.py` runs the short HLE
fixture. `analyze_audio_dsp_playback.py RUN --plot` reuses captured audio.

**Still unconnected to game playback.** The next integration must align the
native voices with buffered software/stream output, preserve MusyX's 1 ms
ADSR/control steps and source sample-end notifications, and handle AUX/reverb,
ITD/filter admission and immutable bank lifetimes. NDSP frame callbacks are about
4.89 ms, not the source's 1 ms control step. Do not feed each subframe into this
immediate queue and assume sample-accurate scheduling, discard controls when the
queue fills, or remove source mixing before that bridge is validated. DSP HLE
success is not hardware acceptance, and there is **no new FPS claim**.

## Preserve the source sample clock without decoding (15 October)

`audio_sample_clock.c` implements the source-control part needed for native DSP
voices. It advances the original unsigned Q16.16 pitch phase, source position,
ring-history index, end flag and interpolation tail in one bounded arithmetic
step per source segment. It does not read or decode samples. Static-loop wrap
happens before the next read, exactly as in the original: reaching position=end
is not itself a wrap or a sample-end notification. The first attempted read past
a nonlooping sample supplies the first zero/tail sample. NONE interpolation
consumes one source sample per output sample regardless of the supplied pitch.

The helper follows the original **32 kHz source clock**, not wall time, renderer
FPS or NDSP output progress. It accepts up to 160 output samples per call and up
to six-times pitch; NONE ignores pitch. Immutable bounds are validated at init.
No allocation, worker, cache clean, DSP command or game callback is added. State
is 28 bytes per admitted voice; it has not been allocated in gameplay.

The actual untouched original `hw_pc.c` private resampler and `hw_pc_decode.c`
serve as the oracle. ASan/UBSan passed **65,544 calls / 4,916,702 original output
samples**, including 2,730 looping cases, all three interpolation modes,
zero/variable pitch and segment length, exact boundaries and history-index wrap.
Cursor/phase/history index/end/tail and the sampleDone decision match exactly.
Four mutants reject early wrap/end, lost fractional phase and incorrect NONE
behavior. Strict ARM11 compilation passes. Original waveform calculation runs
in the test, but the candidate deliberately produces no waveform or ADPCM,
filter, interpolation or ITD history. This is source-clock evidence, not sound
quality or CPU-cycle/FPS evidence.

The game adapter still must preserve source ADSR handling, pitch selection,
subframe order and notifications, align commands with the existing buffered
software/movie output, and bridge the source's 1 ms control steps to the NDSP
frame clock. The clock is for permanently admitted native voices; do not switch
one back into software resampling with stale history. No game build or audio
work has been disabled. The actual source sampler admission and timed control
bridge remain outstanding, as do AUX/reverb and full playback validation.

Evidence: `audio-sample-clock-test/result.json` and its source/mutant logs.
Reproduce with `tools/test_audio_sample_clock.py`. The previous native playback
HLE capture is reused unchanged; no repeated emulator boot or hardware probe.
Old Fast remains physically **9.925 FPS**, with **30 FPS / 33.33 ms** targeted.
Accepted/installed builds, graphics presets, assets and saves are unchanged.

## Prepared samples follow source registration lifetimes (15 October)

`audio_sample_registry.c` and the pure `audio_sample_registry_patch.py` connect
prepared sample ownership to the original MusyX sample-loader functions in a
candidate transformation. They are **not yet installed into the accepted game
archive/generator**. No running mixer was bypassed and no app was deployed.

The source directory may survive individual sample unload/re-upload. Native
registration therefore follows each canonical `SDIR_DATA` record's live source
reference interval, not just directory lifetime or a reused sample address.
Successful upload prepares/publishes a bounded native sample. `dataGetSample`
reuses it, or prepares a cache miss from the still-live original input. A cache
hit performs neither conversion nor cache flushing. Dropping the final source
reference invalidates the registration **before** source bytes are freed; source
registry reset retires all registrations. Directory-table movement does not
change canonical record keys. Duplicate sample IDs follow the source's actual
active owner rather than the most recent uploading directory.

Each cache residency gets a fresh pool bank incarnation and uses sample ID zero.
This preserves the pool's immutable-input contract without changing that tested
owner. There are 64 registry slots, matching the bounded prepared-sample cache.
LRU eviction closes the old incarnation. Pinned native copies remain retained
until the existing voice acknowledgement releases them, even if the source
bytes are freed or reused. A current successful source lookup is required for
native admission; a stale SAMPLE_INFO saved across unload cannot be looked up
again as though it retained ownership. Active native voices retain their lease.

Stream/virtual samples and loops without explicit reset history are declined.
Publication failures and capacity misses leave original loading successful and
retain software audio; they do not omit voices. The timed/ADSR/AUX bridge must
succeed before a voice is removed from software mixing. All registry work runs
under the existing single producer/synth ownership; NDSP callbacks never access
this registry or original asset pointers. Pinned Melee publication/global-exit
rules and the existing pool/retirement implementation remain the basis for
native-buffer lifetime. No new SDK channel operation or allocator was added.

Actual transformed source registration/reference/lookup/reset code plus the
native compiler, pool and registry pass ASan/UBSan: **76 uploads/76 frees**,
137 preparations, 105 reuse hits, eight deliberate declines and 66 cache-slot
evictions. Tests cover same-address reload with different contents while an old
DSP lease remains pinned, duplicate references and IDs, directory compaction,
65 live sources, publication failure/retry, excluded sample classes, reset,
and the disabled optional bridge. Missing invalidation, invalidation after free,
and missing reset mutants all fail. Upload storage alone is substituted with a
fixed-slot allocator to force deterministic address reuse; physical DSP work is
not simulated by these loader tests. Prior DSP playback checks are reused.

Strict ARM compilation passes. Registry metadata is **4,144 bytes** (64 bytes
per slot); existing pool metadata remains 8,760 bytes. The fixture used an 8 KiB
arena and reached 8 KiB peak, measured before shutdown clears pool telemetry.
This is a bounded test allocation, not a chosen or measured gameplay arena.
The initial pin check compared normalized source text to its raw CRLF hash and
correctly refused before compilation; it now pins the normalized text hash while
records retain the raw original hash. The initial printed peak was incorrectly
read after shutdown reset; corrected before recording final results.

Evidence: `audio-sample-registry-test/result.json`, actual transformed `native.c`,
ARM objects and mutation logs. Reproduce with `test_audio_sample_registry.py`.
Original source hash 82d1a4e65fc8; candidate transformed source 5f85d4cc2c69;
native registry 62f8ed6a5a43. Accepted and installed game builds, source archives,
private assets, preferences and saves are unchanged. No emulator rerun or
hardware request. Old Fast is still measured at **9.925 FPS**, aiming for 30 FPS;
this loader integration does not establish a speedup. Next: timed admission and
control aligned with buffered output, preserving source ADSR/AUX/reverb and
notifications, then a real game build and measured workload comparison.

## Timestamped DSP commands and output holds (15 October)

The native voice owner now has timed start/control/stop APIs and
`fh_audio_dsp_voices_service_at(epoch, cursor, available)`. Commands carry
absolute deadlines in the caller's output timeline. Deadlines must be
nondecreasing per voice; immediate and timed controls cannot be mixed. Future
commands remain queued until due; full queues still return BUSY without dropping
commands. The source adapter must supply the actual buffered-output cursor and
retain commands on backpressure. The four-slot queue is unchanged and is **not**
a complete queue for the game's ahead-of-output 1 ms control stream.

Unavailable output holds active timed voices, then resumes their previous
caller-controlled pause state. An output epoch change cancels old voices and
future starts. Explicit abort bypasses future deadlines for reset/shutdown/error
handling, using the existing acknowledged retirement path. It is not an audible
ADSR release. Callback code does not allocate, decode, access the sample pool or
add file I/O. Immediate voice APIs retain their existing behavior.

Two cancellation interleavings are guarded: after acquiring abort, the callback
reloads queue head so a START published since its initial snapshot cannot survive
cancellation; producer poll does not allow channel reuse until a late abort has
been consumed, even if natural retirement had already acknowledged. One exact
producer-start/abort interleaving is injected into **test copies only**, between
the callback head snapshot and cancel acquire. This is targeted protocol evidence,
not general thread/weak-memory proof. SDK/production source is not instrumented.

The actual pinned SDK ARM checks pass **3,290 assertions**, retaining prior
playback coverage plus future deadlines, output holds, caller pause retention,
epoch resets, full-queue abort, late abort/reuse and the injected interleaving.
Seven mutants reject lost histories/pause, mistaken loop completion, ignored
deadline/epoch, stale abort head and premature reuse. The ignored-epoch mutant
initially failed strict compilation due to an unused parameter; that test copy
was corrected to compile, and it then failed its behavioral assertion. The
compiler failure was not counted as semantic rejection.

A short silent Old-mode Azahar run of the actual production owner completed in
9.93 host seconds. Synthetic cursor advances 160 units per available DSP callback
and freezes during holds. Both timed starts requested cursor **4017** and were
dispatched at **4160**: 143 units late each, total telemetry 286. This is deliberate
DSP-block quantization, **not sample-accurate scheduling or an audible A/V-offset
measurement**. The fixture cursor is not the game's 32kHz output cursor.
Compressed/PCM steady captures remain byte-identical at about 2kHz and 1kHz;
pre-start hold, user pause, output hold and cancellation captures are all zero.
Output hold/resume preserved both voices; both sample leases retired. Eight PCM
captures were analyzed and the waveforms inspected. No console probe requested.

ARM voice-owner state is now **13,272 bytes** (560 per channel), +2,656 bytes over
the previous owner. Existing shared silence remains 320 linear bytes; no new
sample storage or allocation was added to gameplay. No accepted game build,
archive, asset, save, preference or console file changed. The implementation is
still unconnected to the game. It must be driven from authoritative queued-output
metadata, then bridge source 1ms ADSR/pitch updates and AUX/reverb without merely
applying every subframe's last gain at the next callback and claiming parity.
Source sample-end/control behavior must remain intact.

Evidence: `audio-dsp-voice-test/{result,timed-validation}.json` and
`audio-dsp-voice-device/20261014-235750/`. Reproduce with
`test_audio_dsp_voice.py` and `test_audio_dsp_voice_device.py --timed`; captured
analysis/plots can be regenerated without another boot. Old Fast's physical
baseline remains **9.925 FPS** against its **30 FPS /33.33ms** target. No CPU-time
or FPS gain is established by this scheduling test.


## PCM output cursor and callback ownership (15 October)

`audio_pcm_queue.c` is a candidate replacement for the two existing platform
PCM queues. It is **not yet connected to platform_audio.c or accepted game
builds**. Producers copy and flush only each written range, then publish a READY
slot. Only the NDSP frame callback changes SDK channel state or descriptors.
The producer's I/O lock is not taken by the callback. Reset blocks new writes
until a permanent silent descriptor is acknowledged by returned sequence and
sync-aware SDK status; local DONE/PLAYING alone never releases PCM storage.
This preserves the earlier retirement rule rather than guessing a delay.

The cursor uses the actual returned buffer sequence and input-sample position,
plus each immutable slot's absolute origin and epoch. Initial/rebuffered starts
can publish their first-frame origin in the same SDK update as native voices.
Prefill spread across callbacks, pauses before first playback, partial finish,
underrun, repeated resets and epoch replacement are handled. The cursor is in
PCM input frames at the reported rate, not milliseconds, rendered frames or
source synth time. Game integration must still map the original two-buffer
synth delay and movie/rate transitions. Different rates require explicit
consumer timebase handling. No synthetic wall-clock advancement is used.

ARM owner size is **1,032 bytes**. PCM storage remains **131,072 linear bytes**;
reset acknowledgement adds **640 permanent linear bytes**, retained through
ndspExit. This does not enlarge stream buffering. These are candidate sizes,
not new allocations in the currently running game. No source voice is omitted
from the software mixer yet.

Actual pinned SDK ARM execution passes **1,342 assertions**. Five compiled
mutants reject premature reset acknowledgement, loss of a prefill origin,
ignored pause, stale reset origins and ignored flush failure. Tests also cover
both shared reply buffers, exact PCM copies, backpressure, invalid cursor
positions and reset while playing. Locks/address translation and DSP replies
are simulated: this is not physical cache/firmware or concurrent memory-model
validation. The first test harness lacked a retained ABI symbol, then supplied
DSP sample positions without the required halfword swap; both were corrected
before the passing result. The source queue is not changed to accommodate that
harness error.

Two short silent Old-mode HLE runs exercise the final owner with native DSP
voices. Starting both streams together gives **matching observed onset**.
For a future start at input frame **3217**, callback dispatch occurs at **3284**;
actual native audio is **230 output samples / 7.028 ms late** relative to the
PCM reference. This includes more than the 67-input-sample dispatch delay.
Both streams produce the expected signal/gain, the actual PCM cursor advances,
reset cancels and retires the native voice, and capture guards remain intact.
The output and cursor plots were inspected. **Future-start alignment is still
unfinished; protocol pass is not audio-parity acceptance.** Do not hide this
by changing the game's timeline or applying a guessed compensation constant.
The 1ms source control/ADSR bridge and AUX/reverb remain outstanding.

Final run evidence: `audio-pcm-queue-device/20261015-001506/` (paired start,
4.04 host seconds) and `20261015-001538/` (future start, 12.02 host seconds).
These elapsed times are test runtimes, not performance measurements. The earlier
`001140` run reproduces the same future delay before a paused-start repair.
Two device-fixture compile failures (uint32 pointer type and constant-zero
unsigned comparison) were fixed before successful execution. All owned
emulators exited; no console request, deployment, assets or saves changed.
Reproduce with `test_audio_pcm_queue.py`, then
`test_audio_pcm_queue_device.py --due 0` and its default future-start case.
`analyze_audio_pcm_queue.py RUN` plots existing captures without another boot.

Old Fast remains **9.925 physical FPS**, targeting **30 FPS /33.33 ms**. This
work enables an architectural audio offload but establishes no game CPU/FPS
saving. The next work is game-output/control integration and future-event
alignment, then actual workload measurements; avoid repeating already passing
sentinel and identical steady-loop checks.


## Actual platform integration candidate (15 October)

The new PCM owner is now connected to **an isolated actual game replay build**,
not just a standalone fixture. `audio_pcm_platform_patch.py` removes producer
SDK queue/reset/pause operations from platform_audio.c, retains producer copying
and exact-range cache cleaning, and multiplexes the platform's capture callback
with PCM service. The existing recursive I/O lock remains separate from synthesis;
NDSP callback never takes it. Pause-before-initialization is preserved. Shutdown
unregisters the callback, completes ndspExit, then frees PCM and permanent silence.
The source mixer still produces all audio: **DSP voice offload is not integrated**.

The queue adds a producer-side status view and atomic callback counters, avoiding
concurrent reads of mutable SDK descriptors for telemetry. Its state remains
1,032 bytes; PCM remains 128 KiB and reset silence adds 640 linear bytes. Pinned
Melee exact-range flush and shutdown lifetime practices are retained. Unlike its
single-channel PCM path, this candidate additionally needs a reset acknowledgement
and coherent input-frame cursor to support the future mixed CPU/DSP output.

The actual SDK ARM test now passes 1,363 assertions including status reporting;
five behavioral mutants still fail. Queue SHA is 99e2005ea831. Prior synthetic
voice alignment results remain separate: paired onset matched, future onset was
7.028 ms late. This integration neither fixes nor accepts that future-event gap.

`build_audio_pcm_platform.py console-replay --revision v2` produced shared
source **e766d5f3790d**, 3dsx **200bd951e44c**, CIA **f10782066274**. Its 689 source
records match the six intended files; all 18 GPU programs and the game archive
are unchanged. Old80/New124 profiles remain. The isolated builder replaces the
live archive directory entry rather than truncating a possibly hardlinked inode.
Root and archive restoration pass. The first v1 invocation rejected an outdated
SDK source-whitelist string before mutating root; v2 uses the accepted builder's
exact condition prefix. No normal variant was built or console file deployed.

The actual CIA content completed the shortened Old Fast ThornTail replay in
57.32 host seconds, with state assertions, preserved saves, drained logs and
owned exit. Capture1200 shows Fox, textured world/vegetation, HUD and touch
controls. No renderer path changed. The 38.42-second DSP output capture has
active stereo audio around -27.14 dBFS RMS and no clipped samples. Its waveform
and spectrogram were inspected; repeated late-capture gaps remain. The queue
records **35 underruns**. Older unmatched candidates recorded 26, 27 and 24;
those differ in code/workload timing and are not a controlled regression or gain
comparison. This is gameplay/shutdown validation, **not uninterrupted-audio or
audio-parity acceptance**. Streaming channel1 submitted no buffers in this route,
so this run does not cover its playback or movie transitions.

Observed Old memory: heap 42,132,440/50,331,648, linear minimum free
3,745,664/12,582,912, VRAM minimum free 4,723,712, measured main stack
73,044/524,288, process 69,902,336/83,886,080 bytes. These are sampled bounds,
not endurance coverage. Capture-run timing is excluded from performance claims;
the generic runner initially marked it valid, then qualification corrected the
record and the dedicated wrapper now supplies the exclusion automatically.
No FPS improvement is established. Old Fast remains physically **9.925 FPS**,
targeting **30 FPS**. Keep this candidate local while completing actual native
voice admission/control and source ADSR/AUX/reverb handling.

Evidence: `audio-pcm-queue-test/{result,build-verification,runtime}.json`, archived
`audio-pcm-platform-v2-console-replay-builds/`, and
`artifacts/native/old-fast-pcm-platform-v2-old/`. Reproduce with
`verify_audio_pcm_platform.py`, `run_audio_pcm_platform.py --pcm-queue --scene
thorntail --short --label UNIQUE`, and `analyze_audio_pcm_platform.py` for the
recorded run. No private assets, normal saves, presets or console installation
changed. All owned processes are terminal.


## Preserve source voice controls while delegating waveform work (15 October)

`audio_source_bridge.c` and the pure `audio_source_bridge_patch.py` add candidate
hooks to the **original full salCtrlDsp control loop**. Original 1ms ADSR handling,
pitch selection, explicit breaks, startup offsets, SAMPLE_END delivery and
source-list teardown remain in place. An admitted voice delegates waveform work
to a backend and advances its original sample cursor with the already tested
constant-time source clock. Zero-pitch segments still publish a hold: simply
returning without informing native playback would leave that voice running.
Source time remains distinct from the PCM output cursor and needs explicit DMA
mapping before audible use.

A backend must accept a voice before CPU waveform/history maintenance is omitted.
Admission zero keeps the original software path. Once admitted, a segment or
stop failure is an explicit error, **not** permission to resume from stale
CPU decoder/filter/ITD history. Backend failures preserve native clock/token
ownership. Reusing a source slot stops the previous generation before replacing
it. Backend stop must queue audible retirement and retain resources through the
existing DSP acknowledgement; source control completion is not that fence.
Backend callbacks are synchronous on the synth owner and borrow source pointers
only until return. All sample data must be pinned/copied independently.

The bridge does not implement DSP gain scheduling, ITD, filters, depop tails,
studio/AUX routing or reverb. The admitting backend must support their changes
and reserve control capacity before source control advances. **The default global
bridge is NULL, the generator patch is not enabled in accepted/game builds, and
the test backend records events only?it produces no audio.** This is a source
integration step, not a DSP offload or performance result.

The test runs the complete original control loop with its real reader and ADSR
implementation, first with CPU rendering, then with native delegation. Across
512 scenarios /24,576 cycles, all 16,910 delegated segments preserve the complete
DSPvoice control snapshot, envelope endpoints, offsets, counts and pitch. This
includes 2,762 zero-pitch holds. Every cycle's complete DSPvoice state and end
notification trace match: 469 source end messages; explicit breaks correctly
omit SAMPLE_END. There are 472 admitted native stops and 40 declined voices;
those declined voices retain byte-identical PCM output. Separate checks exercise
slot reuse, rejected segment/stop without losing ownership, and virtual-sample
exclusion. These are not audible routing/filter/ITD/reverb comparisons.

ASan and UBSan pass; only signed-shift checking is disabled for the original
source decoder's arithmetic, as in prior source-oracle checks. Four behaviorally
rejected mutants remove hold, startup offset, stop or sample-end behavior.
Strict ARM compilation passes. ARM metadata is **4,648 bytes total**, 72 bytes
per source voice and a 32-byte borrowed segment description. There is no live
game allocation. The patched source is pinned by its normalized hash
`ebff81ecc1ea`; raw original is `94108e5ff654`, tested transformed code
`946acaea330a`, native bridge `69ede85d5906`. Adding the source pin was verified
to reproduce exactly the tested transformation and reject altered input, without
rerunning the unchanged workload.

Evidence: `audio-source-bridge-test/result.json`, exact transformed `mixer.c`,
source/variant build logs and ARM objects; reproduce with
`test_audio_source_bridge.py`. No app build, emulator rerun, hardware request,
deployment, asset, preset or save modification. Latest local game candidate
remains PCM-platform **e766d5f3790d**, with its 35 recorded underruns unresolved.
The future DSP-event alignment gap also remains. **Old Fast still measures
9.925 physical FPS; 30 FPS is not achieved.** Next work must connect the actual
DSP control/output backend and source effects, then measure removed CPU work;
do not repeat already passing source-clock and queue tests without a change.

## Map source controls to the actual queued samples (15 October)

Local replay candidate **99c62cd16efc** (`audio-timeline-v1-console-replay-builds`)
connects immutable source provenance through the actual DMA/movie pipeline and
callback-owned PCM queue. It does **not** enable DSP voice admission or retire
software synthesis, and it is not deployed. Old Fast remains **9.925 physical
FPS**; this stage establishes no performance gain.

The earlier assumption of a 320-sample startup delay was wrong. Executing the
actual DMA code shows **480 initial silent samples** in ordinary gameplay and
the direct movie mix route; the delayed movie source route starts at **640**.
The established original DMA test also expects 480. A universal constant would
misalign native sound. The new code stamps each rendered buffer from the
original `sndPCGetRenderInfo().outputFrames`, copies provenance when the real
worker copies/mixes PCM, and publishes four bounded spans alongside each packet.
Movie-only/unknown buffers remain explicitly unmapped. It does not infer source
time from wall time, rendering FPS or the latest scheduled synth operation.

The callback maps the returned SDK buffer sequence/sample offset before releasing
DONE slots. Silence, pause, reset and exact packet-end positions do not report a
source cursor. Queue epochs and synth generations are distinct. PCM reset does
not reset the original source control clock. If that original clock restarts,
newly rendered buffers receive a new generation while old queued spans retain
their own generation. Source provenance is in PCM input frames; a backend must
also respect the PCM cursor's rate, handoff discontinuities and repeated spans.
This metadata does **not** solve the previously measured ~7ms future-event
audible alignment gap or provide sub-DSP-block gain scheduling.

The pinned Melee audio implementation's exact range flush and batching were
reviewed. Cache publication, reset fences, PCM storage, worker ownership and
shutdown lifetime remain as in PCM-platform v2. No new worker, GPU work, linear
buffer, decode or per-sample provenance processing was added. Producer work is
one original render-info query per 160 synth samples, six-entry buffer lookup
and up to four metadata copies per packet. Callback work is a bounded four-span
lookup. These are necessary for the pending native voice backend, but are added
work until that backend removes synthesis. No cost reduction is claimed here.

Tests execute the actual production DMA/pipeline with synthetic source samples:
**1,640,320 mapped frames**, sliced worker operation, partial callback handoffs,
stops and 16 delayed movie schedules. PCM and source-callback ordering remain
identical between synchronous and delayed execution. ASan/UBSan pass; three
mutants reject wrong-buffer sampling, lost movie provenance and early source
positions. The initial test's incorrect 320-frame expectation failed and was
corrected from the actual copy/callback order, not by changing production delay.
The installed libctru ARM implementation passes **2,725 assertions**, including
caller map lifetime, both source generations in one packet, pause/reset,
paired start and malformed-map rejection before copying/flushing. Four mutants
reject lost provenance, stale reset state, invalid-map acceptance and wrong
offsets. The first stale-reset test did not exercise an available source cursor;
it was strengthened and then rejected the faulty implementation. These checks
do not prove firmware timing or native voice waveform/effect parity.

The isolated shared build has **691 source records**, six changed files, the
same 18 GPU programs and unchanged game archive `2f219edec50e`. Old80/New124 CIA
profiles and root/archive restoration pass. The live archive was replaced via a
new inode, preserving potentially hardlinked historical evidence. Linked normal
metadata grows **1,413 bytes** (before linker padding), with **zero additional
linear RAM**. The queue owner grows 1,032→2,216 bytes; six buffer stamps use 144,
the packet map 72 and source-clock fields 13 bytes. No normal app or paired
deployment package was built for this intermediate audio change.

The exact shared CIA content completed the silent Old Fast ThornTail check in
`old-fast-audio-timeline-v1-old`: status1, unchanged saves, drained logs and owned
exit, **63.46 host seconds**. Capture1200 was inspected: Fox, textured world,
vegetation, HUD and touch controls remain present. Captured DSP output contains
38.42 seconds, roughly -27.1dBFS, no clipped samples, and **31 underruns**; late
silence gaps remain visible in waveform/spectrum. Parent recorded35, but these
are not a controlled throughput or underrun improvement comparison. Channel1
submitted no packets; actual movie/stream transition coverage remains open.
All capture timings are explicitly excluded from FPS claims.

Sampled Old memory: heap42,132,440/50,331,648, linear minimum free3,745,664/
12,582,912, VRAM free4,723,712, main stack73,044/524,288, process69,906,432/
83,886,080 bytes. The 4KiB process-grant increase is allocation granularity;
there is no extra linear allocation. Lighting, ENV and specular remain enabled.

Evidence: `audio-timeline-test/{result,build-verification,runtime}.json`,
`audio-timeline-sdk-test/result.json`, isolated build and runtime directories.
Reproduce via `test_audio_timeline.py`, `build_audio_timeline.py console-replay
--revision UNIQUE`, `verify_audio_timeline.py` (recorded v1),
`run_audio_timeline.py --audio-timeline --scene thorntail --short --label UNIQUE`,
and `analyze_audio_timeline.py` (recorded run). No console, private asset, save or
preference changes. Next: actual native voice control/output and AUX/reverb
integration; do not repeat unchanged clock/queue/provenance tests as a substitute
for removing CPU work. Full playability/performance goal remains incomplete.

## First source-controlled DSP waveform bypass (15 October)

`audio_native_voice.c` connects the original source bridge, prepared-sample
registry and leased DSP playback owner. Unlike the earlier recording backend,
this candidate actually produces DSP audio while the admitted source voice
does no CPU waveform decoding, resampling or channel mixing. It is still an
**experimental direct-stereo integration**, not enabled in any game build.
It does not establish a game FPS improvement or complete audio offload.

Admission currently requires a direct master studio with no outgoing studio
connections, surround/rear sends, AUX sends, ITD or filter. Unsupported voices
remain on the original software path before admission; an unsupported change
after admission is an explicit failure retaining source ownership, never a
fallback with stale CPU history. The explicit experimental flag is off unless
a caller enables it. Accepted game snapshots and deployed builds are unchanged.
This narrow executable stage is a development baseline; the required endpoint
remains the complete audio/rendering port with the Old Fast 30 FPS target.

The original control loop still owns 1ms ADSR, pitch changes, source positions
and lifetime notifications. Native segments translate signed stereo volume and
envelope endpoints, pitch and zero-pitch holds into SDK controls. Stop retains
the sample lease until the existing DSP acknowledgement, including when the
source registry closes first. The pinned Melee exact flush, batching and
`ndspExit` lifetime approach was reviewed; no new raw DMA or speculative release
was introduced.

The existing four-command owner could not hold controls for buffered audio.
An explicit, ABI-wide compile-time capacity now allows the integration fixture
to use **256 commands per DSP channel**. Default capacity remains four, with
the same layout and behavior as before. Every translation unit must use the
same capacity. Native source admission requires at least256. The producer must
check available capacity **before** advancing a 5ms source cycle; the check
reserves seven events per live source voice. It does not drop or overwrite
commands when full. The Old device fixture reached161 pending commands, which
demonstrates why the former four-slot queue was insufficient.

ARM metadata for this deliberately uncompressed prototype: DSP owner456,792B,
source adapter6,456B (including the existing source bridge), 80B per source
channel record. The 256-entry owner adds443,520B over its four-entry form.
This is normal RAM metadata, not linear sample storage. It has not been added
to the game's measured memory budgets. A compact control representation remains
desirable before wider integration; do not present the prototype's 4KiB sample
pool as the real game's sample-residency budget.

The actual original full control loop, reader and ADSR were executed against the
installed libctru ARM implementation. **16,341 assertions pass**:50 original
and50 native cycles,250 native segments, matching complete DSPvoice state,
signed gains, varying pitch, zero-pitch holds, playback deadlines, capacity
backpressure, registry closure with live leases and rejection of an unsupported
AUX change. Admitted rendering calls the CPU resampler **zero times** and
produces zero software PCM while issuing the corresponding native controls.
Two mutants reject discarded controls and silently lost AUX routing. This ARM
test simulates SDK replies; it does not execute DSP audio. Strict compilation
passes for the new backend. Original source warnings remain in the fixture.

The separate silent Old-3DS HLE fixture executes the original control loop and
real DSP audio output. It compares a source ADPCM looping tone rendered through
the CPU with the same source-controlled tone rendered by NDSP. Both phases
execute176 source cycles; CPU resampler calls are **880→0**,880 native control
segments are consumed, and both phases record **zero underruns**. Frequency is
the same2287.324Hz FFT bin (authored tone32000/14Hz); stereo RMS differs by
**-0.0174% left/-0.0310% right**, with no clipping and the expected2:1 balance.
The source PCM remains silent in the native phase, so nonblank DSP output is
not coming from an accidentally retained CPU waveform.

The first capture (`20261015-010437`) correctly failed the5% RMS gate: selecting
NDSP nearest interpolation for source `SRC_NONE` omitted the *final*32kHz PCM
output conversion and increased RMS by about5.1%. `SRC_NONE` disables source
pitch conversion, not the platform's final linear conversion. The corrected
native backend uses32000Hz/linear for that case. Capture
`20261015-010626` passes the same gate in7.14hostsec; its waveform/attack plot was
inspected. **Attack shapes still differ**: the source has an initial short
envelope rise, while the candidate's first audible samples reach full level.
The plot aligns each phase to its own onset and proves no onset synchronization.
Do not call the passing steady tone envelope parity or a hardware timing proof.

Outstanding before game admission: source-compatible envelope and depop tails;
real DSP-frame/subframe scheduling and the previously measured future-start
alignment gap; AUX/reverb, ITD, filter and studio routing; source interpolation
coefficient families; mixed native/software clipping behavior; game queue
backpressure and DMA/movie generation transitions. The fixture directly queues
source PCM, so it does not exercise the game's provenance adapter. No hardware
request, new app package, deployment, asset/save/preference change or physical
FPS claim occurred. Latest local game replay remains99c62cd16efc; Old Fast's
physical baseline remains9.925FPS with30FPS still required.

Evidence: `audio-native-voice-test/result.json` and
`audio-native-voice-device/20261015-010626/{summary,audio-analysis}.json`, its
`audio-inspection.png`, and retained failed capture010437. Reproduce with
`test_audio_native_voice.py`, `test_audio_native_voice_device.py`, and
`analyze_audio_native_voice.py RUN` to inspect an existing capture without
another emulator launch. No unchanged game replay was repeated this turn.

## Actor command integration: full-frame result (15 October)

15 October ACTOR COMMAND STREAM runtime recorded; goal ACTIVE/INCOMPLETE.
Latest local replay source aafc50637b8c, 3dsx b28d2be7b5ea/CIA38d6a052d860;
actor-stream-v5-console-replay-builds NOTDEPLOYED, normal variant NOTBUILT.
Root/archive restored; shared Old80/New124, game archive/shaders unchanged.
Prepared actor geometry/material/sampler packets and direct live palette writer
replace ordinary SDK reconstruction for46262/50058actor draws in OldFast
ThornTail. World49920draws, formerworldbinder0. Specialized native paths remain.
SDK borrowed palette/geometry state deferred until real SDK consumer; actualARM
212995checks/7mutants PASS, generated SDK source matches fixture. First native
material admission still occurs at first use, not completely asset-load-owned.
V1compilefailure,V2zero-adoption,V3admissionfailure retained; V4/V5runtime PASS.
Shared actor fetch layout removes per-model interning waste; actorpoolpeak10656,
material420984/524288,heap42249624/50331648,linearfree3763968/12582912,
VRAMfree4723712,stack73044/524288,process69935104/83886080. Reserve gate tight.
V5 short OldThornTail PASS state/save/drain/ownedexit106.67hostsec;1200capture
inspected Fox/world/trees/grass/water/HUD. No full parity or physical gain.
Completeframe matchedwindow parentworld-v3 19.443499ms -> actor-v5 19.692264ms:
raw51.431->50.781virtualFPS (-1.263%). All waits included. Timing attribution
EXCLUDED foreignSMG activity and slightly differing workload. V4 raw+0.798%
was also excluded; neither establishes a useful speedup. PhysicalOld9.925FPS
unchanged;30targetneeds3.02x. No universal emulator multiplier or cumulative
speedup inferred. User latest requests full-frame estimate; answer these limits.
Evidence actor-command-test/{integration-v5,runtime-v5,development}.json,
actor-command-context-test/{result,production-source-proof}.json. Analyzer output
path corrected after discovering reports went to world-stream-test/runtime.json;
current v5 preserved separately; do not claim missing runtime-v4.json exists.
NoFTP/deploy/privateassets/save/preferences changed. All owned emulators exited.
No blocker. Continue substantial CPU/render replacement; targets remain unmet.


## Retained actor ownership (15 October)

15 October PERSISTENT ACTOR OWNER implemented; goal ACTIVE/INCOMPLETE, no blocker.
Previous goal turn PROGRESS(v5 real runtime, no full-frame speedup established).
This turn PROGRESS(retained cross-actor native state, ARM tests, archived v6,
OldFast ThornTail runtime/capture/command traffic/full-frame evidence).
Latest LOCAL actor-stream-v6-console-replay-builds: source91e9ef3f2fe2,
3dsx0c42d74b53aa/CIAc5ad7c6f6e68. NOTDEPLOYED; normal variant NOTBUILT.
Five source changes vs v5; all embedded shaders and game archive unchanged,
Old80/New124 verified, root/archive restored. Never blind-build root WIP.

Ordinary actors retain native geometry/material/sampler state across compatible
actors, replacing per-actor end/begin invalidation and full republishing.
New SDK sync preserves borrowed state while publishing camera/light/bools.
Every new actor still writes its first live pose. Shader/code/target changes
retire normally; SDK shared effect writes invalidate native raster; split/APT/
SDK draw/teardown/material release retire the owner. No new GPU pass/shader,
worker/geometry conversion/linear/VRAM allocation. Static actor state replaces
stack state; new program identity and continuity counter. Existing Melee-inspired
cache publication/fences retained. Specialized native paths remain separate.

Actual ARM11 SDK/serializer tests274029checks PASS, ten fault mutants rejected.
Generated production base.c/uniforms.c byte-identical to tested fixture. Mocked
GPU queue/program upload; not physical acceptance. Test failures before PASS:
undefined test register corrected; mock4096word output exhaustion corrected by
consuming/resetting fixture command output between iteration groups. No softened
production capacity checks. Header-only comment updated after test; no ABI delta.

V6 OldFast ThornTail94.99hostsec PASS state/save/drain/ownedexit;1200capture
(observed1202) inspected Fox/world/trees/grass/water/HUD.45973/49948actor draws
new lane;49757worlddraws/formerbinder0;21883 retained actor/common boundaries.
Whole-run commandbytes101188864->90424288 (-10.64%), workload varies slightly;
not FPS evidence. Fullframe v5 19.692264->v6 19.544587ms (+0.756%rawFPS).
World-v3 baseline19.443499->v6 (-0.517%rawFPS). All exclusive work/waits included.
Timings EXCLUDED foreignSMG emulators; no causal or physical speedup established.
No 12.9% cumulative claim. PhysicalOld9.925FPS still requires3.02x for30FPS.

Materialpool420984/524288,actorpoolpeak10656;sampledheap42254736/50331648,
linearfree3725056/12582912,VRAMfree4723712,mainstack73044/524288,
process69935104/83886080. Heap+fullreservedstack nearly85%: reserve gate stays,
only~2.8KiB remains before that admission ceiling; heap varies by workload.
No new private assets/saves/preferences/FTP or deployments. User shadow-flicker
fix remains accepted. All owned handles TERMINAL incl87516/76136failedtests,
79899PASS,93368build,93472runtime. No hardware question needed for this step.
Evidence actor-command-persistent-test/{result,production-source-proof,
build-verification,architecture}.json, actor-command-test/{integration-v6,
runtime-v6-vs-v5,runtime-v6-vs-world}.json. Build/run_actor_stream --revision v6;
analyze_actor_stream reports fullframe. Actor material preparation still first
native admission, not completely moved to asset load. Full rendering, sustained
hardware targets, final progression/save/endurance remain incomplete. Next focus
remaining source render preparation and shared SDK submission/audio preemption;
retention alone did not produce the required speedup. No unchanged repeat boots.


## Native early-submission experiment (15 October)

15 October NATIVE EARLY QUEUE experiment; goal ACTIVE/INCOMPLETE, no blocker.
Previous goal turn PROGRESS(persistent actor state, v6 runtime). This turn
PROGRESS(actual8frame OldCPU profile, missing native queue-hook discovery,
queue integration/tests/isolated build/runtime, rejected full-frame regression).
Latest accepted local remains actor-stream-v6 source91e9ef3f2fe2 NOTDEPLOYED.
Experimental native-queue-start-v1 source6e43bb368c09/3dsxf4f22bb7d744/
CIAbec7d8a64e43 NOTPROMOTED/NOTDEPLOYED; normal variant NOTBUILT.

Exact v6 OldFast instruction window1101..1108 observed1100..1107,32,085,260
ARM encounters, allcore0; resampleVoice12.78%,mixRamp5.62%,salCtrlDsp4.92%,
reverb3.37% (four26.69%). Actorappend3.39%,SDKcontext.536%; FrameEnd920total
encounters. Not cycles/retiredinstructions/elapsed attribution/FPS. Current
source/time evidence does not prove where all presentation wall time goes.
Profile replay144.93hostsec PASS state/save/drain/ownedexit;1200image inspected.
Timings EXCLUDED instrumentation+foreignSMG. tools/profile_actor_cpu.py records
exact command; analyze_arm_blocks uses exact ELF. No simulation writes.

Confirmed native world/actor entrypoints bypass minimal_draw_submitted hook;
prior native queue overlap was zero. Candidate calls existing Melee-derived
queue after completed native asset draw+fh_native_scene_used publication.
At most2 >=32KiB command prefixes/frame, startup3frames excluded. No GPUwait,
new shader/worker/geometry/image/buffer; existing explicit cacheflush/finalfence.
Single source delta platform_minimal_graphics.c, gamearchive/allGPUprograms
unchanged, sharedOld80/New124, root/archive restored. Added8B static gate state.
Existing archived early_queue differs from rootWIP: tests explicitly use archive.
Actual generated SDK queue2048schedules PASS, originalSDK mutantFAIL; actual
new gate11000frames ASanUBSanPASS, startup/empty/threshold/capacity/bounds.
Production sourceSHA matches fixture; device/GPU behavior still mocked there.

Candidate OldFast short ThornTail79.03hostsec PASS lifecycle/status1/save/drain/
ownedexit.988early batches,starts=finishes988,deferrals=retirements747,capacity
 drains0. Top1200image hasFox/world/vegetation/HUD; BOTTOM UI ABSENT, unresolved.
Not visual acceptance. Fullframe19.544587->23.509557ms,51.165->42.536virtualFPS
(-16.865%rawFPS). Scene2.033->3.659,objrender2.897->6.455,submit3.146->5.280,
presentcopy.042->2.493,presentpublish6.304->.869ms. Total regressed; cannot
present moved timer costs or shorter host runtime as a win. Timing attribution
EXCLUDED foreignemulators/workloadvariation; stillfails promotion gate.
Heap42249624/50331648,linearfree3763968/12582912,VRAMfree4723712,
stack73044/524288,process69935104/83886080. No physical speedup established.

Checked installed libctru gxqueue.o (archive pinned SHA in sdk-queue.json).
Disassembly verifies gxCmdQueueAdd automatically dispatches appended entries
when current queue active/not-running. Upstream gxqueue.c confirms interrupt
also advances to numEntries. This is a verified mechanism, NOT yet proof of
how much it caused slowdown. Need committed-prefix/staged-tail separation if
pursuing overlap, with final/wait commits and actual SDK callback/fence tests.
Do not simply enable this experiment or call it speedup; investigate bottomUI.
Source https://github.com/devkitPro/libctru/blob/master/libctru/source/gpu/gxqueue.c

Evidence native-queue-start-test/{result,build-verification,cpu-profile,runtime,
sdk-queue}.json; tools/native_queue_start_patch.py,build/run/analyze_native_queue_start.py.
All ownhandles TERMINAL12036profile,10658build,69080runtime. No FTP/assets/save/
preferences/deployment changes. User accepted shadow flicker remains FIXED.
Full parity, OldFast30/New60targets, finalprogression/save/endurance INCOMPLETE.
Next safe action queue-dispatch redesign or larger source/CPUwork replacement;
no external blocker and no unchanged repeat tests.


## 15 October: actual effect lifecycle and returned-input ownership

The current experimental DSP backend now observes actual source STD reverb
creation, modification and freeing. A bounded16-entry source registry stores
constructor parameters and an increasing lifetime token. It does not infer
history identity from equal parameters, a reused pointer or a synth-clock reset.
The source callback may have public parameters changed before the underlying
work is modified; native binding now uses the successful constructor snapshot.
An accepted Modify can free old storage and then fail recreation; its old token
is invalidated before that free. Original arithmetic/allocation order remains.

The actual platform AUX callback now receives the SDK bank's returned token and
acquires its retained payload before touching delay state. Tests prepare a new
effect while eight older blocks still need the previous history. Unknown nonzero
input explicitly faults; zero startup input does not advance an arbitrary effect.
This connects the previously isolated source, epoch and transport work, but only
the initial immutable game route is admitted so far. Source resets, live route
changes and mixed-source blocks remain guarded. Source-frame route mapping and
backpressure must be implemented before those guards can be removed.

`test_audio_effect_source.py` executes original source lifecycles and the actual
native binding/callback.414 checks,24 source and80 returned-effect blocks pass
ASan/UBSan; five faulty variants fail. Strict ARM source registry compilation
passes. `test_audio_game_epoch.py` passes3997 service checks, rejecting the old
producer-generation shortcut. Existing actual SDK return-tag transport evidence
is reused. Added source registry452 bytes, epoch owner36, route token4, bus
contexts8 and two source tokens8; no new sample arenas or threads. Melee's exact
flush and NDSP-join lifetime rules were consulted and retained. No measured
hardware callback-cost or performance acceptance follows from these checks.

The first integration candidate used the older timeline builder and inadvertently
lost v7's optimized source/native reverb span kernels. It is retained as rejected
baseline reconstruction, despite a passing short run. `build_audio_effect_owner.py`
now starts from exact archived audio-cost-v7 and preserves both kernels, the game
archive and every GPU program. Use `audio-cost-effect-owner-v2-console-replay-builds`
(source2e0cd08ae243,3dsx76ff24bdb426,CIAef106419f75e), not v1. Root and archive
restoration pass; shared Old80/New124 profiles unchanged. This remains local and
not deployable. Actual source free/modify entries compile into support objects,
but game dead stripping removes unused paths; host lifecycle coverage must not
be described as gameplay coverage.

V2 short Old-profile LLE ThornTail run completes83.84 host seconds, with360 voices
admitted,152378 segments, no source/callback faults and no curve shortages.
The reviewed1000 capture retains Fox, world textures/vegetation, HUD and bottom
controls. Saves unchanged, logs drained and owned emulator exited. Captured PCM
has2310144 scalar samples, RMS1229.15, range-10949..12131 and no saturation;
statistics are not an auditory/source parity comparison. Instrumentation and
LLE capture exclude a performance claim. Sampled normal peak42230800/50331648,
linear minimum free2708736/12582912, VRAM free4723712, process70434816/83886080,
measured main stack73044/524288. Short-route headroom is not endurance acceptance.
Evidence: audio-effect-source-test/, audio-game-epoch-test/, candidate
parent-verification.json, and native-audio-effect-owner-v2-old/.

## Latest physical Old3DS observation

On explicit request, normal2fba94b84350 was uploaded/readback-verified to Old229,
matching New223, with saves/assets/settings preserved. The CIA needs FBI
installation at SD:/3ds/foxhollow/foxhollow.cia. This is the HEAT/audio sample
lifetime repair, not the experimental DSP or actor-stream-v6 branch.

After the user reported up to11FPS, read-only collection20261015-123304 obtained
new logs. Complete windows ending at frame1010..1400 cover402 intervals and
35.830397 seconds:11.219524FPS,89.130341ms mean,295.434ms maximum,96 intervals
above100ms. Reported submission4.746017ms and GPU span27.334535ms are not a full
additive frame breakdown. This confirms roughly11FPS in this run; it does not
establish a percentage gain against a different scene/replay or calibrate Azahar.
The collected executable matches the deployment, and the user identifies it as
the new run; the normal log itself lacks a build tag. See that collection's
performance-analysis.json for input hashes and exact scope.30FPS still requires
about2.67x this throughput. No crashes/GPU pending work appear at normal shutdown
in the collected log; this is not proof all previously reported failures are gone.

## Queued native effect provenance ? local experiment

Effect identity now travels with the actual source PCM: it is stamped after
rendering, copied when movie mixing copies that source, retained in each queued
packet and selected from the SDK upcoming output window. Preparing a newer
source buffer can no longer relabel already queued sound. Adjacent source spans
with different effect tokens remain distinct. No second scheduling ring, sample
arena or worker was added. The callback retains the last playback route during
unknown windows; this does not establish gap/tail waveform parity or retirement.

Source resets, route changes and mixed-source/effect DSP blocks still fail
explicitly. They need real scheduling and bounded publication/backpressure before
this path can be deployed. The retained effect owner and two-publication returned
AUX transport are unchanged. A newer producer route is never evidence that an
older returned bank or its delay state can be freed. Preserve unchanged-bus history
when implementing route replacement; do not recreate both buses indiscriminately.

Actual mapping/service sanitizer checks pass652601 assertions and reject4mutants.
The actual archived DMA/movie pipeline validates1640320 mapped frames across16
worker schedules with unchanged PCM and mutation order;3mutants fail. Pinned SDK
ARM publication passes1455 checks, including copied caller ownership, a route
boundary inside one output block, pause/underrun/reset;3mutants fail. Existing
actual generation service3997checks also passes. Firmware returned-tag transport
is reused unchanged, not re-proved by the synthetic SDK replies.

Isolated `audio-cost-effect-timeline-v1-console-replay-builds` has source44640f8c84e0,
3dsxf355c9b8e11f/CIAa6ca096b7aa9. Parent is exact effect-owner-v2; both optimized
v7 reverb implementations, SDK owners, game archive and every GPU shader are
unchanged. Old80/New124 and root/archive restoration pass. Static object storage
increases660 bytes (queue576, source stamps48, DMA map32, callback token4), with
no new heap or linear arenas. Do not build unrelated root WIP or use an older
builder that loses the optimized reverb kernels.

Short Old-profile LLE ThornTail run finishes73.66 host seconds with366 voices,
155115 segments and zero source/callback faults or curve shortages. Reviewed
capture1000 retains Fox, stone entrance, world textures/vegetation and HUD.
Saves remain unchanged and owned emulator/logs exit cleanly. PCM2326528 scalar
samples have RMS1242.66/no clipping; these are statistics, not auditory or source
parity acceptance. Instrumentation/LLE capture excludes FPS comparisons. Sampled
heap42230800/50331648, linear minimum free2708736/12582912, VRAM4723712,
process70434816/83886080 and measured stack73044/524288 remain within this short
route's limits. Unchanged sampled peaks do not erase the660-byte static increase.

Evidence: audio-effect-timeline-test/, audio-effect-pipeline-test/,
audio-effect-queue-sdk-test/, candidate parent-verification.json and
native-audio-effect-timeline-v1-old/audio-effect-timeline-review.json. Both
installed normal builds remain2fba94b84350; no FTP/deployment occurred.

## Shared native effect histories and source AUX rebinding

`audio_effect_routes.c` replaces the initial static two-effect owner in the local
DSP candidate. Two retained output routes can reference at most four effect
objects. Unchanged buses share their actual delay/rate state. A temporarily
unbound source effect remains resident for later rebinding; the original source
free/recreation event, followed by retirement of every queued route reference,
authorizes destruction. Thus neither newer preparation nor removal from the
current route resets an unchanged bus or destroys buffered sound's effect.

Capacity includes detached histories. Exhaustion returns BUSY before publication,
allocation or destruction; the scheduler must eventually handle that backpressure.
Factory failure rolls back only newly created unpublished objects. Callback
retirement is accepted only after an actual returned route has ended, and callers
must still prove both buses/queued tags can no longer reference earlier tokens.
No automatic retirement based on the producer's current route was introduced.
The game still rejects source resets, route changes and mixed-source/effect blocks;
only initial-route admission is connected in gameplay. The new owner is necessary
infrastructure, not complete native-audio transition or waveform acceptance.

Source AUX rebinding now allows older immutable timed playback leases after an
actual synth reset, requiring empty source maps and rejecting current/future or
immediate leases. It never alters queued DSP commands, samples or SDK state.
The actual original source/SDK ARM fixture passes773 assertions. An expanded
byte-for-byte snapshot test initially hit its15M instruction ceiling; a bounded
60M rerun finishes. This was test capacity, not a game or firmware hang.

The route fixture processes208 original reverb bus blocks and passes1175
sanitizer checks, including live shared state, detach/rebind, identical-parameter
new lifetimes, ended sources still needed by two routes, four-object exhaustion,
and allocation rollback. All6 injected faults fail. An initially surviving
reference-count fault exposed missing coverage; the ended-source/shared-route
scenario now catches it. Actual source lifecycle/native variable-frame callbacks
pass411 checks,24 source and80 returned blocks;5 faults fail. These tests cover
ownership and state continuity, not DSP firmware timing or general audio parity.

Latest local archive: `audio-cost-effect-routes-v2-console-replay-builds`,
sourcee76a815d643d/3dsx309632b97890/CIA1f469d1eba1c. Its builder isolates exact
queued-effect parent sources and restores root WIP/game archive. Both v7 reverb
kernels, SDK queue/AUX transport, game archive, every GPU program and the actual
mixed-source callback service remain byte-identical. Old80/New124 preserved.
V1 is retained but not run/promoted: review caught destruction of detached live
history; v2 fixes it. Do not use the older static-owner experiment as current.

ARM route owner256 bytes; removing two static Effect objects and the earlier
epoch owner reduces static object storage4496 bytes. Used effects now allocate
2360 bytes metadata plus98304 bytes delay arena each. The initial DPL2 bus costs
100664 bytes; the four-effect limit is402656 payload bytes plus allocator
metadata. No new worker or linear sample arena. The `normal_effect_bytes` counter
now includes heap metadata; its increase from98304 is not a larger delay arena.

The exact v2 Old-profile LLE micro-ThornTail route completes110.31 host seconds,
366 admitted voices/154754 segments, zero source/callback faults and zero curve
shortages. Capture1000 was inspected: Fox, textured stone entrance, vegetation,
HUD and bottom controls remain visible. Saved data unchanged; logs drain and
owned emulator exits. PCM2359296 scalar samples have RMS1227.27/no clipping;
statistics alone are not auditory/source acceptance. Instrumented LLE capture
excludes FPS comparisons. Sampled heap42233168/50331648, linear minimum free
2708736/12582912, VRAM4723712, process70434816/83886080 and measured main stack
73044/524288. No physical-console result or deployment follows from this run.

Evidence: audio-native-reset-test/, audio-effect-routes-test/,
audio-effect-routes-owner-test/, candidate parent-verification.json and
native-audio-effect-routes-v2-old/audio-effect-routes-review.json. The installed
normal builds remain2fba94b84350. Next integration work is mixed DSP block
scheduling and producer backpressure with callback-proven retirement; do not
remove guards, drop voices or resume stale software histories to pass a route.

## Independent continuous DSP bus conversion

The native effect payload previously owned its32kHz conversion state. That was
incorrect across effect changes: a new effect reset the stream phase, and two
buses sharing one effect advanced the same converter twice per DSP block.
`platform_audio_game_dsp.c` now retains one converter per enabled DSP bus,
independent of the effect selected for returned input. Allocation happens once
before enabling the bus; pointer publication/acquisition is explicit. Reverb
equations and optimized rate conversion are unchanged. Converter state is freed
only after the SDK thread has joined. General gap/partial-block alignment and
source transitions remain guarded, not implicitly fixed by this ownership change.

Actual source/native callback tests pass1185 sanitizer assertions,24 original
source and272 returned waveform blocks. Shared effect history with two separate
bus converters is exercised. Seven erroneous implementations fail, including
resetting the converter on effect changes and sharing one converter between
buses. This is callback/ownership correctness, not full game audio acceptance.

A separate execution of the actual converter and output-clock functions measures
their partitioning over10000 continuous blocks.456 block lengths differ;
228 boundaries have one extra converted source sample, followed by compensation.
The first occurs at block41 (nominal156, converted157), followed by block42
(nominal157, converted156). The cumulative boundary difference is0 or1. This is
causal resampling alignment, not evidence of a firmware failure. A mixed-effect
consumer needs one-sample lookahead/provenance and cannot simply partition the
returned block using nominal window lengths. Startup, buffering gaps and queue
resets were not covered by this zero-origin continuous-clock analysis. Do not
change rate or pad/drop samples to manufacture matching counts.

Isolated local archive `audio-cost-effect-bus-rate-v1-console-replay-builds`:
source17beeb063dc9/3dsx3133266f7a2e/CIA2386df8e376c. Only the DSP owner translation
unit changes from effect-routes-v2. Both v7 reverb kernels, converter equations,
SDK queue/AUX transport, effect-route owner, mixed-source service, game archive
and all GPU shaders remain unchanged. Shared Old80/New124 and restoration pass.
The new builder uses that exact archived parent; root WIP is not a build baseline.

Linked ARM types: Effect2360->356 bytes; each bus converter2000 bytes. The two
published pointers add8 static bytes. One bus/one effect has100660 payload bytes;
four effects/two converters are bounded at398640 payload bytes, plus allocator
metadata. Separate allocation overhead remains real; these figures are not an
exact process-memory delta. No new linear sample allocation or worker was added.

Exact Old-profile LLE micro-ThornTail completes83.87 host seconds,363 native
voices/153537 segments, no source/callback faults or curve shortages. Capture1000
was inspected and retains Fox, textured stone entrance, vegetation, HUD and
bottom controls. Saves unchanged, logs drained, owned emulator exited. Captured
PCM2318336 scalar samples has RMS1231.79/no clipping; waveform statistics alone
are not auditory/source parity. Diagnostic LLE/capture excludes FPS comparisons.
Sampled heap42233168/50331648, linear minimum free2708736/12582912, VRAM4723712,
process70434816/83886080, measured main stack73044/524288. Live coverage is the
initial effect and one bus, not the transitions covered by host fixtures.

Evidence: audio-effect-bus-rate-test/, audio-effect-clock-analysis/, candidate
parent-verification.json and native-audio-effect-bus-rate-v1-old/
audio-effect-bus-rate-review.json. Installed normal builds remain2fba94b84350.
Next integration must carry a bounded lookahead frame and complete source/effect
window through the actual returned AUX banks, before split effect processing
and native voice scheduling can lift the existing transition guards.


## Returned DSP source windows (15 October, experimental only)

Replay `audio-cost-aux-window-v1-console-replay-builds`, source `b19e866046ae`,
now carries the complete nominal PCM source window, PCM reset epoch and rational
clock phase through the actual SDK AUX banks. A callback-only queue getter also
copies the provenance of one following input sample. That sample may belong to a
different generation or effect. The getter reads only acquired, ACTIVE metadata;
it does not advance the playback clock or extend native voice playback. Missing
lookahead remains distinct from a valid untagged streaming sample.

This addresses the transport needed by the previously measured one-sample
resampler/nominal-window discrepancy. It does **not** complete mixed-effect
processing. The game now uses the mapped callback, validates returned tokens and
keeps the initial-route effect callback unchanged. Source reset, mixed-window and
effect-change guards remain. Startup, gaps, partial-prefix alignment and source
transitions need a converter-aware consumer and matching native voice scheduling.
Unavailable windows cannot be assigned the latest producer state.

The exact pinned SDK ARM fixture passes1,603 queue checks and rejects8 introduced
faults. The AUX dispatcher passes165,695 checks and rejects13 faults, including
future/stale windows, losing lookahead, overwriting metadata between bus callbacks,
and returning metadata for a bus disabled at publication. These use serialized
synthetic DSP replies, not firmware timing/concurrency acceptance. The build
preserves all GPU programs, the game archive, optimized source/native reverb
kernels, rate arithmetic, PCM queue operations and native voice scheduler. Old80/
New124 CIA profiles remain. Pinned Melee range flush and SDK exit lifetimes are
unchanged. Root WIP is restored; use `build_audio_aux_window.py`, never a blind root
build. Queue changes are applied to the selected archived source, not root's older
queue implementation.

Actual linked AUX storage grows2628->3120bytes, plus40bytes of callback-owned
coverage counters:532bytes static total. No added sample, linear, VRAM, heap or
thread allocation. The callback-stack delta has not been measured; main-stack
telemetry is not a measurement of the SDK callback stack.

One silent isolated Old LLE micro-ThornTail run passes state/save/log-drain/owned
exit in65.54hostseconds. Its inspected1000capture retains Fox, textured entrance,
world, vegetation, HUD and bottom controls. It admits359native voices across
152,624segments, with no source/callback faults or control shortages. Bus0 receives
5,293valid windows, all with lookahead;3 are valid unknown streaming windows and
1,615callbacks have no mapped window. Bus1 is unused and no next-effect boundary
occurs. This is initial-route transport coverage, not transition or gap parity.
PCM levels show RMS1222.49 and no clipping across2,301,952scalar samples; those
statistics do not establish audible or source waveform parity. Timings are excluded
because this is the instrumented LLE/capture build. There is no new FPS claim.

Sampled Old heap peaks42,237,528/50,331,648bytes, linear minimum free2,656,128/
12,582,912, VRAM free4,723,712, process70,434,816/83,886,080, measured main-stack
upper bound73,044/524,288. The console remains normal2fba94b84350; latest physical
Old measurement remains11.22FPS/89.13ms, requiring about2.67x throughput for30FPS.
No deployment, private asset/save/preference changes or extra hardware request.

Reproduce with `test_audio_aux_window.py`, `test_audio_aux_mapped.py`,
`build_audio_aux_window.py console-replay`, then `run_audio_cost.py
--audio-game-dsp --revision aux-window-v1 --scene thorntail --short --micro
--label native-audio-aux-window-v1-old`. `record_audio_aux_window.py` verifies parent
scope, exact test/build identity, storage and runtime evidence. Results live under
`audio-aux-window-test`, `audio-aux-mapped-test`, the candidate's
`parent-verification.json`, and the run's `audio-aux-window-review.json`.


## Mixed converted effect blocks (15 October, local consumer)

`audio_aux_spans.c` now implements both the span mapper and a complete converted
block consumer. It admits every effect payload before processing, checks the
retained converter phase against the nominal PCM phase, predicts the actual
converted count, and dispatches the resulting spans to their retained source
effects. The final one-sample lookahead can select a different effect without
relabeling the nominal block. The converter runs once per returned block and is
never reset merely because the source effect changes. No allocation, worker,
GPU change or additional sample buffer is introduced.

Mapping/admission failure leaves converter, samples and effect histories intact.
Failures during an application callback are not transactional rollback. The
consumer does not retire routes: both buses, queued bank identities and needed
history must finish before retirement. The caller must establish the absolute
PCM origin and queue epoch; the consumer checks phase but does not guess startup
or reset alignment. Its stereo/STD mode is immutable for that converter.

The new consumer is **not connected to live game transitions yet**. Initial
unknown prefixes, gaps and mixed native voice scheduling remain necessary before
removing the existing guards. Unknown source generations are rejected. Two buses
using the same effect object also need original source-block ordering; the current
waveform checks use independent buses and share an unchanged object across
successive routes, not across both buses simultaneously.

`test_audio_aux_spans.py` compares the untouched original160-frame reverb against
the exact archived optimized v7 native kernel. It reconstructs converted input,
runs the original source callback using its required contiguous3x160-plane ABI,
and compares complete returned DSP output after resampling. Both STD and DPL2
paths pass across1,536returned bus blocks,12mixed blocks and48lookahead blocks,
with effect changes at source frames160,6400,12800. The unchanged second bus keeps
its history. ASan/UBSan passes13,760checks;11 introduced mapping, routing,
preflight, clock and converter-reset faults are rejected. Early fixture failures
were corrected ABI packing and an invalid non160-aligned source transition,
recorded in `fixture-corrections.json`; they were not production bugs.

Strict ARM11 compilation records individual stack frames:200bytes for the full
block entry,248 for mapping,48 for admission/application and24 for the converter
callback. These are not complete SDK-thread stack high-water measurements or
CPU timing. No new game build/emulator boot was needed for this unconnected
consumer. Latest local replay remainsb19e866046ae and installed normal remains
2fba94b84350. Physical11.22FPS and all performance targets are unchanged.
Evidence: `artifacts/research/pica-scene-rewrite/audio-aux-spans-test/result.json`.


## Mapped stream connected to the game (15 October, experimental)

Replay `audio-cost-aux-stream-v1-console-replay-builds`, source `f296fedf40fb`,
now uses the stream/block consumer in the actual DSP callback. The former scalar
effect callback and its reverb wrapper are removed. Valid source-unknown/movie
prefixes explicitly contribute zero wet audio while advancing the converter;
they do not select a newer producer effect. Silent callbacks without a valid PCM
window hold converter and reverb history. Nonzero unmapped sends fail instead of
silently dropping audio. Each bus tracks its PCM origin, epoch, next input and
fractional phase. First admission requires phase zero and an unused converter;
missed startup, mode changes and queue resets are not guessed into alignment.

The source service and native voice scheduler remain unchanged. Full source
resets, mixed native voice spans and changing effect publication are still guarded.
An effect object shared by both buses is explicitly rejected pending original
source-block ordering. Independent buses and an object retained across successive
routes are supported by the new processor. General gameplay transition support
is not complete, so this candidate is not deployable.

The original fixed160-frame callback and exact archived optimized native kernel
match across3,072returned bus blocks,32mixed and96lookahead blocks, with prefixes
of0/640source frames and eight-callback pauses/resumes. ASan/UBSan passes28,547
checks and16fault mutations are rejected. New failures cover advancing during
hold, wrong epoch/mode, wrong unknown routing and discarding nonzero unmapped
input. `test_audio_effect_source.py` now selects this current stream regression;
`test_audio_effect_bus_rate.py` explicitly exercises its historical archived owner.

The isolated shared build preserves the game archive, optimized kernels, converter
arithmetic, PCM queue, SDK AUX transport, native voice/source adapters and all GPU
programs. Old80/New124 CIA profiles and root/archive restoration pass. Additional
linked static state is144bytes (8mode,64stream,72failure journal), with no added
sample/heap/linear/VRAM allocation. The existing `owner_bytes` log does not include
those additional144bytes; ELF verification and process memory are authoritative.
Individual ARM stack frames include80bytes for stream entry,200 for block entry,
and248 for mapping. Complete SDK stack high-water is not measured.

One silent isolated Old LLE micro-ThornTail run completes in75.96hostseconds with
state/save/drain/owned-exit checks. The1000capture was inspected: Fox, textured
entrance/world/grass/vegetation, HUD and bottom controls remain. It processes
5,330mapped returned blocks (all with lookahead),3valid unknown windows and
1,593silent unmapped callbacks, with364native voices/153,970segments and zero
source/callback faults or curve shortages. Bus1 is unused; this route does not
exercise a game effect/source transition. PCM RMS1235.05/no clipping across
2,310,144scalar samples is not auditory or source waveform acceptance. Diagnostic
LLE/capture timings are excluded from performance comparisons.

Sampled Old heap42,233,168/50,331,648bytes, linear minimum free2,708,736/12,582,912,
VRAM free4,723,712, process70,438,912/83,886,080 (4KiB more commitment), measured
main stack73,044/524,288. No physical speedup is established. Installednormal
2fba94b84350 and the latest physical11.22FPS observation are unchanged.

Evidence: `audio-aux-stream-test/result.json`, candidate `parent-verification.json`,
and `native-audio-aux-stream-v1-old/audio-aux-stream-review.json`. Reproduce with
`test_audio_aux_stream.py`, `build_audio_aux_stream.py console-replay`, the existing
`run_audio_cost.py --audio-game-dsp --revision aux-stream-v1 --scene thorntail
--short --micro --label native-audio-aux-stream-v1-old`, and
`record_audio_aux_stream.py`. Next work is actual mixed voice/source transitions
and bounded retirement/backpressure; a new producer token alone cannot retire
old routes still referenced by returned buses, queued banks or source/PCM data.

15 October: checked queued effect lifetime

Local replay9ac39adce6d8 (audio-cost-effect-retire-v2-console-replay-builds) adds
a producer-owned nondecreasing effect-token watermark at actual PCM publication.
It persists across queue resets and unknown gaps; failed admission, BUSY or cache
flush cannot commit a newer watermark. Invalid order is an integration failure,
not permission to drop audio. The ordinary second PCM channel remains unchanged.

The actual AUX dispatcher now invokes retirement after both returned buses have
processed and the new bank identity is published. The minimum retained effect
includes both banks and lookahead. Combined with completed processing and checked
future PCM order, this permits callback retirement; actual collection/destruction
remains on the producer. A producer's new token alone never frees old state.
Legacy scalar callbacks and unpublished future metadata cannot supply this proof.

Actual pinned SDK ARM tests pass1482queue checks and165733AUX/coordinator checks,
rejecting6queue and7retirement mutations. The latter publishes two real route
payloads, exercises mixed/lookahead banks on both buses, collects only after the
old route is no longer referenced, and rejects subsequent old-route reacquisition.
These are serialized shared-memory fixtures, not firmware or gameplay-transition
acceptance. Prior original-source waveform tests are reused only for exact
unchanged kernels/converter/stream inputs verified against the archived build.

The isolated shared build preserves the game archive, all GPU programs and
Old80/New124 profiles. Six sources change. Static growth28bytes:16PCM queue,
8AUX callback fields,4collection counter. No extra sample or effect allocations.
Telemetry also includes144previously omitted static bytes, so the logged owner
increase156bytes is not the actual28-byte growth (queue counted separately).

One Old LLE micro-ThornTail run passes in93.72hostseconds with normal shutdown,
log drain, unchanged saves and owned-process exit. Capture1000 inspected: Fox,
textured entrance/world/vegetation/HUD/bottom controls.365native voices and
154274segments; zero source/callback faults and curve shortages. Bus0 handles
5384mapped windows with lookahead,3unknown and1666silent unmapped callbacks.
Bus1 unused. Retirement floor1, collected routes0: this route does not exercise
an actual game effect transition. Do not conflate synthetic transition fixtures
with this runtime. PCM2359296scalar samples/RMS1220.69/no clipping provides
statistics only, not auditory/source parity. Diagnostic timing is excluded.

Memory unchanged from aux-stream-v1: heap42233168/50331648bytes,
linear minimum free2708736/12582912, VRAM4723712, process70438912/83886080,
measured main stack73044/524288. No physical speedup or deployment. The latest
Old console measurement remains11.2195FPS/89.1303ms, consistent with the user's
11FPS observation but not a matched replay estimate of emulator improvement.

Reproduce: test_audio_effect_order.py; test_audio_effect_retire.py;
build_audio_effect_retire.py console-replay; record_audio_effect_retire.py;
run_audio_cost.py --audio-game-dsp --revision effect-retire-v2 --scene thorntail
--short --micro --label native-audio-effect-retire-v2-old;
record_audio_effect_retire.py --runtime. Existing output folders preserve evidence;
use fresh revisions/labels for new experiments. The initial builder wrong-file
assertion preceded writes; v1 ARM build rejected a printf uint32_t format mismatch,
fixed by an explicit unsigned cast in v2. v1 restoration and failure log retained.

Still required before deployment: mixed native voice/source windows, bounded
producer backpressure, real transitions, shared-bus source ordering, native
voice control/depop waveform acceptance and meaningful full-frame measurements.
Two route slots can fill before the old PCM is delivered: a blocking wait inside
the synth lock is not a valid solution. No remaining source/effect guards were
removed by this change. Installed normal2fba94b84350 remains unchanged.

15 October: continuous voice scheduling across effect boundaries

Local replaye5ae071b250b (audio-cost-voice-window-v1-console-replay-builds) replaces
the game callback's single-known-span restriction. A source interval can cross
multiple effect IDs without becoming multiple voice scheduling intervals. The
new bounded mapper verifies continuous source frames/generation, keeps leading
unknown PCM, and produces one region for the existing native voice service.
The complete original AUX metadata, including independent lookahead identity,
is passed unchanged to the per-effect processor. A first-token scalar diagnostic
tag is not substituted for that provenance.

Interior gaps, mixed source generations, malformed/partial coverage, overflow,
backwards effect IDs and a zero/nonzero native-route availability change remain
explicitly unsupported. The result is not written on hold/rejection. This does
not enable producer-side effect/source changes: their existing admission guards
remain until rebinding and bounded backpressure are integrated.

Actual mapper and extracted game service pass1147268ASan/UBSan checks over all
0..160leading-prefix/effect-cut placements, four spans and uint64 edge cases.
Nine mutations are rejected, including collapsed AUX provenance and double voice
service. The test records the SDK boundary call and verifies its entire interval
and the original AUX window. It does not emulate firmware or establish native
voice waveform parity. Strict ARM compilation passes; mapper stack80bytes,
full SDK stack high-water still unmeasured.

The isolated shared build changes only this mapper/header and the game service.
All other game-owner functions,729other parent source records, game archive and
GPU programs are unchanged; Old80/New124 profiles and root restoration pass.
No static/sample/heap/linear/VRAM allocation added. The exact tested service body
matches the archive after excluding only the unchanged diagnostic CPU-reference
early return. No new initial-route emulator run: the earlier route would not
exercise effect transitions. No physical FPS improvement or deployment claim.

Reproduce with test_audio_voice_window.py, build_audio_voice_window.py
console-replay, and record_audio_voice_window.py. Evidence is
artifacts/research/pica-scene-rewrite/audio-voice-window-test/result.json and
candidate parent-verification.json. Next implement producer-only live AUX
rebinding without touching queued DSP leases/commands, safe route/control
backpressure and genuine transition coverage. Mixed source epochs cannot be
implemented by repeatedly calling the all-voices service in one DSP block.

15 October: live source/effect rebinding

Local replay2fb90eb933f3 (audio-cost-rebind-v1-console-replay-builds) connects
same-source effect replacements with unchanged enabled buses and mix mode.
It publishes the independently retained effect route before updating source
callback identities. A recreated source object gets new history even with the
same address and parameters; unchanged buses retain their existing effect state.
Bus converters are allocated only at initial admission and never reset on these
handoffs. Old PCM still carries its original effect identity through playback.

New fh_audio_native_voices_refresh_aux is producer-only and requires the same
bound generation, studio and mode. It updates handler/user pointers without
changing any live source voice mapping, DSP commands/descriptors, sample lease
or SDK state. Future source segments capture the new callback mask. Actual game
bus-enable/mode changes remain guarded separately, as do source-generation resets.

Original MusyX control code and the pinned SDK ARM object pass179checks in STD
and DPL2 with live voices, rejecting7mutants. Pending playback/pool/SDK state
remains byte-identical through refresh, and subsequent source segments continue
natively with the new masks. This is synthetic SDK execution, not firmware audio.
The result initially inherited a reset-test scope string; generator and existing
result text were corrected without repeating unchanged test execution.

Original source create/modify/free and the actual binder pass904ASan/UBSanchecks
with36route publications across STDtwo-bus, STDsingle-bus and DPL2single-bus.
Unchanged effect arena/rate history remains byte-identical. Three attempts to
publish a third pending route fail without overwriting prior routes or the
published token; after caller-authorized retirement, retry succeeds. Five faulty
variants are rejected. This fixture manually grants retirement permission;
returned-bank safety is independently tested, not a full integrated runtime
transition claim. Initial fixture build lacked sal.h; the mutable-field mutant
initially hit invalid zero wrapper parameters. Corrected valid wrapper parameters
now reject the actual wrong binding by assertion. Final evidence uses that fixture.

The isolated shared build changes only native source refresh/header and the game
binder/prepare functions. Other owner functions, optimized private kernel, game
archive and GPU programs remain unchanged; Old80/New124 profiles and restoration
pass. Static state and sample/linear/VRAM allocations do not grow. Handoffs can
retain additional old/new effect arenas within the pre-existing four-object
budget; actual transition heap high-water has not been measured.

No new initial-route emulator run or deployment. Real source transitions,
bounded producer backpressure and mixed-generation playback remain incomplete.
Capacity exhaustion still stops this experimental path explicitly; do not deploy
it or replace that guard with a blocking wait inside the synth lock. The CPU
reference diagnostic also needs explicit route-lifetime treatment before using
it for general transitions with native AUX disabled. Latest console performance
remains11.22FPS, with no new physical gain established.

Reproduce with test_audio_native_rebind.py, test_audio_effect_rebind.py,
build_audio_rebind.py console-replay, and record_audio_rebind.py. Evidence:
audio-native-rebind-test/result.json, audio-effect-rebind-test/result.json and
candidate parent-verification.json under artifacts/research/pica-scene-rewrite.

15 October: exact-sized native effect storage (local experiment)

Local replay830ca0e570e1 changes only private effect construction/allocation.
The DPL2 owner already processes only two channels; it now allocates only those
channels. Other modes retain all three. Each owner allocates its actual authored
ring lengths and predelay, with the same alignment, instead of a fixed96KiB.
Source constructors and all private processing kernels remain unchanged.

The native arena ranges35056..60656bytes for two channels and52584..90984bytes
for three. The ARM owner grows356->360bytes to track capacity. A tested33ms
DPL2 predelay uses43504arena bytes, saving54796bytes including owner growth.
This is a storage reduction, not an established FPS improvement. No new sample,
linear or VRAM storage, GPU program, worker or route slot was introduced.

Actual constructor/storage tests pass293108ASan/UBSanchecks:19212constructor
cases cover predelay allocation boundaries and adjacent float32 values, and4800
variable-span waveform blocks are byte-identical to the prior private kernel.
Four broken allocation variants fail. The actual source/effect lifecycle fixture
also passes904checks/36publications and rejects five mutations with sized arenas.
It still supplies retirement permission manually; this does not establish safe
producer backpressure or a real gameplay effect transition.

Isolated build_audio_effect_storage.py retains the exact parent game archive,
GPU programs and Old80/New124 CIA profiles. Root/archive restoration passes.
record_audio_effect_storage.py verifies both changed sources, tested function
identities, unchanged processing/service/binding, and linked ARM owner sizes.
The initial DWARF reader attempted unsupported relocation type0; it now requires
ET_EXEC and reads already-linked debug sections without applying relocations.

A route-capacity wait inside the synth lock is not a valid repair: the producer
can assemble four160-frame slices before submitting the PCM packet needed to
retire an older route. This storage reduction informs a bounded scheduling design;
route backpressure, source resets and mixed-generation playback remain unfinished.
The candidate is NOTDEPLOYABLE. Installednormal2fba94b84350 and physicalOld11.22FPS
remain unchanged. No assets, saves or console settings have been modified.

Evidence: audio-effect-storage-test/result.json,
audio-effect-storage-rebind-test/result.json and
 audio-cost-effect-storage-v1-console-replay-builds/parent-verification.json
under artifacts/research/pica-scene-rewrite. Historical rebind fixture is retained
as audio-effect-rebind-test/tested-fixture.c; the current fixture output uses a
new directory so prior evidence is not overwritten.

Exact-binary Old LLE check native-audio-effect-storage-v1-old completed in
124.12host seconds, status1 with saves unchanged, logs drained and owned exit.
Capture1000 inspected: Fox, textured ThornTail entrance/grass/vegetation, HUD and
bottom controls remain. Native audio reports363admitted voices/153426segments,
5333mapped returned bus windows, zero source/callback faults/curve shortages.
No effect transition occurred (retired_floor1/collected_routes0).

Actual normal_effect_bytes falls100660->45864 versus the previous actual
fixed-arena effect-retire-v2 run, a54796byte reduction. Peak sampled normal heap
42178376/50331648, linear minfree2708736/12582912, VRAMfree4723712,
process70443008/83886080, mainstack73044/524288. Heap falls54792bytes; process
commit rises4096bytes, so allocator savings are not a process-grant reduction.
PCM2334720scalar samples/RMS1229.81/no clipping is a signal statistic, not auditory
or original-source acceptance. Diagnostic LLE/capture timings are explicitly
excluded from FPS comparisons. Review: audio-effect-storage-review.json in run.

16 October: prioritize actual game audio paths over speculative queue expansion

The previous turn made concrete storage progress. This turn's direct game-source
and exact game-archive symbol audit finds one global STD reverb preparation and
one AUX binding, both in audioInit. No direct game update/shutdown-reverb call
exists. audioReset/sndQuit and user sound-mode selection do exist. This is direct
reference evidence, not an alias/indirect-call proof, and does not justify deleting
unsupported-transition guards. It DOES change priority: do not keep delaying game
coverage to implement an arbitrary future-effect queue that this route never uses.
A proposed small-metadata/lazy-history redesign has NOT been implemented.

Exact current830ca0e570e1 full ThornTail water-route (all movement legs, not the
180-tick micro-prefix) passes in211.40host seconds.808native voices/395038segments,
8721mapped returned windows; zero source/callback faults/curve shortages. Saves
unchanged, complete logs/owned exit. Capture1770 inspected: Fox outdoors, multiple
ThornTails, world textures/vegetation and HUD. Heappeak42178376/50331648,
linearminfree2689920/12582912,VRAM4723712,process70443008/83886080,stack73044.
Still one effect route; no effect changes are claimed. PCM3588096scalars,
RMS1590.62/no clipping is not auditory or original-mix parity. LLE/capture timing
excluded; no physical FPS gain. record_audio_effect_storage.py now accepts --run
so it records the actual named scenario/final step rather than assuming a prefix.

New run_audio_gameplay.py exercises an exact50-second prefix of the user's
preserved FHTAPE5 opening through normal title/new-game controls, with all packet
state and IO checks retained. It does not force an in-game state or claim the
unplayed rest of the recording. Local-only Old80/LLE/silent/headless run.
First two invocations failed argument validation before emulator startup (capture
list syntax, then terminal HID frame mismatch); corrected. V3 failed at recorded
IO event118/tick247.12245, before any native voices started. It incorrectly used
the newer201493568byte prepared movie: first frame16320bytes, while the tape
requires6432bytes at offset192. This is fixture incompatibility, not evidence of
a DSP fault. V3 failure is retained; source/callback counters were0faults.

Runner now verifies the exact original37014432byte prepared movie against the
previous completed recording replay SHA656ad2a7f8b300be3c1d49996da3bf5cde832e8a545783675fe505bc3836e298.
No recorded packets or IO assertions are relaxed. This movie encoding is a private
recording fixture, not validation of the current normal-install movie. Corrected
V4 is separately recorded; consult its final result before claiming completion.
Evidence: audio-game-lifetime-audit/result.json, native-audio-effect-storage-full-
v1-old/audio-effect-storage-review.json and native-audio-opening-v3-old failure.
No app source/build/deployment/private console files changed by this coverage work.

Corrected native-audio-opening-v4-old PASS186.48hostsec: exact3000tick/50second
prefix completes normally,1105IOevents/2918clocksteps/3912padstates and all
recorded checkpoints retained. Captures1200/2500 inspected: name entry with Fox,
then opening story text/cloud background. This does NOT show the completed flying
fight or a visible Krystal gameplay capture.956native voices/454202segments,
11358mapped returned windows,zero source/callback faults/curve shortages.
Saves unchanged/logs drained/owned exit. Heappeak41574896/50331648,
linearminfree4786304/12582912,VRAM4723712,process70443008/83886080,stack72936.

One same-executable full ThornTail CPU-reference run was added, reusing the native
full run. Reference PASS119.38hostsec/status1/saves/drain/ownedexit;1770capture
inspected: Fox/world/ThornTails/HUD present. Images differ in camera/position,
so they are not matching-pixel acceptance. New compare_audio_frames.py reads
phase names from this build's archived header (root has older unrelated WIP),
verifies every interval's full exclusive sum against wall ticks, and includes
ALL measured movement steps3..6 and every wait. Binary/ELF/emulator/firmware,
save/input/assets/preset identities match. No new app build or runtime code change.

Raw emulated guest-clock fullframe20.011480->18.017412ms,49.9713->55.5019FPS,
+11.06745% raw throughput,NOT a causal or physical speedup acceptance. Reference
1102frames/native1222;draws/frame168.2514->168.8953,indices56658.19->57153.15.
Paths start together but diverge by the later movement legs; exact endpoints are
retained. All timing comparisons remain EXCLUDED for diagnostic LLE/capture;
never apply this ratio to physical11.22FPS or call the emulator's statusbar FPS
console performance. Host211.40native/119.38reference seconds are NOT the guest
frame measurement and include different LLE host emulation work.

Producer mean1.4543->0.9139ms per5ms source call; SDK-active mean0.0468->0.2431ms.
Producer+SDK elapsed/produced-second303.7927->246.5740ms(-18.835%), but includes
preemption/possible overlap and different voice histories, NOT exclusive CPU
utilization. analyze_audio_cost.py retains the individual buckets. Frame
present_publish7.0570->4.2764ms; this is a wall scope including scheduling, not
an isolated GPU command cost. Work is reduced but no multi-fold win is established.

Original hw_pc.c still invokes the source effect callback for software voices,
while native returned AUX independently processes its own effect history. Do NOT
remove the source callback or tails blindly. Next prioritize actual remaining
producer/callback cost and shared-effect design with preserved source-phase,
clipping and histories; generic effect-route expansion is not the demonstrated
blocking condition. Only do another profile if it answers that remaining-cost
question; do not repeat these unchanged boots or claim a deployment is ready.

Evidence: native-audio-opening-v4-old/{audio-opening-scope,audio-effect-storage-
review}.json; native-audio-effect-storage-full-v1-old/{audio-full-frame-comparison,
audio-cost-analysis,audio-effect-storage-review}.json. No FTP/console changes.


16 October: source-generation handoff integrated in the local DSP experiment

The previous goal turn made progress by integrating the resident renderer into
one normal startup application and completing Old benchmark/New Play checks.
That hardware candidate45631515f007 remains immutable and NOTDEPLOYED; the last
Old229 FTP attempt reset during listing, before any upload. No additional FTP
attempt or console changes were made during this audio continuation.

The audio-generation integration fixture initially failed to compile because
its temporary `voices` macro also rewrote struct member names. The aliases now
end immediately after including the actual owner functions. Fault mutants also
caused the linker to discard the otherwise unreachable completion variable;
the test now explicitly retains all three diagnostic symbols. Neither repair
changes production behavior or weakens result assertions.

The actual patched prepare/capture functions now pass985 ARM11 fixture checks,
using original MusyX controls and the pinned libctru ndsp-channel object. New
source generation2 is prepared while generation1 playback leases and raw AUX
remain live. Old and new raw blocks are consumed in order; the SDK voice window
switches generations while sample leases retain distinct identities. Three
mutants damage ONLY the reset branch (initial admission remains intact): omitted
reset and omitted rebind panic; omitted epoch publication fails the generation
assertion. Effect binding is stubbed here, not an actual physical DSP test.

Existing exact-source queue/reverb evidence is reused:3.84M threaded source
frames;1536 returned bus blocks for each of reset and continuous cases, with
source-offset/channel/omission mutants rejected. Release/acquire publication,
bounded queue storage and immutable old playback ownership remain unchanged.
Pinned Melee audio.c confirms cache publication before NDSP queueing and buffer
retirement before reuse. Foxhollow adapts that lifetime rule to separate synth
and playback generations; it does not clear queued DSP buffers on a synth reset.

Local audio-generation-v1-console-replay-builds sourced546e4b29d63 builds
successfully with Old80/New124,18 identical GPU programs and unchanged parent
game archive. Only four audio owner/queue sources differ from resident-offsets
parent. Root and game archive restoration pass. This separate local diagnostic
artifact must NOT be deployed; future audio adoption belongs in the unified
startup app. No new allocation, shader, geometry or render worker was added.

Evidence: audio-generation-test/result.json; audio-generation-v1-console-replay-
builds/{scope,integration,restored}.json. Queue capacity backpressure, bus-mode
changes, wet/dry latency and physical auditory acceptance remain open. No FPS
improvement is claimed for the generation repair. Short Old LLE integration
run native-audio-generation-v1-old is pending; consult its final summary before
claiming gameplay coverage. This route does not force a synth reset; that
specific transition is covered only by the original-controls/SDK fixture above.


Completed native-audio-generation-v1-old PASS75.00hostsec/status1/step4,
161rows, original save unchanged, complete logs/drain/ownedexit. Capture1000
inspected: Fox, ThornTail entrance, textured world/vegetation, HUD and Old items
pad.368native admissions/158273segments/5388mapped windows; zero source/callback
faults and curve shortages. Raw AUX5283blocks,peak35/64,16retained at shutdown.
No reset was induced by this route; do not claim it exercises the fixture reset.
Sampled heap42451736/50855936,linearfree2184448/12058624,VRAM4723712,
process70492160/83886080. Normal-after-stack/linear/process headroom15.49/18.12/
15.97%. No additional GPU or gamecode change; no physical/performance claim.

Captured35.42seconds stereo PCM, zero clipped samples; waveform inspected.
Quiet intervals include0..13.77,23.53..24.03,27.28..30.29seconds. The earlier
resident-offsets run also has similar startup/later silence at different times;
this is not source-matched acceptance and does not establish that all gaps are
intentional. WSL plotting lacked matplotlib; existing Windows Python generated
the plot from the same PCM without rerunning emulation. Wet/dry timing and
physical auditory fidelity remain unresolved. All owned build/test/emulator
sessions terminal. Prepared normal hardware candidate45631515f007 unchanged.


16 October OLD229 UNIFIED PALACE HARDWARE RESULT; goal ACTIVE/INCOMPLETE.
Collected new normal startup replay, exact source45631515f007, status1/step10,
75rows/68 measured frames over5.031s. Actual13.5157FPS/73.988ms; p95 82.077,
max87.342ms, no>250ms in measured window. Saves unmodified. PICA ACTORS+HUD,
MAIN/VRAMoff/GPUprofileoff, graphics2113537, recipe25d74062aeca.
Exclusive wall breakdown: native_submit35.493, present_publish10.277,
object_update8.321, object_render6.284, scene_render5.339, present_copy3.268,
remaining5.006ms. Sum100%; scopes include shared-core audio/OS preemption.
GPU pending12.849ms OVERLAPS CPU, not utilization; coarse log interior has only
one GPU snapshot, frame accounting valid. No reason to request another same run.
Need40.655ms removal to reach30FPS; submit elimination alone still38.495ms.
No matched recent hardware baseline: prior11.2195FPS roaming is different route;
9Oct palace18.413FPS had different recipe/options/assets/workload. Do not invent
an attributable improvement percentage or claim this beats all older builds.
Physical memory heap41589784/50331648,linearfree4904192/12582912,
VRAMfree4723712,process71102464/83886080, measured mainstack72984/524288.
Audio queue-empty counts rise; investigate shared CPU audio as well as native
submission and present publication. Do not treat35.49ms scope as pure renderer
CPU cycles. All optional visual restoration remains gated by performance.
Evidence artifacts/console-test-app-results/20261016-003914-229-direct/
{analysis,hardware-conclusion,transfer}.json. Direct bounded named RETR succeeded,
matching before/after marker; no remote files changed. Two earlier LIST/RETR
reset attempts preserved003834/003905. Collector now bounded3 full-file retry,
retains marker immediately; cache/race/partial reset/exhaustion fixtures PASS.
No new deployment. Old4563/New2fba remain installed. All owned processes terminal.

Local audio return-delay prototype remains NOTINTEGRATED/NOTDEPLOYABLE:
audio-balanced-return/native-20261016-003534 PASSfixed320DSPsample transport
(9.777ms), source160sample/5ms wetdelay, NRMS.047784; missing-delay and dry-bypass
mutants reject. Uses both AUX buses, adds1924B, needs lifecycle/routing integration.
Samebinary d546CPUreference run PASS; raw Azahar frame21.352->18.994ms,12.415%
throughput but causal timing NOTaccepted (diagnostic LLE/capture), NO physical
prediction. Similar silent intervals in both CPU/native captures; not unique DSP
regression. tools/compare_audio_frames.py proof integration.json supported.



16 October RETAINED WORLD PUBLICATION; goal ACTIVE/INCOMPLETE, no blocker.
Previous turn PROGRESS(Old physical collection); this turn PROGRESS(exact deployed
ARM profile, native world state retention, SDK fault tests, unified build/runtime).
Local normal source3da284025118/3dsx5c56dda36807/CIAc9433a181582 NOTDEPLOYED.
Old45631515f007 and New2fba94b84350 remain installed; no FTP changes this turn.

World native material/fetch state now survives compatible block camera/shared
state publication; explicit actor/program/target/resource boundaries retire it.
Shared SDK effects invalidate native raster cache. Deferred buffers and material
uniform rows7..9 are restored for the next SDK consumer, including empty batches.
Pinned Melee state ownership/retirement consulted; no new shader/pass/geometry/
worker or linear/VRAM allocation. Adds176BstaticCPU, replaces164Blocalstate.
Not old GX interpretation. Seven source deltas, same game archive and18GPUprograms,
Old80/New124, prepared recipe unchanged, root/archive restoration verified.
Use world-retention-v1-console-setup-builds/source-snapshot.zip; NEVER blindbuild
unrelated root WIP. tools/build_world_stream_retention.py isolates parent4563.

Actual SDK ARM fixture283784checks PASS, five incorrect ownership mutants reject.
Initial actor begin contract change failed existing fixture: corrected to explicit
scene boundary. Initial lost-deferred mutant escaped: added empty-batch departure
coverage; final all reject. Generated build SDK base/uniforms match tested sources.
Evidence world-command-retention-test/result.json; candidate acceptance.json.
Old unified startup palace PASS67.08hostsec, status1/306rows,106496layoutdraws
zero layout faults, saves/settings/owned exit/drain PASS. Palace capture inspected:
Krystal/texturedhall/purple creatures/HUD visible. Single-view visual scope only.
Sampledheap41589784/50331648,linearfree4904192/12582912,VRAMfree4723712,
process71102464/83886080,stack72984/524288. Timing EXCLUDED layout instrumentation;
no established FPS win or physical acceptance. No extra hardware test requested.

Exact deployed4563 profiler run unified-resident-v2-benchmark-old-profile-v2
PASS115.88hostsec,8measuredframes1300..1307,27651093ARMencounters/core0.
Selected audio functions30.2058% encounters, NOTcycles/frame-time percent; audio
cadence differs with real time. World/native SDK publication also remains costly.
Profiler rebuilt for additive startup counters/touch, sources restored,204HIDstate
host fixtures pass. Initial profile failed prelaunch manifest guard (preserved).
Profile review records timing exclusion and exact ELF; no universal FPS scaling.
Profile is NOT evidence that entire35.49ms physical submit scope is rendererCPU.

All owned sessions terminal: profile55098fail/44728pass, profilerbuild67738,
SDKtests42157, appbuild78144, runtime92946. No private assets/saves changed.
Next matched uninstrumented cost comparison and shared audio migration; current
DSP/AUX branch still NOTDEPLOYABLE. Need40.655ms savings to reachOld30 from73.988;
this ownership change alone is not a claim of meeting target. Full goal incomplete.



16 October BALANCED DSP GAME TRANSPORT; goal ACTIVE/INCOMPLETE, no blocker.
Previous turn PROGRESS(world owner/profile); this turn PROGRESS(matching fullframe
comparison, actual audio routing/delay integration, SDK tests/build/Old runtime).
No console writes or hardware requests. Old4563/New2fba remain installed.

World-retention-v1 vs installed unified-resident-v2 HID-only Old palace pair:
300frames each,16.709669->16.715012ms,59.8456->59.8265FPS; effectively paced60.
Native submit3.097905->3.034629ms, publish5.033186->4.587846 but VBlank grows.
NOT a causal/physical win. Draws162.37->162.323 and indices73272.02->73207.52
perframe; absolute simwindow differs75ticks, same save/settings/recipe/inputs.
Candidate host sample catches foreignld71925, untouched. No further identical
saturated timing runs; do not extrapolate toOld hardware13.5157FPS. Entire wall
partition sums100%. tools/run_unified_startup.py --cost disables layout/block
instrumentation and gameplay captures, retains all state/save/drain checks.
compare_unified_cost.py validates configs except three identity-checked per-run
paths (initial strict raw config compare failed on those paths, corrected).
Both PASS/state/save/ownedexit;10584/35457 TERMINAL. Evidence respective
*-benchmark-old-cost-v1 and candidate/full-frame-comparison.json.

New LOCAL experimental audio-balanced-v1-console-replay-builds source7159c91b7b3b,
3dsx3c405652d96c/CIAf1eba2c691c3. NOTDEPLOYABLE/NOTDEPLOYED, local diagnostic
variant only; eventual console delivery MUST remain one unified startup app.
Parentaudio-generationd546. Actual native voice mixes and PCM defaults/reset now
route dry viaDSP AUXB; AUXA retains wet. B mapped passthrough preserves existing
bank/retirement identity. Original160sample/32kHz wet return delay is applied
before rate conversion, including null-handler tail spans. Uses existing two
DSP buses, no added worker/samplepool/linear/VRAM. Adds1924Bstatic callback-only
history, ELFsize verified. Older owner_bytes telemetry omits this field; process
memory includes it; do not cite that subtotal as complete. Modeadmission explicitly
DPL2(oneeffect)only; STDtwoeffect path needs another design, not silent bus loss.
CPU reference unchanged output. Source mode transitions/backpressure, runtime
source-aligned waveform/AV latency and physical fidelity remain OPEN.

PinnedMelee port/3ds/audio.c flush-before-publication/SDKqueue lifetime reviewed.
Native adaptation keeps existing cache/lease owners and only changes output route.
Actual patchedARMvoice3385checks +PCM1363checks PASS; bypassdry/lostresetrouting
mutants rejected. Exacteffect_apply orderingfixture ASanUBSan passes chronological
160sample delay across varying callbacks/nullhandler; missingdelay rejects.
Effectkernel STUBBED in orderingfixture; prior realDSPbalanced-return waveform
prototype remains separate evidence, NOTfullgamewaveformproof. Initial tests
caughtSDKconstsignature, then stale root PCMheaders/missingfixturelinks; corrected
archivedheaders/actualtimelinemodules, rerunPASS. No weakening checks.
Eight source deltas, gamearchive2f219edec50e unchanged,18GPUprograms unchanged,
Old80/New124, root/archive restoration PASS. tools/audio_balanced_game_patch.py,
build_audio_balanced_game.py, test_audio_balanced_game.py,
record_audio_balanced_game.py and run_audio_balanced_game.py reproducible.

Actual Old80 LLE ThornTail movement run native-audio-balanced-v1-old PASS142.82s,
status1/561rows, saves/drain/ownedexit. Screenshot1000 inspectedFox/terrain/HUD.
6512rawAUXblocks captured/1039534sourceframes consumed, peak35/64, retained18;
source/callbackfaults0, curveshortages0. No generationreset exercised this route.
Reserves normal-after-stack15.4946%,linear18.0017%,process15.9619%.
42.4268sec capturedstereoPCM, zero clipped samples; waveform inspected. Longquiet
intervals0..13.697,23.394..24.244,27.093..30.393sec resembleprior signal phases,
NOT source-matched acceptance. Existing parent was HOLD180, current runnerHOLD600;
DO NOTcompare raw19.0->17.94ms as speedup, workloads differ. Timings explicitly
excluded LLE/capture, initial inner printed true correctedbyouterqualifierfinalfalse.
No unchanged CPUreference rerun or another hardware request this turn.

Allowned handles TERMINAL10584/35457(cost),99590(build),76695(runtime);
SDKtest/verifiers also terminal. No privateassets/saves/preferences changed.
Next audio integration gates: source-aligned shared dry/wet capture, generation/
bus-mode/backpressure handling and merge into unifiednormal only after acceptance.
Retainedworldcandidate3da2 separatelycorrect, noestablishedhardwaregain. Full
renderparity/progression/endurance and targetFPS stillINCOMPLETE.

16 October audio follow-up: sourceb28284500a9f corrects reverb input-delay
ordering, verified against the original AUX loop and real reverb state. Its
recorded opening prefix completes under Old80 LLE with preserved saves and
zero reported source/callback faults. This is correctness evidence, not an FPS
comparison. The initial empty-effect admission guard passes host owner tests
but is not yet in a built app. See latest IMPLEMENTATION.md and
artifacts/native/native-audio-return-order-v1-opening-old/generation-review.json.
No console deployment. Read-only Old229 attempt014053 timed out before login.
Physical result remains13.5157FPS/73.988ms;30FPS requires40.655ms reduction.

16 October New startup integration: initial-route239c reproduced callbackfault13
when queued software PCM preceded native admission. Software-prefixfcdf now
derives native-only voice/AUX windows while preserving completed software audio,
source clocks and queues. Same New opening-prefix scenario passes, no source/
callback faults, captures inspected, saves preserved. Actual consumer regression
tests cover32admission positions including captured960. Added160Bcode, no static/
dynamic memory. Not deployed; no measured FPS gain. Audio waveforms are captured
but full source-aligned fidelity is not accepted; generic backpressure/transitions
remain open. See current IMPLEMENTATION.md and audio-software-prefix-test/result.json.

16 October dry/wet phase repair: a controlled simultaneous original-left/native-
right DSP test found42% sustain mismatch from unequal return delay. Matching
the rate adapter's two output samples on dryB reduces ratio1.41939->1.02876
and envelopeRMS.04413->.00869, with no clipping or altered level. Adds32Bhistory,
no allocation. d59a10a3dde3 shared local candidate passes Old80 ThornTail micro
route, no audiofaults, preserved saves; normal-after-stack reserve15.49%,linear
18.12%,process15.96%. Not deployed/fullgame-fidelity accepted/FPS improvement.
See audio-output-alignment-test and audio-balanced-voice-device/20261016-021146.

16 October unified audio benchmark integration: local normal package047a9491e50b
now contains Play and Benchmarks with a launch-only Start: CPU/DSP audio choice.
Default remains CPU; cancel resets it; mode locks before audio workers. Original
CPU mixer no longer binds unused native effect objects. Exact same game archive,
GPU programs and converter recipe; shared Old80/New124 and restoration verified.
Old DSP palace and New CPU palace complete with state/saves/drain checks and
inspected images. Old minimum linear headroom1.67MiB/10MiB, heap39.79/48MiB;
not full-area/endurance coverage. No new waveform capture or physical timing.
LLE integration timings excluded; NOT a demonstrated FPS gain, NOT deployed.
Reproduce with build_unified_audio.py console-setup; run_unified_startup.py
benchmark --family unified-audio --revision v1 [--old --audio-native].
Use record_unified_audio.py with --run names to preserve exact executable,
mode/counter and memory proofs. See candidate integration.json and latest
IMPLEMENTATION.md. Next measure actual removed ARM work and remaining audio
gates; hardware comparison belongs in this single startup app.

16 October matched unified audio ARM sample: same Old80 CXI/firmware/case,
CPU28768859 vs DSP24994608 encounters over scheduler window1300..1307 (-13.12%).
Includes new callback/queue work. Producer counts27/25 and particle phases vary;
NOT exact causal reduction, cycle/FPS estimate, or physical performance evidence.
Both runs pass state/save/drain, images inspected. User accepts audibly close
audio; do not chase exact small reverb/resampling differences. Single-app
packagea9fcd84359b840d9 is ready for CPU/DSP hardware comparison; upload024622
Old229 timed out before login, nothing deployed. See hardware-benchmark.json.



16 October: retained collision candidate lists (local 5f50dd9e7bc7)

Existing Old DSP palace ARM profile mapped to exact reconstructed track source
(DWARF MD5 3b59144e2ac3df158a72172d8b993cb3). Mask scanning and loop iteration
accounted for most assigned collision work; the old plane cache cannot remove
that scan. The new bounded eight-key list retains only immutable topology-mask
results. Load/forget clears keys before address reuse; group bounds/flags,
vertices, collision offsets, normals, edges, ordering and capacity remain live.
Broad/overflow queries keep the original scan, with overflow remembered until
invalidation. Existing bounded bitset index supplies misses; no second index.

Actual original-vs-candidate function checks cover 2000 changing map queries
and three repeated calls each, 810603 returned triangles, capacity boundaries,
missing indices, same-address reload, empty masks, overflow and lower bounds.
ASan/UBSan passes; missing lifecycle invalidation mutant is rejected. Shared
build changes only track_dolphin.o; 728 source records, 18 GPU programs unchanged,
Old80/New124 verified, parent converter recipe and root/archive restoration pass.
Linked additional static normal RAM is16592 bytes, not the host sizeof16656.

One bounded Old LLE palace run passes status1/307rows, saves/settings/drain/exit,
source/callback audio faults0. Palace image inspected, including Krystal,
floating creatures, world textures and HUD. Cache5999hits/95misses/0overflow.
Same sampled1300..1307 window: collision+cache group1629520->587904 ARM encounters
(-63.9216%); whole window24994608->23937467(-4.2295%). Rendererentry16/16,
collision72/72,audio producer25/25,particle update8/8,actorappend280/280. Voice/RNG
phases can still differ; instruction encounters include failed predicates and
are not cycles, FPS, or exact net causal attribution. Elapsed LLE148.949hostsec
is excluded from performance comparisons. No new physical run/deployment.

Old sampled heap41717672/50331648,linearfree1755776/10485760,VRAMfree4723712,
stack72984/524288,process69582848/83886080. Heap/linear/VRAM/stack unchanged from
DSP baseline; processcommit+24576 includes static data and code/alignment.
Setup and DSP mode selection unchanged from ee353; prior checks reused. No
fresh New boot, moving route, full progression/endurance or physical FPS proof.
Evidence collision-candidates-test/{result,runtime}.json, collision-profile-v1/
result.json, unified-audio-candidates-v1-console-setup-builds/verification.json.
Next larger combined cost: particle update/draw/submission, not more plane-cache
micro tuning. All renderer parity/gameplay/performance goals remain incomplete.
