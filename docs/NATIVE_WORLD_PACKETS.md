# Persistent ordered world packets

16 October 2026. Local component under development, **not connected to
gameplay or deployed**. This builds on the resident-command execution experiment;
it does not make that experiment a performance win or complete the port goal.

## Real-asset grouping result

`audit_native_world_groups.py` reads the prepared original and performance scene
packs from the latest accepted Old palace fixture. It checks directory/payload
CRCs, group ordinals, main-stream coverage, and every section's material identity.
Both packs contain 1,259 nonempty main-world packets and 10,912 native draws.

There are **no adjacent same-material runs** to combine: those boundaries are
already grouped. Changing material alone therefore must not break a persistent
packet. Runs sharing authored vertex format and fog flag contain 10,457 draws
in runs of at least four. A maximum of 16 draws per packet gives a theoretical
1,409 calls versus 10,912, but this is only an inventory bound. Live fog flags,
special material owners, visibility, target and queue boundaries are not covered
by that static result. No draws were reordered, combined, omitted or rewritten.

Evidence: `artifacts/research/pica-scene-rewrite/command-group-audit/result.json`.
No private assets were changed or included in distributable files.

## Implemented component

`pica_world_packet.c` prepares up to 16 native draws in original order. Static
attribute/layout changes are compiled once; per-draw raster, sampler, material,
uniform and index commands remain explicit. It borrows already load-compiled
native descriptors; it does not interpret GX or reconnect the retired renderer.

Each entry retains a small normal-RAM copy of its last live parameters and
load-computed patch offsets. Updating changes only values that differ. It does
not rebuild geometry, copy the entire packet, or reconstruct SDK state per draw.
Unchanged inputs produce zero command-buffer writes. All live inputs are checked
before any entry is changed, including rejection of an invalid final entry after
an earlier entry has changed inputs.

Preparation and updates do not allocate, flush, submit or wait. Preparation is
allowed to modify unpublished storage on failure; the packet is invalid until
complete. Updates require the caller to have retired all GPU readers of that
storage. Dirty ranges are half-open and must be flushed before GPU use; texture
pixel publications remain separately owned even if sampler pointers are unchanged.
Sampler/raster objects are borrowed and immutable, as in the existing native
command owner. The packet ends with channel-1 return for the pending group-call
integration.

The pinned Melee split-flush/fence rules remain the publication model. The
application resource owner, hardware-mode budgets, SDK state invalidation and
visibility/eligibility logic are **not implemented yet**. The existing game has
no new allocations or rendering changes from these files.

## Current evidence and limits

The ARM test extracts the exact native compiler, retained command publisher and
header from `unified-audio-nativefx-v3-console-setup-builds`. It checks every
draw's register/uniform state with an independent command decoder, including
nonzero inherited stencil, TEV4 colour and camera uniform state. It exercises
64 updates / 1,024 draws: UV and colour changes, animated sampler selection,
raster variants, environment parameters and unchanged inputs. Five fault mutants
reject stale uniforms, wrong colour offsets, stale raster, bad vertex offsets
and omitted input preflight.

An initial fixture accidentally mixed the root's older draw header with the
archived publisher (different descriptor ABI), producing wrong reference offset
state. The harness now stages both dependent headers alongside the extracted
sources. This was a fixture mismatch, not evidence of a shipping geometry bug.

In this synthetic 16-draw test, the packet updater uses approximately **71%
fewer ARM instruction encounters** than rebuilding the retained native stream.
This excludes group call/return relocation, live material selection, culling,
cache maintenance, SDK handoff, game logic and GPU execution. It is not a
whole-frame or console-FPS estimate. GPU command volume increases about **8.6%**
over the retained publisher for these inputs, because per-material commands are
kept even when neighboring draws happen to share live values. That cost must
pass the actual GPU/gameplay budget before adoption.

The fixture's 16-draw packet uses 7,248 command bytes out of a 12,288-byte caller
capacity, plus a 1,868-byte CPU descriptor. These are component sizes, not a
measured game residency budget. There is no production allocation yet.

Exact ARM evidence and hashes: `world-packet-test/{result,acceptance}.json`.

The subsequent `world-packet-gpu-v1` local GPU run executes eight cases: four
material states, each through the archived native publisher and the persistent
packet. Across 512 draws and 16 calls to 16-draw packets, every paired RGBA image
matches byte for byte. Independent analytic colour expectations differ by at
most one byte. Actual two-texture materials, sampler animation, material colour
updates and half-alpha blending are exercised. Images were inspected; the
coloured grid retains all 64 distinct draw locations. This does not test game
geometry, source image parity, particle/shader transitions or SDK departure.

Packets are patched only after successful FrameBegin has retired previous GPU
work, then dirty ranges are flushed before executing their calls. The last
unchanged state writes zero packet words and exactly repeats the previous image.
The 3.116-second host runtime is test duration, not GPU throughput. No physical
cache/coherence, resource-lifetime or gameplay performance acceptance is claimed.

The command-call owner now accepts aligned prepared leaves up to 64 KiB and
validates actual leaf ranges when sealing. Its separate bounded interpreter
checks 810 sequences, with 162,238 ASan/UBSan checks and four rejected mutants.
The old 128-byte draw API remains supported. These tests exercise return address,
size and ordering; the game still needs its own resource and SDK-state owner.

The next ownership component is now implemented in
`platform_pica_packet_owner.c`, with a pure archived-source adaptation in
`world_packet_owner_patch.py`. It retains at most 256 pending return locations,
seals them relative to each actual SDK segment before its cache flush, and
advances a reuse epoch only when called after successful FrameBegin. Epoch
advancement rejects unsubmitted calls; it does not itself establish a GPU fence.
The SDK entry rejects pending shared state, marks draws used, and invalidates
the individual native draw cache after a group so it cannot retain stale state.

`world-packet-owner-test/result.json` passes 1,205 ARM checks for pending SDK
flags, capacity refusal, rejected foreign segments without buffer mutation,
segment-relative returns and retirement guards. This executes the real owner
helper and SDK-entry fragment using actual SDK definitions, **not** the complete
SDK begin/end state machine or GPU fences. The pure patch adds hooks at the
existing flush/FrameBegin sites but has not been applied in an application build.
The resource allocator and scene caller remain absent. In particular, a packet
used earlier in the same epoch must not be changed for another draw: preserve it
and use the existing native individual path or a separately retired packet.

## Real application integration, 16 October

`unified-world-packet-v1-console-setup-builds` integrates the component into the
actual unified startup application, from the accepted nativefx-v3 snapshot.
`world_packet_game_patch.py` adds a load-owned command group to the existing
native material lifetime; it does not reconnect the retired GX renderer.

During material loading, consecutive ordinary draws sharing a vertex format are
compiled in groups of four to sixteen. Each group borrows the already compiled
native descriptors and immutable sampler commands. A global 128 KiB linear cap,
24 KiB descriptor cap, existing material heap admission and 15% remaining linear
grant guard bound residency. Allocation refusal preserves the existing native
individual draws. There is no first-visible compilation or per-frame allocation.

The scene caller keeps the original culling, hidden-material and fog rules. A
group executes only when every constituent draw is visible and has compatible
live fog state. Partial visibility uses individual native draws in original
order. Material changes patch precomputed slots; only dirty command ranges are
flushed. The command owner seals returns relative to the actual SDK segment.
Every group records its last use epoch: subsequent uses in that same epoch
cannot patch it, even if the GPU command list has not been submitted yet.
Successful FrameBegin retirement permits reuse; the epoch counter itself is
not a fence. Material release frees packets before their borrowed descriptors,
after the existing scene resource fence.

The first Old 3DS-profile palace run passed (88.28 host seconds, DSP LLE plus
layout instrumentation; **not a performance measurement**). It executed 16,530
packet draws in 1,734 calls, with zero epoch/live/publication rejections. Of these
updates, 1,177 wrote no packet data. The GPU layout observer finished 110,577 draws
without a fault. The inspected palace capture retains the textured floor and
walls, Krystal, purple creatures, particles and HUD. This single static scene
does not establish complete visual parity, hardware cache safety or endurance.

Packet allocations peaked at 81,840 linear bytes and 16,992 descriptor bytes;
both returned to zero at shutdown. Total sampled normal heap was
41,734,480/50,331,648 bytes, minimum free linear 1,673,216/10,485,760 bytes, free
VRAM 4,723,712 bytes, process commitment 69,611,520/83,886,080 bytes, and sampled
main stack 72,984/524,288 bytes. The archive retains all 18 shader programs and
the exact parent game library. Old 80/New 124 MiB profiles and restoration of
unrelated root sources were verified. No console files were changed.

`test_world_packet_game.py` executes the actual load/update/release owner with
simulated selectors, allocator, SDK admission and retirement epoch on ARM11.
176 checks cover same-epoch immutability, last-member hidden/fog rejection before
mutation, failed publication retry, unchanged update, release and budget refusal.
This complements, rather than replaces, actual gameplay and GPU testing. Initial
fixture errors were a mismatched SDK stub return type and unenabled emulated VFP;
no production guard was relaxed to pass the test.

Evidence: `world-packet-game-test/result.json`,
`unified-world-packet-v1-console-setup-builds/{scope,restored}.json`, and
`artifacts/native/unified-world-packet-v1-benchmark-old-dsp/summary.json`.

The same binary's New 3DS-profile run also passed (95.68 host seconds), with
16,462 grouped draws, 1,723 calls and zero live/epoch/publication rejections.
The New capture and the separate Old profile capture were inspected; palace
geometry, materials, characters, particles and HUD remain visible. New sampled
normal heap is 56,611,688/71,303,168 bytes, free linear
16,353,280/25,165,824 bytes, free VRAM 4,723,712 bytes, process commitment
104,751,104/130,023,424 bytes and main stack 72,984/524,288 bytes. Old executes
the shared CIA's CXI; New executes its shared 3dsx. Therefore their executable
container hashes differ while source/shader/game identity is shared.

The actual Old eight-frame ARM window passed separately (107.11 host seconds).
Whole-window encounters changed 24,143,808 -> 23,408,397 (-3.046%) versus
nativefx-v3; versus the last hardware build's matching profile, the observed
delta is -15.344%. **Neither number is an attributable renderer speedup or FPS
prediction.** Roughly 74% of the net change is an unmodified audio-worker stack
watermark scan falling inside the parent window and outside the candidate's.
The comparator disambiguates the several static functions named `work` by ELF
address: audio-worker encounters are 545,931 -> 1,253. Actual disassembly at
0x1078e8..0x107900 confirms the byte scan for 0xa5. Source and game-work cadence
were not altered to avoid that scan. Audio producer visits also differ 25 -> 27
and particle batches 293 -> 288.

World draw count in this window remains 768: 552 individual submissions plus
216 draws in 24 persistent groups, versus 768 individual submissions before.
The ordinary append routine falls from 446,512 to 323,448 instruction encounters;
new packet update uses 53,760, and selection, culling, SDK handoff and command
return costs remain. Actor append is unchanged at 624,312. These results establish
actual integration and limited coverage, not the required large whole-frame win.
The next expansion must address eligibility/visibility granularity and remaining
actor/particle submission costs without claiming the standalone 71% saving for
the whole renderer. Comparison evidence is `world-packet-game-test/profile-comparison.json`.

## Remaining integration gates

- Larger-leaf calls and the initial material/texture GPU comparison now pass
  locally; retain their regressions during integration.
- The initial real-game resource/visibility integration above now exists.
- Measure the complete matched frame window and inspect shared New 3DS behavior;
  extend coverage beyond this palace view before console promotion.
- Physical cache/coherence and performance remain unvalidated.

Reproduce component checks through `tools/linux_exec.sh`:

```text
python3 /work/tools/audit_native_world_groups.py
python3 /work/tools/test_pica_world_packet.py
python3 /work/tools/summarize_world_packet.py
python3 /work/tools/test_pica_command_chain.py
python3 /work/tools/build_world_packet_gpu.py
python3 /work/tools/run_world_packet_gpu.py --label world-packet-gpu-UNIQUE
```

The prior installation package remains `0d6f83a3f5d5`, undeployed. The new
experimental application is `b8350ab83f67`, also undeployed. The last physical
13.52 FPS measurement is unchanged; standalone packet savings are not FPS gains.
