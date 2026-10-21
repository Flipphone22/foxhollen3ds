# Current status: user confirms fixed on hardware, 14 October

The user explicitly reports that the rapid whole-shadow flicker is fixed.
Accept that physical observation for this symptom. Earlier unresolved notes
below describe the investigation history, not the current acceptance status.
The known installed New build at confirmation was f2c4fafba819; that confirmation did not
include fresh logs or another build ID. Do not infer additional hardware
measurements or complete shadow fidelity from it. No further flicker probe is
needed. Normal source06685e13fd29 was subsequently deployed on 14 October at
the user's request; see IMPLEMENTATION.md for its separate acceptance scope.

# Entity-shadow flicker, 13 October

The user reports frequent entity-shadow flicker in the deployed New 3DS build.
Read-only collection `artifacts/console-visual/20261013-142848-223` is complete.
Its manifest identifies normal setup build `912b6c79f861` (3dsx `6027e2e19fc2`).
The final image shows Fox and Tricky in ThornTail Hollow; projected silhouettes
were enabled. The run exits at retrace 5872, with 59,868 GPU jobs submitted and
completed, zero pending/failed. There are 5,840 silhouette captures/casters,
zero rejected casters or empty receiver sets. These counters cannot establish
temporal image stability. Settings, saves and assets were not changed.

The sampled frame windows average 30.42 FPS across mixed scenes including
loading, with 15 intervals above 100 ms. Submission averages 8.28 ms and the
existing GPU metric 18.78 ms; these overlapping spans must not be added into a
frame breakdown. This is not a matched performance comparison.

## Concrete source discrepancy

Original `main/lightmap_draw.c` applies `Camera_ApplyDecalViewport` before
projected and ground shadows. Original `main/camera.c` requests near=-0.05,
far=1. Native ground decals already implement this as reversed-Z scale=-1.05,
offset=0. The deployed projected silhouette receiver instead used scale=-1
and a constant 1/4194304 offset (roughly four D24 quanta). That discards the
original depth-relative lift between collision receivers and visible terrain.
It is a plausible source of flicker, not yet a proven explanation of every
physical symptom.

The candidate restores scale=-1.05, offset=0 for projected shadow receivers,
retaining GEQUAL and colour-only writes. Actual finite collision triangles,
query bounds, shaders, texture publication and lifetimes remain unchanged.
It adds no passes, geometry, allocations or per-vertex CPU work. This is the
original decal tolerance: nearby foreground within that tolerance follows
original behaviour; this is not mathematically exact surface equality.

Pinned Melee `native-3ds/port/3ds/early_queue.c` was consulted for queue and
resource ownership. This repair does not relax retirement, skip waits or reuse
GPU-owned resources. Deployed sources were read from the immutable snapshot,
not the older unrelated VRAM worktree.

## Validation

`tools/test_shadow_project_depth.py` embeds the actual original camera function
and exercises both jitter/non-jitter viewport branches. 393,216 source mapping,
receiver-tolerance and foreground-occlusion cases pass under ASan/UBSan. The
former fixed epsilon, equality comparison and enabled-depth-write mutants are
rejected. These numerical cases do not emulate physical raster precision.
Evidence: `artifacts/research/pica-scene-rewrite/shadow-decal-depth-test`.

Build/runtime/deployment status will be appended after completion. Physical
flicker acceptance remains open; no new probe is requested.

Candidate normal setup build `f2c4fafba819` completed: 3dsx `57d6c0e23c62`,
CIA `456fd01ab0e3`. `shadow-decal-v1-console-setup-builds/scope.json` verifies
the sole source change is `fh_pica_shadow_depth.h`, all embedded shaders are
identical, the game archive remains `64298a7423a1`, installation recipes are
unchanged, and CIA profiles remain Old80/New124. Worktree/archive restoration
is verified. The paired blur builder now carries this header into future
normal/replay builds; the already-built blur pair has not been rebuilt.
Package: `build/console-validation/foxhollow-iso-setup-76da3410980e0b98.zip`.
The package does not claim a fresh conversion on this particular executable;
existing full conversion evidence and identical recipes remain separate.

The exact candidate normal app passes the isolated, silent save-load test
`artifacts/native/shadow-decal-after-save-new-v1` in 157.15 host seconds.
Both loaded captures (1600/1750) were inspected: Fox, the cave, HUD and the
projected shadow remain visible. This is limited static-view coverage, not
proof of hardware temporal stability. The earlier fetch-layout regression
audited at least167,936 draws with zero faults. Select exits normally, logs
drain, owned emulator exits, source assets and copied-save sentinel remain
unchanged. Timing comparison is explicitly invalid: diagnostic emulator and
unrelated host compilation were active. No console FPS claim.
Correction: the memory panel shows USED6795/24576KiB linear and1531/6144KiB
VRAM, not free space. These agree with final telemetry free17780/4613KiB.
The previous claim of missed lower-free values was a misreading. Sampling is
still not continuous allocation tracking. No allocations were added.

New223 deployment COMPLETE: `artifacts/console-test/20261013-145001-new223/transfer.json`.
All six app/metadata files were staged, fully byte-readback verified, then
renamed and checked for final size. Saves, settings, ISO and prepared assets
were untouched. Matching .3dsx and CIA are on SD; HOME CIA requires FBI install.
No Old229 or replay deployment. Launch normal `SD:/3ds/foxhollow/foxhollow.3dsx`,
build `f2c4fafba819`, with SHADOW SHAPE enabled for physical validation.
No reboot or asset conversion is required. Full visual parity and performance
remain incomplete; physical flicker resolution is not yet confirmed.

## Whole-shadow clarification and command-order audit

The user clarified that entire shadows disappear/reappear rapidly. This is not
confirmation of a new run on f2c4fafba819. Physical acceptance remains open.

The actual SDK already flushes/invalidate framebuffer writes at FrameSplit and
clears the texture cache after binding. Retained sampler optimization preserves
these side effects. The deployed image owner cycles three images, with ordered
capture and sampling; no CPU fence was added speculatively.

`tools/test_shadow_sdk_handoff.py` runs the deployed image begin/end/rebind and
actual ARM SDK context/framebuffer/texture publishers, including the existing
NULL binding adaptation. All 1,024 capture/project/reuse cycles pass (66,558
checks). Dropping framebuffer flush or texture cache clear fails as expected.
The test also covers repeated same-address texture bindings. Results are in
`artifacts/research/pica-scene-rewrite/shadow-sdk-handoff-test`.
This tests command publication order only: it does not execute the shader,
model asynchronous PICA raster/cache behavior or establish temporal stability.
No second renderer defect has been established by this audit; no additional
build/deployment or performance claim follows from it.

## Continuous observations, 13 October

Added a bounded hidden-X11 capture helper (`tools/capture_x11_clip.py`) and
`--temporal` mode to `tools/run_shadow_decal_handoff.py`. It captures 480 screen
observations over eight seconds without guest writes, audio, or desktop input.
Observation timestamps are asynchronous: duplicate images and skipped game
frames are possible. These diagnostic runs are excluded from timing comparison.
The first attempt failed because ffmpeg was unavailable in the isolated runtime;
the helper uses libX11 directly instead. That failure is retained separately.

Both exact normal builds completed the same cave save-load/idle/Select route:
`shadow-decal-before-save-new-temporal-v2` (912b6, 82.21 host seconds) and
`shadow-decal-after-save-new-temporal-v2` (f2c4, 95.46 seconds). Both drained their
logs, exited their owned emulator and preserved the source assets/save. Each
audited at least 249,856 vertex-fetch draws without faults. Contact sheets and
lowest-contrast frames were inspected: world, Fox, HUD and shadow remain visible.
Neither run reproduced rapid whole-shadow disappearance. The older build also
passing this scene means this is **not** acceptance of the physical repair.

`shadow-temporal-capture-test/result.json` records 230/228 unique observed images.
After the first two seconds, the shadow-region contrast ranges were 26.09..57.58
and 30.34..41.62 RGB levels. This region includes terrain and changing fog; these
numbers are not an isolated shadow-pixel classifier or proof of improvement.
Sparse memory samples match: heap54,285,824/71,303,168, linear free18,207,360/
25,165,824, VRAM free4,723,712, measured main stack72,984/524,288; process
103,993,344/130,023,424 bytes. No new renderer allocation/change was made.

The command-capacity reset path was also inspected in the deployed snapshot.
`acquire` invalidates scene/project state; capture binding restores the shadow
target after reserve, while projection republishes its program/parameters after
reserve. No missing restoration was established; no speculative wait was added.

Read-only collection `artifacts/console-visual/20261013-155001-223` found gameplay
logs/captures unchanged from 142848; only the installed build manifest is newer.
The existing f2c4 app remains installed. No additional remote writes or hardware
probe request were made. Physical whole-shadow stability remains open.

Read-only collection `artifacts/console-visual/20261013-163615-223` again found all collected hashes unchanged from155001. No remote writes, new shadow defect, physical acceptance or extra probe request resulted. Native heat work is separate; see `NATIVE_HEAT.md`.

Read-only collection `artifacts/console-visual/20261013-185610-223` completed;
all file records match175106 exactly. The retained gameplay log reports5840
captures, zero rejected/empty projections and zero declined receiver builds,
but aggregate submissions cannot establish pixel visibility or explain a
hardware-only flicker. There is still no new run of the deployed depth
correction. No files on the console were changed and no probe was requested.
Read-only collection `artifacts/console-visual/20261013-234210-223` timed out
before login; no console files were changed. The initial root-source review
looked at the older triangle receiver shader and its65,536-case VM regression.
That is **not** the current archived executable's receiver shader. The actual
snapshot selects the133-instruction point/geometry program with the repaired
32-byte fetch extent. Do not use the older shader test as new current-binary
evidence. Inspection of the actual source snapshot confirms live receiver writes
follow the parent upload join, while range trees deliberately retain authored
query envelopes and are independent of moved positions. No new ownership defect
or physical flicker resolution was established by this review.

Read-only collection `artifacts/console-visual/20261014-001304-223` timed out
before FTP login. No console file was changed and no new physical shadow result
is available. Ambient shader work is independent of the unconfirmed shadow fix.

Read-only collection `artifacts/console-visual/20261014-010618-223` also timed
out before login. No remote files changed. The user's whole-shadow high-frequency
description remains the symptom to validate; newer scene-bank optimizations
are independent work and are not evidence that this failure is repaired.

## Moving cave coverage, 14 October

Read-only collection `artifacts/console-visual/20261014-013905-223` timed out
before login. No console files changed. There is still no collected new run
that validates the installed depth correction against the reported flicker.

`run_shadow_decal_handoff.py --temporal --motion` now extends the same isolated
normal-app save-load route with four 60-retrace cardinal holds, circle-pad
axis70, then neutral and Select. Camera movement follows ordinary game input;
no game-state writes, altered clocks or source-save changes. The first harness
attempt rejected an ambiguous text-replacement boundary before starting a game;
the boundary was narrowed before running the route.

`artifacts/native/shadow-decal-after-save-new-motion-v1` runs the exact installed
3dsx57d6c0e23c62/sourcef2c4fafba819. It passes in97.21host seconds, with normal
shutdown/log drain, preserved source assets/save sentinel and owned-process
cleanup. The vertex-layout audit observes258,048 draws without a fault.
The eight-second clip contains480 asynchronous observations/154 unique images.
All154 foot/ground crops were inspected across `shadow-unique-{0,1,2}.png`;
the shadow remains visible while Fox moves and takes mushroom damage. This is
not a reproduction of the hardware disappearance. Duplicate/skipped guest
frames remain possible, and the route is a cave scene, not outdoor coverage.
The static terrain ROI metric is deliberately disabled for this moving route.

`analyze_shadow_temporal.py --run <run>` reproduces the contact sheets and
records observation spacing and uniqueness; manual inspection is separate
from assertions about pixel visibility. Timings are excluded. Sampled heap
54,286,112/71,303,168bytes, linear free18,182,784/25,165,824, VRAM free4,723,712,
stack72,984/524,288, process103,993,344/130,023,424. No renderer or asset change
was made for this test. Physical whole-shadow stability remains unresolved;
the separate uniform-bookkeeping change is not a shadow fix.

Read-only collection `artifacts/console-visual/20261014-021410-223` timed out
before login. No console files changed. Reinspection of the deployed snapshot
confirms capture publication, receiver-state rebinding after command splits,
and explicit receiver shader admission. This source inspection does not
establish GPU execution correctness or resolve whole-shadow flicker. No new
repair, deployment, or hardware probe request resulted.


## Follow-up: whole-shadow dropout

The clarification is whole silhouettes disappearing/reappearing at high
frequency, not speckled pixels. This does not identify the running build or
confirm a post-install run on f2c4fafba819. Keep physical acceptance open.
Read-only collection `artifacts/console-visual/20261014-024922-223` timed out
before login; no new console evidence or remote writes resulted.

Reinspection of the exact installed source snapshot covered caster admission,
morph/compact capture selection, receiver admission, point/geometry shader
outputs and three-slot image ownership. No additional defect was established.
The existing per-run counts cannot distinguish intermittent blank captures or
frame-specific receiver rejection. The moving cave emulator observations do
not reproduce the hardware dropout. Do not report this as fixed, add a
speculative GPU wait, or treat unrelated shader/CPU optimizations as a repair.


Read-only collection `artifacts/console-visual/20261014-033413-223` timed out
before login. No console files changed. The fused uniform writer retains exact
SDK command bytes in offline checks; it is not a claimed shadow-flicker repair.
The installed f2c4 build and its physical acceptance status remain unchanged.

Read-only collection `artifacts/console-visual/20261014-034547-223` timed out
before login. No remote files changed. The user explicitly confirms whole
shadows flicker rapidly, rather than speckles within a silhouette. Inspection
of the latest archived image owner, capture clear, receiver draw and point/GS
program found no new demonstrated fault; this is not a repair or additional
physical validation. The installed build remains unchanged.
# Clean-exit mask evidence, 14 October

The user confirms rapid whole-shadow disappearance, not speckled coverage.
This remains **unresolved on hardware**. The new normal-app diagnostic captures
the last three generated RGB565 shadow masks on Select exit, before scene/UI
teardown. It does not capture every gameplay frame or authored texture masks.
Model ID, render-frame number, generation, alpha, capture/receiver draw counts
identify each slot; slots can belong to different objects and older frames.

The implementation follows pinned Melee `blend_verify.h` colour readback and
citro3d's queue semantics: the in-frame display-transfer call only queues work.
The parent renderer explicitly joins before cache invalidation, CPU reading or
scratch release. Input is RGB565 colour, output BGR8; no depth-readback path is
introduced. Scratch is 49,152 linear bytes only at clean exit; persistent slot
metadata is 96 bytes. Gameplay adds bounded metadata stores, no new draws,
GPU waits, readbacks, workers or per-frame disk writes. Automatic replay exit
does not request this diagnostic.

`tools/test_shadow_capture.py` passes eight real-code lifecycle/failure cases
under ASan/UBSan, twelve malformed/corrupted files, stale-log rejection, and a
missing-join mutant. These mocks do not establish GPU correctness. The collector
now fetches one bounded 147,600-byte file and validates its exit-log timestamp,
generation order and pixel CRCs before decoding images.

Normal build source `ffd8a2bd12d1`, 3dsx `0a23850d0a03`, is local, NOT DEPLOYED.
`artifacts/native/shadow-capture-save-new-v1` passes real converted-assets
startup, cave save load, Select exit, log drain, save/asset preservation and owned
process cleanup in 87.71 host seconds. The layout audit sees 163,840 draws with
zero faults. Three mask images inspected: generations 309..311, render frames
1667..1669, 671/672/672 nonblack pixels, one receiver draw each. The loaded-later
image contains Fox, cave geometry, HUD and the ground shadow. This validates the
emulator evidence path; it does NOT reproduce/fix physical flicker or establish
temporal stability from three images. Timing comparison explicitly disabled.
The emulator emitted an X11/SIGSEGV message during owned host teardown after
the guest's successful log drain; this is not classified as a guest crash.

Runtime sampled heap 54,287,256/71,303,168; linear free minimum
18,085,376/25,165,824; VRAM free 4,723,712; stack 72,984/524,288; process
104,030,208/130,023,424 bytes. These are New-emulator samples, not Old hardware
measurements or guaranteed capture-time minima. The shutdown scratch is
separately bounded above.

Stable source changes live in the isolated `shadow-capture-v1` snapshots; the
pure `shadow_capture_patch.py` reconstructs them from `archive-identity-v4`.
An attempted all-or-nothing root integration rejected a differing WIP anchor
before writing any file. Do not blindly build or overwrite the unrelated root
renderer. No hardware probe request, deployment, or console save/settings edit.
