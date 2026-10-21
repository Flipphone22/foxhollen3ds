# Native packet submission without GPU jumps

16 October 2026. Candidate `a6184b997ac1`; hardware hang resolution is **unproven**.

The user reproduced the startup hang with `852f9538c147`. Fresh startup evidence
identifies that exact build. At retrace 213/GPU frame 206, the queue had seven
entries, three completed and six submitted. First unfinished queue 3 contains
65 draws. The new fog publication is present, including neutral c94 before the
first scene draws. It did not resolve the hardware hang.

The independently downloaded 262,144-byte command buffer matches the same range
in the interrupted CPU-memory transfer byte for byte. Only 7,721,536 of the
declared 10,485,760 memory bytes are available; there is no checksum/completion
proof. The strict full-input reader still rejects it. An explicitly partial
diagnostic decodes 92 draws/24 jumps, bounded to available bytes. Its vertex VM
executes 64 first-unfinished-queue draws: 15,820 vertices have observed inputs
and finite outputs, 19,337 have unknown inherited inputs. One geometry-shader
draw is excluded. This is not full GPU validation or a root-cause finding.

The VM's geometry-stage guard was also corrected: the pinned Azahar register
uses a two-bit enum, No=0 and Yes=2, rather than a bit-zero enable flag. No
production geometry shader was changed by that diagnostic correction.

The candidate isolates GPU command jumps, which had passed local tests but had
no physical acceptance for this implementation. It copies the already compiled
world packet payload into the SDK command buffer, removing only the terminal
return. Shader uploads use the prior tested load-compiled four-variant owner.
There is no game-side GX interpretation, per-frame geometry rebuilding, omitted
draw/effect, shader change, simulation change, extra worker, or added GPU wait.
This increases CPU command copying; no speedup is claimed. GPU jump publication
is absent from the linked executable. The retained world packet resources and
their existing fence ownership remain intact. Future hardware acceptance of
any faster transport must precede reintroduction.

Pinned Melee `command_cache.c` and `early_queue.c` were consulted: segment cache
publication, SDK queue ownership, and resource retention until completion stay
unchanged. The normal SDK path consumes the full prepared command sequence.

Evidence:

- `packet-inline-test/result.json`: actual copy owner under ASan/UBSan, 16,384
  size/alignment/capacity cases and 10,000 frame epochs. Lost-word, copied-return,
  and missing-capacity mutants rejected. The host packet prefix is extracted
  from the archive; SDK-dependent trailing metadata is not exercised there.
- The shader owner exactly matches the prior ARM SDK test source: 7,874 checks,
  1,612 comparisons, all 18 embedded programs, invalidation/capacity mutants.
  That unchanged evidence is reused rather than rerunning it.
- Isolated build changes two source files only; all 18 GPU programs, game
  archive, conversion recipe and Old80/New124 profiles remain unchanged.
  Root/archive restoration passes. `packet-inline-test/build-verification.json`
  checks absence of the jump publisher in the actual linked ELF.
- `artifacts/native/unified-packet-inline-v1-play-old-dsp/`: normal Start game,
  title/menu/profile/save load and Select exit pass, 83.79 host seconds. Final
  layout audit covers 169,362 draws without fault. Captures show cockpit video
  and saved cave/Fox/ladder/mushrooms/particles/HUD and were inspected. DSP LLE
  and capture exclude timing comparisons; parent also passes this emulator path.
- Sampled heap 39,767,384/50,331,648 B; linear free 2,279,936/10,485,760 B;
  VRAM free 4,723,712 B; stack 72,984/524,288 B; process
  69,595,136/83,886,080 B. Shader resident allocations are removed; world packets
  remain allocated. No console performance claim or complete rendering claim.

Reproduce with `tools/build_packet_inline_submission.py console-setup --revision
<new-revision>` and `tools/run_packet_inline_submission.py play --family
unified-packet-inline --revision <new-revision> --old`. Do not build root WIP.
Package `foxhollow-iso-setup-01820ea67e14f4e2.zip` includes app files only.
Deployment status must come from the transfer record, not this candidate record.
Use the existing normal app and **Start game** for hardware validation.

The first two upload attempts failed without replacing any installed files.
Windows active FTP reset during manifest read; WSL passive FTP completed the
first staged 3dsx upload but timed out reading it back. The resumable record is
`artifacts/console-test/20261016-120449-old229-direct-install/transfer.json`.
Its `installed` list is empty. Old229 therefore remains on `852f9538c147` until
an explicitly complete transfer supersedes this record. User was asked to
restart ftpd. The staging-only resume helper requires unchanged package/host/
installed manifest, verifies the attempted file before any further writes,
then verifies every payload before publication. Fake FTP tests cover success,
corrupted staged content and changed installed manifest; both faults reject
before mutation. No further network retries until ftpd is reopened.

Deployment subsequently completed after ftpd was reopened: the same transfer
record now reports COMPLETE, with all six staged and final files byte-verified.
Install `SD:/3ds/foxhollow/foxhollow.cia` through FBI; source `a6184b997ac1`.
Hardware resolution remains pending. Bounded REST read recovery compares every
received chunk against the exact known payload before advancing its offset.


## 17 October aligned CPU command copy

17 October NATIVE COMMAND COPY candidate; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(core-ID tooling); this turn PROGRESS(actual aligned ARM
copy/owner tests/shared build/linked checks/Old palace runtime). NoFTP/deploy.
InstalledOld5052 unchanged, latestphysical26.40484FPS; save-load crash OPEN.

Local 7d8eaf8def39 unified-command-copy-v1-console-setup-builds NOTDEPLOYED.
Only command-copy header plus packetleaf/programconfigure consumers changed.
ARM LDM/STM eight-word copy replaces libc for aligned nonoverlapping commands;
tail reads exact words. No command removal/interpretation/chaining, no new GPU
work/shader/worker/buffer, no allocation; BSS3266800 unchanged. CPU copy remains
necessary because direct command chains previously failed hardware. Pinned
Melee command_cache.c reviewed: existing per-segment cache publication retained.

Actual ARM312 helper cases include page-end access guards/ABI/full destination
canaries. Representative498word instructions1654->330,1294word4241->841;
zero length4->5. Not cycles/FPS. Actual host owner16384bounds/10000epochs,
3faultmutants rejected. Actual ARM SDK shaderowner7874checks/1612cases,
all18 currentembedded programs,3mutants rejected. Same source/embeddedprogram
baseline synthetic shaderupload4537->958instructions; source writer6577 is NOT
the deployed baseline. No fullframe gain established. Linked bothconsumers
contain intended LDM/STM; reversing only copy substitution exactly reproduces
parent owners. Gamearchivea336c7fd3bd1 andallGPUprograms unchanged; Old80/New124
profiles/root+archive restorationPASS. No extra normal boot/conversion.

Old palace exactbinary PASS91.97hostsec with corrected core-ID
emulator, audio core1/grant30, missingactors0/layoutfault0/drain/ownedexit,
saves/settings unchanged. Palace1602 inspected world/textures/purplecreature/
HUD, camera differs fromparent so NOT alignedimage or fullparity proof.
DSP LLE and phase variation exclude timing comparison. Budgets inacceptance.json.
Allowned53685disassembly,31587build,83242game TERMINAL; tests completed.
Evidence command-copy-{arm,owner,program}-test, tools build/test/run/verify/
record_command_copy. Next actual frame CPU work and physical save-load followup;
do not substitute helperinstruction ratio for fullframe/performance targets.



### Whole-window follow-up

17 October COMMAND COPY FULL-WINDOW comparison; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(implementation/build/runtime); this turn PROGRESS(serial
parent/candidate actual8frame profiles, exactcopy-byte workload verification).
No new gamebuild/deployment/FTP/assets/save/settings changes. Local7d8eaf8def39
retained, installedOld5052 unchanged. Physical26.40484FPS/37.87184ms latest;
hardware save-load crash OPEN. Do not call whole-window instructionratio FPS.

Parent5052/candidate7d8e exactELF render1301..1308 (scheduler1300..1307):
main16207459->16074418 encounters (-0.82086%); core16007482->5852686 varies
withaudio phase, notaudiooptimization. Parent/candidate executionPASS128.47/
122.12hostsec, saves/settings/drain/ownedexit/missingactors0. Rawbothfailed-audio-
core-placement retained: profiling emulator stillhasSVC11stub. Independent
exactELF executionaudit provesmixer core1; previous corrected diagnosticPASS
reused. No fullprofile emulator rebuild or extra GPU probe.

Actualcandidate ARM STM bulk+tail encounterschosen bychecked opcode inside
two exactlinkedowners: packetleaf144calls262656bytes; shaderconfigure184calls
371136bytes. EXACTLYmatches parent'scapturedmemcpyargs, dropped0. Copygroup
memcpy+twoowners1052928->630336 encounters (-422592); groupincludesothermemcpy
callers, notisolatedcopytiming. Overallnet-133041; unrelatedwork+289551 varies.
Largest increaseslabel96942,memset32102,particleupdate32062,drawGlow20608;
not evidence copy code made those routines intrinsically slower. Fullrecorded
scene mismatch:camera/player/simulation/particle/draw counts; doNOTnormalize
awaywork or attribute allnetdifference tocopy. Both1601images inspected:
world/textures/purplecreature/HUD present; camera phase differs, notimageparity.

Allowned14169parent/85640candidate/94235+34091audits TERMINAL. Newtools
profile_command_copy,compare_command_copy_work,analyze_command_copy_bytes,
record_command_copy_profile; evidencecommand-copy-arm-test/{arm-comparison,
profile-acceptance}.json andeachrun/{summary,audio-core-execution}.json.
No new allocation or shader changes; previouscorrectness/memorygates retained.
Keep safe copy improvement but insufficientalone for30/60FPS. Next revisit
existingunpromoted particleGPUclipping tradeoff with currentCPU/GPUbudgets,
review its recorded regression/phase variation first; don't blindly reapply.
No blocker; fullrenderparity/sustainedperformance/progression/endurance open.

