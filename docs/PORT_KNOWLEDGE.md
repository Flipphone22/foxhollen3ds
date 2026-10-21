# Port knowledge base

This is a condensed version of `private/CODEX_PROGRESS_LOG.md` (local only), which holds 30k lines of Codex session history from 6 to 19 Oct 2026. Where facts conflict, the latest value wins.
- Per-period detail with log line citations (`Lnnnn` refers to lines in CODEX_PROGRESS_LOG.md): `docs/knowledge/01..08_*.md`. File 01 is the newest.
- Topic deep dives: `docs/NATIVE_*.md`, `CRASHES_20261015.md`, `SHADOW_*`, `OLD_FAST_30FPS.md`, `PERFORMANCE_ACCEPTANCE.md`, `RENDERING_ACCEPTANCE.md`.
- Experiment verdicts: `tools/index_port_experiments.py --query <topic>`, which writes `artifacts/research/port-experiment-decisions.json`. A topic with no record there is not automatically a new idea.

## 1. Goal and targets (as of 19 Oct)

Overall goal:
- Old 3DS: Fast preset at a steady **30 FPS**. The user raised this from about 20 on 15 Oct, and it is the top priority.
- New 3DS: **60 FPS**, with 30 as the minimum.
- Both: full render parity, progression, saves and endurance.

As of 19 Oct none of these are met, and there is no external blocker.

Hardware:
- New 3DS at `new223`, FTP port 5000.
- Old 3DS at `old229`, FTP port 5000.

App layout:
- One unified startup app with a single HOME icon. Its startup menu holds Play, Benchmarks/playback (isolated replay saves) and Diagnostics. Do not create extra apps or launchers.
- Graphics presets: `n3ds-fancy`, `n3ds-fast`, `o3ds-fancy`, `o3ds-fast` and Custom.
- Lighting, Environment and Specular are ON in every preset, which is a user override from 15 Oct.
- Optional or expensive effects are OFF by default. A disabled effect must cost zero CPU while still keeping simulation and RNG order.
- Assets are installed only from the ISO at `SD:/3ds/foxhollow/disc.iso`. Conversion runs once on the device, and under 70 minutes is acceptable. Prepared output goes to `SD:/3ds/foxhollow/prepared`. The ISO is the only source of game data, so never ship converted assets.

## 2. Deployed state (latest)

| Console | Build | Notes |
|---|---|---|
| Old229 | batch2-v55 (21 Oct, package `09c3b75e48c1`, launcher "Build c303c7d5378e") | v54 (depth copy + GPU stall quarantine) + streamed audio queue items 18 -> 54 ADPCM blocks (~84 -> ~252 ms buffered; `tools/audio_stream_patch.py`). Hardware result pending. |
| New223 | fancy-v55-restore (21 Oct, launcher "Build 034f5b5492a5") | n3ds-fancy back to the v55 set; heat, blur filter and distortion off. **Hardware (21 Oct, user): starts and plays on New and Old; startup hang fixed.** Ripples later removed from fancy (6c66faf, not yet deployed). |
| (prior) | texture-mips-v4 `d317a80e0e95` | Hardware-tested 19 Oct; see section 5 and the d317 report notes. |

- Prepared assets pushed from PC (no console conversion): `tools/deploy_texture_mips_assets.py` uploads the host-converted FHTXPK5 pack to `prepared/textures/g00000002/asset-1.bin` (full readback), writes bundle `g00000002` manifest and `current-1.bin` serial 2, reusing the console's own original/performance/movie generation 1. Old bundle slot 0 untouched. Manifest/record encoding is byte-identical to the app's publisher. Reports `artifacts/console-test/20261019-19100{0,2}-*-texture-mips-assets`.
- Previous: New `7b980b145315`, Old `4f58d475c8f7`. Transfers `artifacts/console-test/20261019-19004{0,2}-*-direct-install`.
- texture-mips-v4 emulator checks (synthetic fixture `texture-mips-fixture-v4`): New play, Old play, Old palace benchmark all pass with full render coverage. 144 mip images, 0 failures, 0 skipped; image peak 2.15 MB (was 1.84); Old linear min free 3.78 MB play, 3.93 MB palace.
- The app's bundle check (`install_bundle.c`) had to accept FHTXPK5; without it the console would reject the new pack.

- Use `4f58` as the parent for production work. Do not use unproven candidates such as CPU-skin V4.
- **Next deploy candidate: `world-fade-v2` normal `887a4a894b98` (3dsx `15ca1f93639e`, CIA `92e612d75859`).** Not deployed. It adds a per-surface player-distance gate for the `0x2000` geometry (skips a surface wholly nearer than player distance minus 10) to world-fade-v1 `75e7a64060e9`, which is built in this chain:
  - On world-water-slice-v3, which adds world water drawing and logs every dropped transparent surface.
  - Which is built on shadow-plan-gate-v5 `18f2d5fda0de`, then queue-mode-v2 `31188af896ea`, then 7b980.
  - world-fade-v1 itself adds back the `0x2000` near-camera fade geometry.
- The v5 base contains:
  - Route-capacity deferral.
  - The event-driven `acquire()` GPU wait.
  - A startup-menu row, "GPU queue: FRAME END/EARLY" (default FRAME END), for a hardware A/B of early command-prefix submission through the private batch gxqueue.
  - A projected-shadow plan/receiver gate (no build unless SILHOUETTE is on and GROUND SHADOWS is not off).
  - A 64 KB normal-heap margin for scene metadata placement.
  - A logged reason whenever an actor plan is rejected.
  - Evidence: `route-defer-acquire-test`, `gpu-queue-mode-test`, `queue-mode-v2-palace-*`, `shadow-plan-gate-v{3,4,5}-palace-*`.
- Builds that are built but not deployed:
  - Glow/particle-distortion pipeline (glow-adaptive / packed / quality-v4 / shared-v1).
  - `world-deferred-draw-v5`.
  - Movie wait / aligned CRC / read-decode overlap / LZ4 short copy.
  - Reflected-particle chain (glow-reflection, screen-colour, spirit-filter, distortion-separated).
  - `842f8d298080` crash candidate: audio EOF repair, a strong abort guard, and a link-time `exit` wrap routed to the fatal owner.
- Builds marked `RUNTIME_REJECTED.json` are refused by the packager. Palette refine v3 `0f7aa0a77ede` is one of them.

## 3. Standing user directives

The original wording is in `docs/knowledge/08_*.md` §6.
- **No human-readable prose**: no commit messages, PR descriptions or code comments.
- **Headless and unattended testing.** Use isolated profiles, captured logs, images and audio, scripted input, bounded runs, and cleanup limited to processes you own. No visible emulator windows, desktop input automation or audible playback. Custom emulator builds are authorized. Preserve the RVZ/ISO and keep assets and saves out of source control. Never touch console saves or settings.
- **Native PICA renderer only.** Never reconnect the old GX/CPU-vertex renderer. Separate shader binaries are allowed; do not sacrifice fidelity to fit everything in one 512-instruction program.
- **Consult the Melee 3DS reference** (`reference/melee-3ds/native-3ds/`, pinned in `sources.lock.json`) for graphics, OS integration and hardware behaviour. Watch for alignment, ABI, normal vs linear memory, cache visibility, lifetimes, sync, affinity, SD I/O and suspend/resume. Do not transplant Melee gameplay code or its BE8 bridge.
- **Performance discipline:**
  - Prefer structural changes that remove a measured dominant workload. Aim to halve frame cost rather than chasing 5% wins, and reject ineffective directions promptly.
  - A gain well above 12.9% is wanted.
  - Keeping worthwhile cumulative gains is fine; being modest on its own is not a reason to reject a change.
  - Crash fixes are necessary but do not count as FPS progress.
  - Reduced visual detail is authorized while gameplay and essential effects remain.
- **Evidence honesty.** Never claim physical FPS from emulator numbers or instruction counts, and never scale hardware FPS by emulator ratios. 21.5 ms is not 16.67 ms. Do not re-run profiles until a favourable phase shows up, and do not subtract phases to manufacture an estimate.
- **Hardware requests:** evaluate on PC or emulator first. Ask for a console run only for a substantive change or a specific open hardware question, and prefer one consolidated ordinary gameplay check over many probes. Never poll or overwrite a pending hardware test, and never loop FTP retries.
- **Deployment:** deploy only on explicit request. The user has standing authorization for uploads once a build is ready, so just ask whether the console is available, once. On deploy requests, skip slow re-verification. Report the exact SD path and build ID.
- **Slow tests are a bottleneck.** Run the smallest check that answers the question, batch changes before integration runs, and do not repeat long cold boots per edit.
- **Settled topics: do not reopen.**
  - Whole-shadow rapid flicker is FIXED and user-accepted on 14 Oct. Do not run more flicker probes.
  - Audio only needs to be audibly close, not 1:1. Crashes, dropouts and gross level or timing errors are still bugs.
  - DSP audio is the default.
  - No larger audio buffers.
  - Replay determinism work is paused, so do not fake RNG or timing.
  - Do not re-ask about MORPHS OFF. Do not ask the user to transcribe crash screens, because Luma dumps are collected automatically.
- Stalls are acceptable during startup or loading only. Do not make a stall fatal.

## 4. Open bugs

### Crashes and hangs
- **All open Luma dumps are secondary heap-unmap faults (19 Oct).** Dumps 194-198 (New) and Old 04/05/06 are translation faults (DFSR status 5) at addresses inside the allocated heap. Most hit the faulting thread's own heap-resident stack: dump 198 FAR=SP-24, dump 06 FAR=SP-0x28, 195/197 FAR=SP. `free()` never unmaps, so the heap mapping had been released, and the only linked releaser is `__libctru_exit`. The faulting threads are victims of a process exit started on another thread.
  - Before the 18 Oct guard, `fh_platform_panic` ended in `exit()` (verified in the 88915 disassembly), and the `salCtrlDsp`/`salDeactivateVoice`/assert `abort()` went to `_exit`, then `__libctru_exit`.
  - In 4f58 and 7b980, the disassembly shows `exit` wrapped, `abort` replaced, and `fh_platform_panic` ending in `svcExitProcess`. Only `main` and `fh_platform_quit` call the real `exit`, after joins. This dump class cannot recur; the primary cause will now appear in `panic.txt`.
  - Evidence: `tools/classify_console_dumps.py` writes `console-dump-classification/result.json`.
- **Old save-load crash (dump 06, 88915): cause identified, fix deployed, retest needed.** The primary is that run's `panic.txt`: "Native raw AUX queue capacity reached".
  - Mechanism: 88915's `fh_audio_output_step` invalidated the output clock whenever the NDSP dropped-frame count changed, and only a full underrun re-arms it (`audio.csv` shows none). With the clock invalid, `fh_audio_pcm_aux_window` returns nothing, so the AUX consumer stops while the producer keeps pushing, overflowing the 64-slot queue.
  - cfa8 removed the dropped-count condition; it is in 4f58 and 7b980.
  - Residual: the clock still invalidates without an underrun on a nonconsecutive callback or `out.end>queued_end`, and that would still overflow raw AUX within about 0.3 s.
- **Old dump 05 (a618):** primary is the actor-texture linear-reserve panic that 7d87 fixed. **New dump 194 (22e57bb):** `panic.txt` reads "Native morph resident budget"; the audio thread faulted reading sample bytes after `exit()`. The sample-UAF theory is not needed for 194.
- **SnowHorn cave rescue crash (New, dump 198, b32f).** Victim fault as above. `panic.txt` was stale, so `fh_platform_panic` never ran, and the primary was an unguarded `abort`/`exit`.
  - The linked abort callers were `salCtrlDsp`, `salDeactivateVoice` (seen firing on Old cfa8), assert and C++ terminate.
  - The confirmed defects on that path, timed-voice EOF retirement (c2b08) and the stop boundary (4f58), are both in New 7b980. Needs a SnowHorn rescue retest on New.
  - The rescue route is mapped (key at rom310). Emulator combat remains the blocker.
- **Old Arwing-segment crash.** Panic in `salDeactivateVoice` (`hw_pc.c:855`) when `fh_audio_source_stop` returned false. With the `4f58` fix (in d317), one Old session through the Arwing segment on 19 Oct ended with a normal Select exit. `panic.txt` on SD (`caller=0014f6d0`) is stale: that address is not a call return in the d317 ELF.
- **Old Arwing audio stutter and erratic motion (d317 hardware, 19 Oct).** CPU audio mix per 160-frame (5 ms) block rises from avg 5.0 ms to 20-40 ms, with max 0.1-0.8 s (frames 2835-3001). Native DSP `declined` voices go 2 to 262 over the segment. Declined voices fall back to the MusyX CPU mixer. Decline reasons are not counted: `route()` (filter, rear/aux volumes, studio graph), sample registry miss, or no free DSP channel. Even outside Arwing the CPU mix average is about 5 ms per 5 ms block, so there is no headroom.
- **Arwing audio overflow cause (emulator, audio-decline-v3 "cockpit" run; that run actually holds the Game Select menu with the cockpit backdrop, NOT the Arwing flight; Arwing numbers come only from the Old hardware log):** every decline is "no free DSP channel" (22 channels, 2..23). Full-channel census over 87 declines: 87% ARMED (playing), 12% WAITING retirement, ~0 ACKNOWLEDGED. So the game's polyphony exceeds 22 and the overflow is mixed by the MusyX CPU path, which on Old shares a 30% core1 grant with NDSP. hw-followup-v1 requests 80% (emulator granted 80). Not hardware-confirmed. A remaining option is to drop or steal for overflow voices instead of CPU mixing.
- **New Select-exit hang (d317 hardware, 19 Oct):** at shutdown `fh_pica_shadow_image_capture_exit` queued a 128x128 RGB565->RGB8 DisplayTransfer of a shadow image (VRAM 0x1f380000 -> linear); the GX queue shows it submitted but never completed ("GPU wait exceeded three seconds", pending=1). Removed from shutdown in hw-followup-v1.
- **New crash dumps 199-201** (3dsx_app, title 000400000d921e00) match neither d317 nor 7b980 ELF code windows: not attributed to Foxhollow. 200/201 are NULL reads.
- **Blur filter hang:** not reproduced in the emulator with blur enabled from startup (1068 captures, no stall). The console session log was overwritten; hw-followup-v1 keeps previous-native.log for the next occurrence.
- **batch2-v2 `b7b6d76c28b4` (built 19 Oct, package `16a78078b31b`, not yet deployed; Old ftpd closed at deploy time):** hw-followup-v1 plus live-scene conversion, `0x2000` fade geometry no longer queued or drawn (suspected flickering white/cyan ground sheet on Old, user screenshots 19 Oct), and non-fatal fallbacks for actor material set, fur mask and morph resident budget (draw skipped / no morph instead of panic). Emulator: Old user save with Old settings, New user save, Old/New play, Old palace all pass with 0 missing draws. New fancy settings on Old memory still end in a HUD image allocation panic (linear exhausted; not a shipping preset).
- **Old hardware on 40a70 (19 Oct):** core1 grant 80 granted; CPU mix 0.3-2.9 ms per 5 ms block (was ~5); declined 0-34 (was 262); gaps 17-28% of windows (was >100%). Intro movie audio still stutters: movie audio is produced on the main thread in step with THP decode, PCM queue drops to 2-4 x 20 ms packets. Fix needs movie audio read ahead of video decode.
- **Old SD tidy (19 Oct):** 251 backup/uploading files moved to `/3ds/foxhollow/backups/` (report `artifacts/console-test/20261019-221555-old229-backup-move`); `deploy_console_setup_direct.py` now writes backups there.
- **Old ground sheet (hardware only, 19 Oct):** flat cyan/white sheet near the ThornTail river; survives removal of `0x2000` fade geometry (batch2-v2 on hardware). Not reproducible in the emulator at the same spot (walk route `--walkpath=1610:-60:90,1700:-30:100,1800:0:0` from the Old save reaches it). Water-coloured, appeared with world water slices; batch2-v4+ skips family-5 world water on Old (`fh_hardware_is_new`). Root cause on hardware still unproven.
- **Ground sheet ROOT CAUSE (20 Oct, hardware frame capture `console-test-app-results/20261020-013302-229-sheet-capture`, analysis `research/sheet-capture-analysis-1`):** the sky/cloud dome (cloud program, VSH entry 276, 59 vertices at radius ~10000) sits at clip z/w ~ 0, i.e. exactly on the reversed-depth FAR plane, and also crosses w=0 behind the camera. The console clipper (f24) clips these straddling triangles inconsistently frame to frame, producing a sky-coloured wedge from the horizon to the bottom of the screen; the emulator clips in f32 and never shows it. Fix (batch2-v20): when the cloud program is active, `scene_camera` uploads projection row z minus 0.001 x row w (z' = z - 0.001 w), pulling the dome inside the far plane; sky depth range is fixed so written depth is unchanged. Emulator sky visually identical. Hardware confirmation pending.
- **Old ground sheet is NOT water or fade geometry (20 Oct):** still flickers on batch2-v5 with Old world water skipped and `0x2000` removed. Every sheet frame has the large ThornTail dinosaur next to the camera. Hardware-only. Next evidence: Select frame capture on the sheet frame, then `analyze_frame_capture.py` (vertex execution with Azahar f24 for all draws; ranks near-plane crossings and projected size). Emulator baseline: world draws (entry 278) routinely cross the near plane harmlessly.
- **Movie audio worker/read-ahead attempts rejected (20 Oct):** running the movie DMA callback on the audio worker (with locked queues) raised starvation (worker drains the output queue ahead of just-in-time decode). Audio-only read-ahead with partial `DVDRead`s panicked (`Native audio callback fault=13`): the prepared movie pack serves whole-frame reads only, so partial header/audio reads return the wrong bytes and THPAudioDecode overruns. Both reverted. A real fix needs movie audio stored/readable separately in the prepared movie pack. Hardware (b2-v5): 561 audio buffers for 320 video frames (movie runs ~half speed on Old), mixer starvation 2, stutter comes from main-thread-only pump cadence.
- **Audio fault 13 = late DSP service (20 Oct):** `fh_audio_dsp_aux_service` resets bank state when `ndspFrameId` skips, so the next AUX block has a NULL window while still carrying wet samples; `effect_mapped` then raised fault 13 and panicked. Any long stall can trigger it (emulator v21/v22 at movie exit, audio-decline-v2 cockpit, Old229 palace 18 Oct). batch2-v23 converts it to a logged permanent reverb bypass on bus 0 (wet zeroed, raw ring drained). Hardware unconfirmed.
- **Movie read-ahead ring REJECTED (hardware v23, 20 Oct):** Old intro movie fell to ~144 ms/frame (d317 ~28) with stutter unchanged; movie queue never starved (starved=0) while the channel-0 output queue sat at 1 packet. Removed in v24.
- **Old palace regression (v23 hardware):** 46-50 ms/frame vs 38.9 (d317). Core1 audio mix 3.0-3.3 ms per 5 ms block in palace; the v20 particle worker shares that thread. Worker made opt-in in v25 for an A/B; ground sheet still present on v23 (sky-dome fix did not resolve it).
- **Ground sheet, revised cause (20 Oct):** hardware PICA clips only depth; x/y extents after near clipping are huge for the cloud dome (~4300x screen) and near-camera ground (88-264x). Emulator clips x/y exactly (sw_rasterizer clipping_edges), so it cannot reproduce. batch2-v26 adds a user clip plane at eye depth 1000 for cloud-shader draws. Hardware unconfirmed. CPU near/guard clipping of world draws (`tools/world_clip_patch.py`, batch2-v27/v28) REJECTED: correct in emulator but ~12 straddling chunks and ~1000 triangles/frame reach the CPU path (near-plane straddle dominates; guard 8x vs 32x barely differs) with 96 KB arena overflow; ground ratios of 88-264x are ordinary for 3DS games, the dome (~4300x) is the outlier.
- **Core-1 grant 80% REJECTED (20 Oct hardware):** user said >30% does nothing; logs show GPU busy/frame 10.6-15.2 ms at grant 30 vs 23-29.7 ms at 80, palace 38.9 -> 48.2 ms, SD 3x slower. GSP/FS live on core 1. Default back to 30 in batch2-v33 (`core1_grant.txt` A/B override).
- **Intro movie I/O bound (20 Oct):** bundle movie is 201 MB RGB8 (~2.7 MB/s); Old SD gives ~3-4 MB/s, main blocked ~92% of the movie. v33 prefers the 37 MB ETC1 `starfox-pica.thp` on Old. Movie pipeline cap 6 (v24-v29) caused the Mapped PCM panic; removed.
- **River sheet (20 Oct analysis):** wedge = world path draw 11 on screen, but its hardware color (172,254,254) cannot come from its captured inputs (G max 229 before fog). Dome clip (v26) and sky z-shift (v20) both failed on hardware. Suspect GPU reading memory that differs from the exit-time snapshot (CPU/GPU lifetime race); batch2-v34 adds that probe (frame_capture.txt + Select; `tools/analyze_race_probe.py <dir>`).
- **River sheet, v39/v40 (20 Oct):** wedge is sky showing through where path draw 10's near-plane-crossing triangles are not rasterized (hole frame drops tris 0-1, normal frame renders crossing tris 3-4; clipped extents ~40x NDC). Race probe: 0 pages changed (not a race). v38 no-cull and v40 PICA user clip plane at 1.01x near (GPUREG_FRAGOP_CLIP) did NOT fix it. batch2-v41/v42: CPU near-plane clip (`tools/world_near_patch.py`) for world draws whose bounds straddle z+w=0: in-front triangles redrawn from original vertices via an arena index subset, behind triangles dropped, crossing triangles clipped to float-layout vertices; straddling draws leave the stream/packet path. Differs from rejected v27/v28 (all triangles of 12 chunks through CPU, guard band, 96 KB overflow): near only, only crossing triangles emitted. Emulator river walk: ~8.7 draws, ~90 clipped and ~630 kept triangles per frame, 0 fallbacks with 2x192 KB arenas and 8192-vertex cap; images match v40. Hardware flicker and cost pending.
- **Load time (21 Oct):** ThornTail save load on Old hardware (v39) is 13.6 s; SD reads in that window 950 calls / 19.9 MB / 8.5 s (~9 ms per ~21 KB call, 2.3 MB/s) versus ~10 MB/s for the movie's few large reads, so per-call cost dominates. DVD (disc.iso) is only ~5 MB of it. Per-file stats (`sd-files.csv`, v43+; trace `fs-reads.bin`/`dvd-trace.csv` with `dvd_trace.txt`): Old Fast session reads the texture pack in 1075 calls / 16 MB (hardware v39 window log: 1104 read_calls, 16.1 MB) and the performance scene pack in 415 calls / 3.2 MB. Texture lookups are random across the 92 MB CRC-sorted pack and go through a 32 KB stack stage (`FH_ASSET_STAGE_BYTES`); each texture re-reads its source bytes from SD to rule out CRC collisions, then reads the payload. Merging contiguous reads in recorded order only cuts 446 to 406 calls. Azahar models SD as a flat ~5 MB/s, so read-strategy timing needs hardware: batch2-v44 adds launcher Diagnostics > "SD read benchmark" (`tools/sd_bench_patch.py`, writes `/3ds/foxhollow/sd-bench.csv`: random/sequential 4 KB-1 MB, unaligned, stdio, two-thread, and the recorded texture-load pattern as recorded/merged/offset-sorted). Harness: `run_v5_checks.py play --sdbench`.
- **SD benchmark, hardware (21 Oct, v44, Old229/New223 within ~10%):** fixed ~3.7-4 ms per FSFILE_Read plus ~0.093 ms/KB (10.7 MB/s peak). Aligned random 4K 4.5 ms, 32K 5.7, 128K 14.2, 1M 94. Unaligned 32K 10.9 ms (2x). Default-buffered stdio unaligned 32K 19.9 ms. Sequential = random; offset-sorting the recorded texture pattern = no change; two threads = no gain (SD serializes). Recorded texture pattern 6.5 ms/call. Results `console-test-app-results/20261021-v44-{229,223}-hw/`. Install recipe hashes `platform_texture_pack.c` and `fh_texture_pack.h` (textures stage): editing them forces an on-console texture re-prepare, so v45 swaps the pack FILE* from `platform_pica_material.c` instead. Emulator v45: scene pack 415 -> 130 calls; texture 1075 -> 1074 calls (2x read-ahead was tried: +9 MB, -27 calls, removed).
- **v44 hardware user report (21 Oct):** river ground still flickers (CPU near-plane clip did NOT fix it; v41/v42 hypothesis rejected), ice wall still invisible, audio still buggy.
- **River wedge, revised cause (21 Oct, v44 Old capture `console-test-app-results/20261021-v44-229-hw`, analysis `research/river-v44`):** with the world CPU near clip active, world draws have 0 near-crossing triangles, yet the wedge persists and is fully covered by path draw 14 (tex 0x204f0e00, moderate extent). Draw 9 (cloud program, entry 276) is a depth-only pass (DEPTH_COLOR_MASK 0x1071: no colour, depth write, GEQUAL) with 15 near-crossing triangles drawn before the world; material `cloud_state` without bit 8 = cloud depth pass (`PICA clouds: depth_draws` 167 on hardware, 0 in emulator walks). Near-crossing dome depth punches the path (sky colour; black when sky colour off). batch2-v49 skips cloud depth-only draws and strips depth writes from cloud colour draws.
- **Distortion mask GPU hang (fancy-fx-v1 hardware, 21 Oct):** New223 hung at frame 207 right after start with blur/heat/distortion on; watchdog wrote quarantine. First unfinished GX entry is again the 0x4f0-byte distortion mask list (`distortion_make_mask`), after the v54 depth TextureCopy. Registers, vertex data (6-vertex quad, 40-byte stride) and the 11-instruction VS (ends in END, 5 outputs) all decode as valid. TEX0 = `motion_history` (aliases live colour buffer 0x18000000, 256x512 RGBA8), TEX1 = depth copy 0x1815DC00, target `distortion_mask` 0x183A1000 512x256 no depth. The v54 depth copy did not fix it. Root cause unknown; needs hardware bisect. Heat/blur never appear as the stuck list. Results `console-test-app-results/20261021-fancyfx-223`.
- **v53 hardware (21 Oct):** Old229 session `20261021-v53-229`: cloud depth pass occurred (`depth_draws=157`) and all 157 were skipped by sky_depth_patch with no flicker reported, confirming the cloud depth pass as the river wedge cause. ThornTail save load window 10.5 s (v39/v44 13.6 s), 751 reads / 20.2 MB / 5.6 s read time (was 951 / 19.9 MB / 8.5 s). New223 `20261021-v53-223`: 114,938-frame session through DarkIce Mines with no panic (rigid shadow fix holds); it ended in a GPU stall after enabling blur/heat/distortion. A second session stalled after enabling DISTORTION EXP: first unfinished command list is the distortion mask pass (512x256 RGBA8 target at 0x183A1000 sampling the live colour buffer 0x18000000 and depth buffer 0x18300000 as RGBA8, 6-vertex quad, consistent 5-output shader map). Distortion requests never occur on the emulator DIM walk route. Analysis `research/distortion-hang-v53`.
- **Experimental effect GPU stall (21 Oct):** the stuck command list is `blur_capture()`'s depth capture draw (target motion_history 512x256 in VRAM bank B; unit 0 = live colour buffer 0x18000000, unit 1 = `fh_pica_depth_view` over the live D24S8 depth buffer 0x18300000, same bank as the target). The depth view header states it was only exercised in an emulator fixture. Heat, blur filter and distortion all use this draw; motion blur (colour-only capture) runs every frame on New without stalling. batch2-v54 copies depth into a bank-A VRAM texture first. Emulator: 1869 depth captures, pass. Quarantine verified in emulator (captures 1869 -> 0 with `1610612736 32`). Hardware A/B pending.
- **Low-FPS audio stutter (21 Oct, user report: audio sometimes stutters below 15 FPS):** synth/music mixing runs on the audio worker, but AI streamed audio (ADPCM, channel 1) is decoded only in the main-thread `fh_audio_pump` and was queued as 8 items of 18 blocks (504 frames at 48 kHz, ~84 ms total), so frames slower than ~84 ms underrun it. The game advances stream position by frame time (`audio_stream.c`), not `AIGetStreamSampleCount`, so deeper decode-ahead is safe; volume is applied at decode, so fades lag by the buffer. batch2-v55: 54 blocks per item (~252 ms).
- **River wedge VALIDATED by user on batch2-v49 Old229 (21 Oct): ground flicker reported fixed.** Caveat: that v49 session logged `PICA clouds: depth_draws=0` and `sky depth: skipped=0 stripped=0` (cloud depth pass not exercised), so v49's guard-band world clip and the cloud depth-pass skip both stay in; do not remove either without a hardware A/B (`sky_depth.txt`=1 restores the cloud depth pass).
- **Canonical source and build (21 Oct):** the working tree now equals the batch2-v55 shipping source (47 files folded from its source-snapshot; previous copies in `private/root-before-v55-fold/`), plus publication fix-ups (`tools/publish_source_fixups.py`: vendored citro3d `internal.h`, provenance headers, MusyX assembly removed). 52 unshipped WIP files moved to `native-3ds/wip/` (not compiled; `build_arm_support.py` globs only `native-3ds/source/*.c`). From-scratch flow: `python tools/fetch_sources.py` (pinned checkouts incl. `vendor/foxhollow`), `tools/bootstrap_foundation.py`, `setup_linux.sh`, then `linux_exec.sh python3 /work/tools/build_console_setup.py --game-library` (full 794-unit game library, console-setup app, source snapshot, package). Verified 21 Oct: package `foxhollow-iso-setup-1a0cc085300ded34`; emulator Old river walk and New DIM (Fancy) pass (`research/pica-scene-rewrite/fresh-v1-console-setup-builds`). Future changes go into the tree directly; `build_batch2.py` patch modules are history against the old base snapshot.
- **Console hosts (21 Oct):** console IPs are no longer in tracked files. `tools/console_hosts.py` reads `FH_OLD_3DS_HOST` / `FH_NEW_3DS_HOST` or the gitignored `private/consoles.json` (`{"old":...,"new":...}`); docs and artifact folder names use the aliases `old229` / `new223` (local artifact folders were renamed to match). `tools/linux_exec.sh` and `setup_linux.sh` derive the workspace from their own location. Root `LICENSE` is GPL-3.0 (required by the vendored melee-3ds GPL-3.0 code).
- **Console file transfer (21 Oct):** use `tools/console_files.py` from WSL (`bash lx.sh python3 /work/tools/console_files.py old|new ls | pull <label> logs capture bench dvd <name> | flag <name> <value>`). Console paths are relative to /3ds/foxhollow and joined in Python; output goes to `artifacts/console-test-app-results/<label>/` via `.part` then rename after a LIST-size match; per-file connection with resume (Old ftpd resets per RETR; ftpd has no SIZE). `flag` only writes whitelisted flag files. Avoid hand-quoted Git Bash FTP paths (MSYS path conversion and `\$` escapes misplaced files twice on 21 Oct).
- **New223 v49 crash (21 Oct, `console-test-app-results/20261021-v49-223-crash`):** panic "PICA projected shadow pose" after the DarkIce Mines bridge with silhouette shadows. Root cause: `pica_ground_shadow_override.py` passes `jointCount`; rigid models have jointCount 0 but one joint matrix in the original (`model.c` ObjModel_GetJointMatrix), and the port's actor/shadow draws already map bone==joints to the world matrix. Fix `tools/rigid_shadow_patch.py` + centre from world when joints==0 (`tools/crash_semantics_patch.py`). Panic audit (21 Oct, 422 sites): other wrong-assumption sites fixed in crash_semantics_patch (NULL authored shadow texture is legal; non-finite shadow query skipped like the original; hologram prepare without asset/materials). Disc scan of 1,232 blocks / 11,081 world shaders: no second layer other than type 9, max 10 animation frames, no mixed-format frames, so material coverage panics 30/31/32 are true invariants. Remaining capacity limits: HUD 256 pages (peak 216), DSP queues.
- **Inert SD switches (21 Oct audit):** about 20 switches are read only after `fh_renderer_init` returns for the minimal/PICA renderer (`platform_graphics.c:782`), including `efb_contiguous.txt` and `reflection_lod.txt` set on Old229; `render_disabled.txt` is opened and ignored. They do nothing in console-setup.
- **Ice wall VALIDATED by user on batch2-v49 New223 (21 Oct): ice reported visible.** Fix: family-4 deferred mask admits 0x100 (`tools/ice_deferred_patch.py`).
- **DarkIce Mines ice wall (21 Oct, New223 v45 capture `console-test-app-results/20261021-v45-223-icewall`, analysis `research/icewall-v45`):** DIMIceWall (DLL 452, def 367, model 1119) is an 864-byte hidden hit-box placeholder; the visible ice is map geometry. No draw covers the doorway in the capture. The deferred owner logged `family=4 flags=4000010d` and `40000104` skipped every frame: both carry 0x100 (SHADER_FLAG_PROJECTED_TEX_PASS, which in tex_dolphin.c only appends addSmallReflectionTevStage), outside the family-4 mask 0x400c001d. batch2-v47 widens the mask to 0x400c011d (reflection stage omitted). Emulator DIM save (`console-save/20261021-new223`): unsupported 622 -> 0, pass. v45 New texture reads: 1120 calls, 3.5 ms/call (v44 Old 8.6 ms/call). Models 223/259 "not admitted" have zero draws (no geometry), not a defect.
- **Movie audio starvation:** PC THP path (`dll_3e.c`) decoded at most one frame per VI post-retrace callback and resynced away the backlog, so movie audio fell behind whenever the game loop ran below the movie rate. batch2-v4 catches up (up to 4 per call, audio-only for late frames, video only for the newest). Emulator Old: starved mixer chunks 43 -> 31, with none after movie frame 560 (v3 kept leaking ~1 per 64 frames). Remaining starvation is during the first ~4 s of load. Counters `audio_buffers`/`audio_starved` are in the `THP video frames` log line.
- **World water translucency (batch2-v5):** slice water now blends at constant alpha 0x60 without depth writes, so the riverbed shows through. Still not the original screen refraction.
- **Prepared render worker does not apply to the PICA renderer (19 Oct):** `platform_prepared_worker.h` (native_worker.txt, New-only gate) only feeds `fh_minimal_world_group`/model/UI legacy paths; current PICA scene/actor submission is synchronous on the main core ("Native world: draws=0"). Enabling it on Old (batch2-v6/v7 experiment) changed nothing. A core1 submission worker for PICA scene draws would be new work; it is the main structural candidate for Old 30 FPS now that core1 has an 80% grant with audio using only part of it.
- **Old 30 FPS plan (19 Oct):** hardware frame on d317 is 38.9 ms = native_submit 11.6 + object_update 8.6 + object_render 6.4 + scene_render 5.5 + gpu_wait 3.1 + rest. The gap is ~5.6 ms. Candidates, largest first:
  1. Move PICA scene/actor command generation (native_submit) to core1: main thread records immutable draw descriptors (matrices, material ids, buffer handles); core1 owns citro3d command building. citro3d is single-owner, so ALL command generation for a frame must move together, with a frame-boundary handoff. core1 budget: 80% grant minus audio (0.3-2.9 ms per 5 ms block on hardware). Prior art: `platform_prepared_worker.h` descriptor ring (legacy paths only).
  2. Specular off in Old Fast (-2.8% main encounters, emulator); conflicts with the 15 Oct user override.
  3. Emulator profiles are not representative for Old core split: the emulator reports the audio worker on core 0, so audio functions appear in main-core counts.
- **Core1 particle-update worker (batch2-v19+, `tools/particle_worker_patch.py`):** `expgfx_updateFrameState` publishes the pool update as a job on the audio worker (core1); every non-static `expgfx.c` function, `Obj_FreeObject` and `Obj_FlushDeferredFreeList` join first; worker-side texture frees and partfx/ripple/splash/sfx calls are deferred and replayed on main at the join; worker has private `getCache` buffer and `sfaRand` state. Join from inside an inline-run job returns (state 3). Emulator Old palace: main-core encounters 15.05M -> 14.31M (-4.9%), core1 6.36M -> 7.25M; worker ran ~96% of jobs, main waits ~14 us per job, publish-to-first-join window ~2.4 ms. Not a hardware FPS claim.
- **Old option costs (emulator 8-frame palace, Old user settings, batch2-v4):** base 21.40M main ARM encounters; specular off 20.80M (-2.8%); environment off 21.30M (-0.5%). Option bit 1 is FH_GRAPHICS_ALL, not particles; particles are ~10% (update 5.2%, glow 4.2%) and interleaved with simulation.
- **Invisible small models (both consoles):** ids 801, 1198, 1200, 1256 (640 B, 8-vertex boxes), 259, 223, 319 are never in the prepared inventory (runtime-built or rewritten: 801 seen with two CRCs; several ids share CRC 5594caf8). They logged "PICA actor source not admitted" and drew nothing (user save: 602 missing actor draw attempts). Likely the invisible ice wall and missing pickup. **live-scene-v2** (base hw-followup-v1, `tools/live_scene_patch.py`, `build_live_scene.py`) converts unadmitted actor sources up to 16 KB at load with a bounded copy of `fh_install_scene` (64 KB meta / 128 KB GPU staging) and publishes them through the normal admission code. User save: 0 missing draws, coverage pass. Old/New play, Old palace, game-select backdrop pass. Not deployed (pending 40a70 hardware test). `run_v5_checks.py --save=<dir>` loads a collected console save.
- **Other d317 hardware reports (19 Oct, user):**
  - New hung when the blur filter was turned on.
  - Water transparency looks wrong, even on New fancy.
  - Flicker still visible on the Dinosaur Planet select screen.
  - Arwing dialog character portraits render incorrectly.
  - Old: an interactable object (Pick Up prompt shown) did not render (`last-frame-top.ppm` in the result folder).
- **Old d317 session texture stats:** 891 loads, 0 errors, 350 mip images, 4 skipped for memory, image peak 2.18 MB.
- **Effect route capacity panic (19 Oct).** There are only 2 route slots. Two reverb recreations within the playback lead (about 40 blocks) panicked with "Native effect route capacity".
  - `route-defer-acquire-v1` keeps the current route on BUSY and retries the next block. BUSY mutates nothing.
  - Host test `route-defer-acquire-test/result.json`, ASan/UBSan: unpatched, 9 of 20 schedules panic; patched, 0, with convergence to the newest source and every object freed.
- **New196.** memcpy in the audio aux 640 B copy. It is the same secondary heap-unmap signature, not a UAF.
- **Old startup GPU hang on Start (frame 206/211).** a6184 removed GPU jumps; whether it is fixed on hardware is unconfirmed.
- **HEAT enable GPU stall.** A 6-vertex combined capture is left unfinished. The shader contract is repaired; the physical result is unproven.
- **Shadow fetch hang** (fixed: short3 → short4 fills the stride padding). Physical recovery is unconfirmed.
- Waiting inside the synth lock can deadlock, so route backpressure must stay non-blocking. Deferral, not waiting, is the route-capacity fix.
- **Old `gpu_wait` is quantized by polling (19 Oct).** cfa8 hardware per-frame waits are exactly 0.02, 1.05, 2.08, 3.10 or 4.14 ms, because `acquire()` polls `C3D_FrameBegin(NONBLOCK)` with 1 ms `svcSleepThread`.
  - citro3d starts the queue only at `FrameEnd`, so the whole of each frame's GPU work (14.7 ms) runs as a tail behind the next frame's object update.
  - `route-defer-acquire-v1` waits with `gxCmdQueueWait(queue, 1 ms)`, which blocks on GSP events and wakes on P3D completion. The New shared-worker path keeps its sleep.

### Rendering defects and missing effects (n3ds fancy correctness)
- **World water and fade geometry were never drawn by the fast world path (found 19 Oct).**
  - Why: `fh_render_floor.h` hard-codes `fh_render_budget_strip=fh_render_budget_world=1`. For callback-free blocks, `renderMapBlock` therefore skips the original transparent and water instruction streams.
  - The deferred replacement only drew family-4 surfaces passing `fh_pica_deferred_surface_flags`. It dropped water (family 5, e.g. ThornTail `c058000c/d`, `c058080d`) and `0x2000` surfaces, which `addShadowFalloffTevStages` fades by depth relative to the player (walls between camera and player). The palace dropped about 12 per frame.
  - Fixes:
    - world-water-slice-v3 draws water through the native slice path, with the water bit allowed while binding and the normal depth range (near 0, not the transparent -0.01 bias, which lifted water over the ground).
    - world-fade-v1 draws `0x2000` geometry. world-fade-v2 approximates the original per-pixel fade per surface.
  - Evidence: `world-water-slice-v3-play-new-dsp-actual-save/before-after.png` shows a blue pool in the SnowHorn dungeon that was missing. ThornTail matches the desktop reference (no sheet over the grass).
  - Still open: the original water is a refraction of the screen (reflection texture warped by two caustic indirect lookups, plus ambient/8). The native version draws the material texture with vertex alpha. The `0x2000` fade is per-surface, not per-pixel.
  - Reference renders of the original exist under `artifacts/desktop/*` (desktop build).
  - **Cost (19 Oct):** world-fade-v2 Old palace main-core encounters 14,803,579 to 15,112,156 (+2.08%, 8 frames) versus 7b980.
    - About 2.8 extra slice draws per frame for the `0x2000` geometry, about 13K encounters each.
    - The cost is bind_scene_pipeline, uniform/context republish and the per-draw `fh_pica_scene_invalidate`.
    - Batching consecutive slice draws, or avoiding the full invalidate, is the obvious reduction.
    - These are instruction counts, not hardware time.
- **Sky renders flat on hardware.** The full cloud shader gives zero-variance blue; it resembles the C-menu shared-shader defect. The direct-UV cloud route was later accepted by a physical probe and is in actor-live-v2. Check whether that build is deployed.
- **C-menu shared 511-instruction shader** drew only a 256-pixel strip on hardware. The separate 43-instruction UI shader fixed it. The root cause is unknown. Do NOT apply direct UV globally to the fur and specular paths.
- **White Arwing and flickering trees (hardware only).** Turning SPECULAR ON hid the problem. The root cause was never established. The Arwing's environment-bump (NBT) family is unsupported, and its basis is non-orthogonal, so do not normalize it.
- **Old: static bottom-screen UI intermittently missing.** The cause is unproven and could be emulator LCD caching.
- **Water:** indirect distortion, filtered 5-bit bump alpha, dynamic ripples, droplets and fragment fog parity are missing. Splash reflection uses one undistorted sample where the original has three indirect matrices.
- **Reflected particles:** indirect distortion is missing. Distortion has never been visually triggered by the game.
- **Shadows:** a single hard plane only. Dynamic and object receivers, finite volume, softness and self-shadow are missing. The shadow feature costs about 0.72 ms of CPU against a 0.5 ms gate.
- **Fur** looks weaker than the original: wind, length masks and indirect displacement are missing. Fur z-fighting has been reported.
- **Actor height fog** is incomplete: environment, specular, dynamic and morph families are excluded. The `o3ds-fast` preset excludes height fog, so that work does not help Old Fast.
- **Effect toggles OFF still build load-time plans.** Partly fixed in v5: projected-shadow plans (up to 325 KB) and collision receivers (410 KB linear, 118 KB normal) are no longer built when unused. That covers every preset except n3ds-fancy.
  - The shadow image VRAM (98 KB) is still held.
  - Turning silhouettes ON mid-session leaves already-loaded blocks without receivers until they reload.
- **Old normal heap is pinned at the 85% `normal_reserve` line (19 Oct).** `scene_meta_allocate` fills normal heap with metadata to the line, then spills to linear. Any later small essential allocation then fails.
  - Example: a 112 B actor plan for asset 1199 was rejected by 61 B, and the actor did not draw (coverage site 35).
  - v5 leaves 64 KB of normal headroom before spilling metadata to linear.
- **Blur filter** has never been activated in gameplay.
- **Distortion particle mode 4** is missing. Two warped-noise/decal materials are missing. Moon coverage and dynamic particle quads have never been exercised.
- **Cave haze** varies with particle phase. This is NOT proven to be a defect, so do not change mist alpha.
- **Multi-second SD stalls on hardware** (5 to 10 s gaps; about 1.3 s during GCI saves). A fresh run with the burst tracer is needed.
  - In the emulator, a static gameplay camera does 0 SD reads. The steady 30-per-second reads (about 3 MB/s) belong to the title/menu movie and stop at save load.
  - So gameplay hitches need a moving trace: hardware `sd-bursts.csv` from normal play, or a scripted walk.
  - Native material images are read synchronously from the texture pack at acquire time (`fh_texture_pack_read`).
- **Distance flicker (user report, 19 Oct: pop-in and flicker).** Native images have no mip levels; 203 of 592 SwapStone Hollow textures request mips.
  - citro3d's `C3D_TexGenerateMipmap` is CPU-only and cannot handle ETC1 pack images.
  - Mips must be baked into the install pack (reconversion) or generated per format.
  - **texture-mips (20 Oct, in test):** installer records levels as FHTXSPL5 spool records and assembles FHTXPK5 (unchanged directory plus an FHTXMIP1 chain section). Runtime builds citro3d mip chains at acquire.
    - Patches `tools/texture_mips_install_patch.py` and `tools/texture_mips_runtime_patch.py`; builder `tools/build_texture_mips.py` (base world-fade-v2). Only the textures stage recipe changes, so each console re-prepares textures once.
    - Host test `tools/test_texture_mips.py` (texture-mips-test-v7) passes: resume inside mip encoding and mip assembly, exact levels bit-identical to the oracle, CMPR levels quality-gated (a failed level ends the chain), runtime lookup and reduced-base suffix, 4/4 corruptions rejected.
    - Whole disc: 1397 chains, 4790 levels, about 7.0 MB of mip data, about 50 KB runtime directory.
    - Mips are admitted only with 1 MiB spare in both the 4 MiB image budget and linear memory; otherwise the image loads without mips. Emulator image peaks are about 1.86 MB.
    - The game sets LOD bias -2 on mipped textures; the runtime clamps it to -1. Tune after visual comparison.
    - Emulator checks use a synthetic fixture: `tools/prepare_texture_mips_full.py` (host whole-disc conversion), `tools/prepare_texture_mips_fixture.py`, and `run_v5_checks.py --fixture=`.

### Audio
- Quiet intervals remain unattributed (startup, about 41 s and about 49 s into the opening, and 27-30 s on ThornTail).
- Native wet delay is about 4.8 ms late.
- Only DPL2 reverb is supported.

## 5. Performance picture

### Old 3DS physical history (palace benchmark unless noted)
| Date | Build | FPS | Frame |
|---|---|---|---|
| 7 Oct | early | 3.03 | |
| 9 Oct | first native actors | 18.4-20.0 | |
| 14-15 Oct | Old Fast, ThornTail | 9.9 | 107 ms (PREP 31, DRAW 31-38, OTHER 18, SIM 16-17, PRES 13.6) |
| 16 Oct | 4563 | 13.5 | 74 ms (native_submit 35.5) |
| 17 Oct | 6d78487 (audio queue) | 23.0 | 43.5 ms |
| 17 Oct | 88915 (actor palette CPU) | 26.40 | 37.9 ms |
| 19 Oct | cfa8 | 26.36 | 37.9 ms, p95 41.3 |
| 19 Oct | d317a80e (texture-mips-v4), FRAME END | 25.68 | 38.94 ms: native_submit 11.62, object_update 8.61, object_render 6.36, scene_render 5.46, gpu_wait 3.13 |
| 19 Oct | d317a80e, EARLY | 25.82 | 38.73 ms: gpu_wait 0.03 but native_submit 13.06, object_render 7.36, scene_render 5.84 |

| 20 Oct | batch2-v34 (grant 30) | 27.0 | 36.99 ms median: native_submit 10.96, object_update 8.72, object_render 6.30, scene_render 5.35, gpu_wait 2.68, gpu_busy 14.56 (v31 at grant 80: 48.18 ms, gpu_busy 25.58) |

- **EARLY GPU queue rejected on hardware (19 Oct).** It removes gpu_wait (3.10 ms) but CPU phases slow by the same amount (likely memory-bus contention with the running GPU). Net -0.2 ms, within noise. Keep FRAME END.
- **d317 is about 1 ms slower than 88915/cfa8 on Old:** native_submit +0.9 (world water and `0x2000` fade slice draws), gpu_wait +0.8. The fade/water draws need an Old Fast gate or cheaper submission.
- Results: `artifacts/console-test-app-results/20261019-195102-229-texture-mips-hw/runs/00-resident-palace-{000000029908bfcc,0000000425259958}-0`. Old ftpd resets after about one RETR; use one connection per file.

The current gap is about 4.5 ms to reach 33.3 ms.

Breakdown (88915): nativeSubmit 10.7, objectUpdate 9.3, objectRender 6.2, sceneRender 5.3, GPU wait 2.3 ms. GPU pending time (about 14 ms) overlaps the CPU and is NOT additive.

The biggest Old wins were structural:
- DSP audio replacing the CPU mixer. resampleVoice and mixRamp had been about 26% of main.
- Audio worker on CORE1 at a 30% grant, the default. CORE0 measured 15.2 FPS against 19.75 on CORE1.
- The spatial audio control queue (audio update 6.45 → 0.10 ms).
- The texture thrash fix (1 → 9 FPS).
- Resident shader leaves, the collision candidate cache (-64% collision), and actor stream/palette work.

### New 3DS physical
- Palace about 54-55 FPS, cave about 32 FPS (9-10 Oct).
- Cave with shadows ON: 20.5 FPS, with 35 ms of GPU service.
- New is GPU-bound in heavy scenes; GPU wait dominates.

### Hot functions (emulator 8-frame ARM instruction encounters, 7b980; counts, not cycles)
| Function | Share |
|---|---|
| Particle update | 7.8% |
| Particle draw (drawGlow) | 6.4% |
| Uniforms | 3.9% |
| Actor append | 2.5% |
| Joint matrix | 2.6% |
| Anim decode | 1.2% |

Particles total about 14-16% of main and are interleaved with simulation, so they cannot simply be skipped.

### Do-not-repeat list (rejected; details in the period files)
- **Particles:**
  - Sparse/live-slot update traversal (+6 to +10%).
  - GPU particle lifetime/alpha.
  - Packed store, direct-member pack, outline helpers, and per-draw material cache.
  - Early visibility v1.
  - Same-stride compact producer.
  - Common *draw* specialization: code doubled for -4%.
- **Actors and skin:**
  - Per-call actor pose row cache (rejected 3 times).
  - CPU-skin copy-path microtuning.
  - CPU-skin integration V3/V4. V3 was +6.45%; V4 is unproven.
  - Actor cache conditional write.
  - Camera-space palette/view poses.
- **Collision:** plane/edge geometry cache (4.5% hits, +30%), geometry cache v3, spatial index, admission V1/V2, old plane cache / bitset cursor.
- **World and submission:**
  - Do not re-tune packet runs, batch sizes or admission scans.
  - Do not rediscover packetization; most draws are already packetized.
  - Resident command execution with small leaves.
  - Early queue start and queue batch: libctru `gxCmdQueueAdd` auto-dispatches, so these lost 10-17% and dropped the bottom UI.
    - Those numbers are Azahar wall timings, and Azahar runs GPU commands on the emulation timeline, so they cannot measure overlap.
    - The batch variant fixed the bottom UI.
    - Re-test ONLY on hardware via the queue-mode-v2 menu row; do not re-run emulator timing for it.
  - GPU command jumps/chaining, which failed on hardware.
  - Shared shader leaves and code-bank merges.
  - Uniform sparse scan.
  - Slice-draw state retention (slice-retain-v1, 19 Oct): keeping `bound` instead of the full `fh_pica_scene_invalidate` after deferred slice draws.
    - bind_scene_pipeline -9.8K and UpdateContext -11.3K, but UpdateUniforms +16K (8-frame Old palace), so net zero.
    - The per-slice uniform republish is intrinsic. Rejected.
  - Palette refine/scheduling.
  - Material packet publication.
  - Texture hint coalescing and image identity cache.
- **Shadows:**
  - Range BVH, Morton/spatial ordering, preflight, rigid visibility, and query context.
  - Depth-state GEQUAL+offset (failed on hardware).
  - The combined 65-instruction shadow program, which stretched silhouettes on hardware.
- **Geometry and LOD:**
  - Decimation and reduction (0.1 to 1% gains, damages surfaces).
  - Animated per-bone bounds.
  - Blanket actor LOD.
- **Audio:**
  - Decoded ADPCM block cache (+16%).
  - Packed sample history.
  - Sparse tails.
  - Native LTO V1/V2.
  - Audio stack growth.
  - Morph storage V2-V6. Never return to the 40-byte padded morph layout, which is corrupt on hardware.
- **Effects:**
  - Coarse water, wake, distortion and ripple grids, whose error was worse than omitting the effect.
  - Two-plan ripple bounds.
  - Blanket bump mapping.
  - Coarse per-vertex fur wind.
  - Blanket 256-triangle glow budget.
  - Height-fog per-material shader switches.
- **Movies:** RGB8 lossless with synchronous reads. Movie CRC gains seen under GCC do not hold under Clang.
- **Methodology:** the hidden-cost "OTHER" bucket is not GPU wait unless proven.

### Cost gates for optional effects
- CPU: +0.20 ms on New, +0.50 ms on Old per effect family.
- GPU: 1.5 ms on New.
- Deferred-surface submission: 0.5 ms, currently failing at 0.51-0.57 ms.

## 6. Engine, PICA and SDK facts (hard-won)

### PICA / GPU
- Vertex buffers must be padded to the full stride, because the fetch extent must cover the stride. This caused several hangs: water 20 B stride with a 16 B fetch, morph 32 vs 40 B, and shadow short3 vs a 32 B stride.
- Do not put L4/A4 textures in VRAM, because TEXUNIT1 corrupts them. CPU stores into VRAM crash; use GX memory fill instead. Never read back raw depth.
- VSH float-uniform writes alias GS registers c87..95 when GS is not exclusive, so GS uniforms must be republished.
- Bool uniforms need the queried location 0x68.
- Shader limit is 512 instructions. The main bank is at about 501-511, with all 96 float and 16 bool uniforms used.
- Picasso constant arrays are stored in descending order, so indexing needs a negative offset.
- DisplayTransfer needs a width of at least 64 on hardware; Azahar accepts invalid sizes.
- Stock `C3D_TexBind(unit, NULL)` dereferences NULL, so the patched `texture.o` is required. The stock gxqueue overflows at 96 aux calls per frame.
- GS→HUD in one command list hung Old hardware; split them with `C3D_FrameSplit`.
- Azahar differences from hardware: it keeps float32 internally (it is not a float24 model), processes GS in software (so it overestimates GS cost), rounds the combiner output before scale, lacks SVC 0x11 (except in the custom build), and HLE hardcodes `dropped_frames=0`.
- A D24S8 depth buffer sampled as RGBA8 puts the high depth byte in G, inverted (1-G). Software PICA texture addressing inverts T.

### libctru
- `__libctru_exit` unmaps the heaps before `svcExitProcess` and does not join workers. Fatal paths must call `svcExitProcess` directly.
- `threadJoin` returns early, so wait on the kernel handle and then call `threadFree`.

### Original game
- The decal viewport is near -0.05 / far 1. Projected shadows use reversed-Z scale -1.05, GEQUAL, and no depth writes.
- Fur interpolates STQ with a per-fragment divide.
- Mode-0 constant RGB is TEXC*KONST.
- `DECAL_LAYER` is 0x100000; 0x20000 is WATER_CAUSTIC.
- A zero light mask gives white, not ambient.
- `ObjModel_RelocateAnimData` must run before skin plans are published. The model resource version changes on every geometry write, so it cannot be used as a plan lifetime.
- The particle pool has 25 slots. Lightning updates the caller's RNG seed, so its CPU work cannot be skipped.
- The game uses one global reverb (DPL2). The AUX loop applies the current handler to the previous 160 samples.
- A normal save does not move the checkpoint; only the op31 `savePoint` trigger does. Enemy respawn timers are not saved.
- The root headers differ from the generated ARM structs. Derive offsets from the archived ELF DWARF.
- The ripple uses A/B offsets plus G coverage, while the wake uses B/G/A. They are not interchangeable.
- `gNewShadowBumpTexture` is static procedural content, generated once.

### Cores and memory
- New core2 is already used by audio. Never assume two idle graphics cores.
- CIA profiles: Old80/New124. A 15% reserve applies to normal, linear and process memory.
- Old: heap 47-48 MiB normal plus 12 MiB linear, with linear free often only about 1.6-3.7 MB, which is tight.
- New: heap 71.3 MB (about 54-57 MB used), linear 25.2 MB (about 16-18 MB free).
- VRAM free is about 4.7 MB. The main stack uses about 73 KB of 512 KB. Image budget is 4 MiB; scene metadata 4 MiB; GPU caps 6 MiB.
- Do not blindly grow linear caps.

## 7. Tooling and workflow
- **Layout (21 Oct, fork restructure):** the Foxhollow tree (upstream `33f3079`) is in the repo root (`game/`, `port/`, `extern/`) with the 3DS edits committed in place. `native-3ds/` holds the platform (`source/`, `include/`, `shaders/`, `citro3d/` patched files, `ctru/gxqueue.c`, `vendor/`, `home-menu/`, `host/`). The Python string-patching build and ~3,100 legacy scripts are gone.
- **Build:** `tools/linux_exec.sh make -C /work -j8` → `build/app/foxhollow.{3dsx,cia,cxi,elf}`. The Makefile reproduced the old Python build's 3dsx byte for byte (with the old build tag and meshoptimizer macro path). `BUILD_TAG` is sha256 of `git rev-parse HEAD`; uncommitted edits do not change the launcher build id. A no-op make takes ~1 min on the Windows mount.
- **Install recipe:** `native-3ds/include/install_recipe.h` and `native-3ds/data/install-recipe.json` are committed. Regenerate with `tools/install_recipe.py` (needs the ISO) only when converter sources change; it maps `native-3ds/vendor/meshoptimizer/` back to the old `reference/meshoptimizer/src/` names so existing keys stay valid (regenerated header byte-identical, 21 Oct).
- **PC conversion:** `make convert` → `build/fh-convert DISC.iso OUT`; ~5-6 min for the full disc. Original/performance/textures are byte-identical to ARM output; the movie differs only in LZ4 payload bytes (64-bit vs 32-bit LZ4 hashing), decoded frames/audio identical (same output as `install-movie-rgb8-full-v2`).
- **Shaders:** every program is a committed `.pica` in `native-3ds/shaders/`; `tools/shbin_link.py` (link / link-compact) joins multi-entry programs.
- **Host tests:** `make -C tests check` (109 targets, 21 Oct). Excluded tests and reasons are listed in the 21 Oct progress log entry.
- **Emulator:** `tools/emulator_check.py --app build/app --model old|new [--walk] --save DIR --settings DIR`. Old needs `artifacts/console-save/20261019-134345-settings` (New fancy settings panic on Old with "PICA HUD image allocation").
- **Hardware benchmark procedure (user):** install the CIA with FBI, then Benchmarks/playback → `00-resident-palace`, let it auto-exit, then reopen ftpd. Old has a CPU perf panel (PERF ON).
- **Deploy:** `py -3.13 tools/quick_deploy.py old|new` uploads `build/app/foxhollow.{3dsx,cia}` and checks sizes (~30 s on New). Run it in the background and stop it if no file finishes within ~2 min. Old's ftpd lacks SIZE (untested with quick_deploy). Do not retry in a loop.
- **Console files:** `tools/console_files.py old|new ls|pull|flag`.
- **Gotchas:**
  - Windows Python 3.7 lacks `removesuffix` and `math.dist`, so use WSL Python 3.
  - pyelftools needs `relocate_dwarf_sections=False`.
  - Normalize CRLF before hashing.
  - The C: drive frequently fills up; prune `artifacts/`.
  - If WSL hangs, a targeted `hcsdiag` kill has worked, but only with user approval.
