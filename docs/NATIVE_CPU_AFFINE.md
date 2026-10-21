# Native ARM11 affine composition

Local work, not deployed. Whole-shadow flicker and the full renderer/performance
goal remain open. This CPU kernel supports the existing native PICA renderer;
it does not move vertex work from the GPU to the CPU.

An eight-rendered-frame guest block profile of the exact scene-bank-v2 replay
(`scene-bank-v2-cpu-profile-new`) completed normally with state/save/drain checks.
The scheduler sampled counters1600..1607. Main-core instruction encounters were
distributed: particle simulation5.72%, memcpy5.41%, affine composition5.10%,
uniform publication4.70%, command construction3.78%, CPU vertex skinning2.15%.
These are instruction encounters, not cycles or independent frame-time shares.
Profiler timing is explicitly excluded. Audio work was separately observed on
core2; it is not an idle graphics core.

The typed source audit covers all794 compiled game units. DIMMagicBri and
WCTempleBri read/write current CPU vertices; hit spheres instead use matrices.
Raw-pointer escapes, callbacks and temporal morph-buffer ownership are not
proved absent by that audit. CPU skinning has therefore not been removed or
delayed. It is not the dominant main-core cost in this sampled scene.

The affine kernel replaces `C_MTXConcat` with scalar ARM11 VFP instructions.
It loads B once into registers and processes A one row at a time. Both output
aliases and A=B=output are supported. Original multiplication/addition order,
including explicit +0 and translation grouping, is retained. It uses no vector
FPSCR mode, fused operations, new worker, heap, linear memory or GPU resources.
It saves32 stack bytes for callee-saved VFP registers; it does not keep pointers
or shared mutable state across calls. Simulation and animation computations
still execute. No game archive or rendering shader changes are required.

`test_native_affine.py` executes both the actual pinned C implementation and
the candidate using the project's ARM compiler flags in Unicorn ARM11 mode.
2,048 cases /24,576 coordinates are bit-identical, including alias modes,
signed zero, subnormals, finite values, infinities, NaNs, rounding modes and
flush-to-zero. It verifies output bounds, stack restoration, callee-saved GPRs
and VFP registers and unchanged FPSCR control bits. Counts are91 instructions
for the candidate versus212..217 for the source. This is not a physical cycle
measurement or an established net FPS gain.

The same test counts guest data accesses:128 read bytes and80 write bytes per
candidate call, versus340/84 for distinct output and388/132 for aliased output
in the compiled reference. Stack spills are included; instruction fetch and
physical cache/bus traffic are not. Mutants that break output=B aliasing,
callee-saved VFP registers or translation arithmetic are rejected.

Pinned Melee's uniform ownership and early-queue lifetime rules were revisited
while following the profile. This change affects CPU math only; GPU publication,
cache coherence, command submission and resource retirement remain unchanged.

`build_native_affine.py` isolates each scene-bank-v2 app snapshot, redirects
only the pinned source definition and adds `native_affine.S`, verifies exact
linked kernel bytes and unchanged shaders/game archive/profiles, and restores
the worktree in `finally`. Never blind-build the unrelated root WIP. The normal
and replay variants must be paired before deployment. Runtime/build results
will be recorded after completion. No physical performance claim is made.

## Local game checks

Replay source`348537b85014`,3dsx`3da14402b040`, completed the same New Thorntail
and Old palace routes in83.88/53.16 host seconds. Both pass state/save/log-drain/
owned-exit checks. New1740 and Old1410 were inspected: world textures, characters,
HUD and effects remain visible. This is limited regression evidence, not a
full playthrough or shadow-flicker acceptance.

The New route's raw object-render phase was2.6666ms versus2.8877ms in the prior
scene-bank-v2 run. Scene preparation was2.0701 versus2.0963ms and native
submission3.6743 versus3.6687ms. Whole frame18.039 versus18.085ms, with different
animation/particle work. **Do not accept this as an FPS win:** both candidate
runs were contaminated by foreign compilation. A short local kernel test also
overlapped the Old route's tail. No unchanged timing reruns were requested.

New sampled heap49,230,480/71,303,168bytes, linear free16,631,168/25,165,824,
VRAM free4,690,944, measured main stack73,044/524,288, process103,419,904/
130,023,424. Old heap41,079,824/50,331,648, linear free3,234,176/10,485,760,
VRAM free4,723,712, stack72,984/524,288, process67,768,320/83,886,080.
Sparse measurements are not a claim of unchanged peak usage in every scene.
Private assets/saves and both console installations remain untouched.

Normal setup source`abce232a3f0d` (3dsx`eac3affa3748`, CIA`6aca73773f65`)
and replay`348537b85014` (3dsx`3da14402b040`, CIA`f5287333ec9e`) are archived
under`native-affine-v1-*-builds`, not deployed. Their619 source records,17
embedded GPU programs and game archive`c996af790c5a` match. Both linked affine
kernels match the tested machine code exactly; profiles stay Old80/New124.
Original setup recipe and worktree restoration pass. There was no fresh normal
boot/conversion: unchanged setup evidence is reused, with separate replay
gameplay checks above. Root support builder also has the affine substitution,
while unrelated root renderer WIP remains isolated. Do not double-apply this
patch to the latest snapshot or blind-build the root.

All owned build/test/emulator processes ended. Full rendering, physical shadow
stability, sustained console performance and final progression/save/endurance
acceptance remain open. The next larger renderer opportunities are uniform
publication/command construction and memory traffic; this kernel alone cannot
close the entire frame-budget gap.

## Uniform bookkeeping follow-up, 14 October

The next candidate changes `tools/retained_uniforms.py`, not the GPU renderer
or game's state producers. Consecutive dirty flags are scanned four bytes at
a time; installed-value validity, dirty retirement and ever-dirty restoration
flags are maintained together with aligned word stores. The public SDK boolean
array ABI remains unchanged, including direct writes. Every retired row is
known installed: trimmed prefix/suffix rows were valid and equal, and middle
rows were copied into the command list. Float24 constants and lost contexts
still invalidate retained values. Pinned Melee `uniform_upload.h` remains the
installed-value comparison reference. No command, fence, target, shader, asset,
simulation, allocation or worker was added or removed.

`test_retained_uniforms.py` passes10,020 ASan/UBSan state comparisons against
the pinned SDK, and another10,020 against the archived previous uploader.
The latter also compares the hash of the entire published register stream
after every update:204,627 rows/6,357 batches are identical. Both missing
constant-invalidation and missing-context-invalidation mutants fail in each
suite. Sources, generated source hashes and results are under
`artifacts/research/pica-scene-rewrite/uniform-retire-test/{sdk,prior}`.

`test_uniform_retire_arm.py` executes both generated uploaders on ARM11 across
246 synthetic dirty-run cases:353,830→330,800 instruction encounters in total
(6.51% fewer).212 cases improve,34 regress; the largest regression is20
instructions for a short unchanged run. This distribution is synthetic, not
captured gameplay. Counts include identical command-hash stubs and memcpy;
they are not CPU cycles or a console FPS prediction. This is a bounded local
contribution, not closure of the frame-budget gap.

Shared local builds are `uniform-retire-v1-*-builds`: replayf0c98aeb6c24
(3dsx9186989fc75c/CIA9a2ad01a9df8), normalbc8802c1cd9b
(3dsx0cedbb3de476/CIAc0da92eb36b7).619source records,17embedded GPU programs,
archivec996af790c5a, Old80/New124 profiles and restoration pass. The only source
delta from native-affine-v1 is the uniform generator; all GPU programs are
unchanged. `build_uniform_retire.py` isolates the archived native-affine sources
and verifies the built SDK source against the tested hash. Root remains partly
unrelated renderer WIP: do not blind-build it. No deployment or fresh normal
startup/conversion is claimed; unchanged setup evidence is reused separately.

Serial New ThornTail and Old palace checks pass state assertions, save
preservation, shutdown/drain and owned cleanup in69.74/53.51host seconds.
New1740 and Old1410 captures were inspected for world, actors, textures, HUD
and active effects. Both timing comparisons are excluded because foreign SMG
compilation was observed; no unchanged boots were repeated to seek clean
numbers. Raw New native submission3.6743→3.4967ms and wall18.0395→17.7578ms
cannot be attributed to this change. Animation/workload also differs.
`uniform-retire-test/runtime-comparison.json` preserves that limitation.

Sampled New heap49,230,480/71,303,168, linear free16,635,264/25,165,824,
VRAM free4,690,944, stack73,044/524,288, process103,419,904/130,023,424bytes.
Old heap41,079,800/50,331,648, linear free3,234,176/10,485,760,
VRAM free4,723,712, stack72,984/524,288, process67,768,320/83,886,080.
These are sparse scene measurements, not worst-case whole-game budgets.
Physical shadow flicker remains unresolved; this CPU change is not its fix.


## Fused native uniform publication, 14 October

The upload path previously copied every changed float32 row into a PICA command
and then copied it again into the retained-value cache. The new generated
`fh_uniform_packet` reads each word once and writes it to both destinations.
It keeps the SDK CONFIG command, non-incrementing DATA register, 256-word chunk
limit, command padding, row order and all original invalidation rules. Capacity
is checked for the complete packet before command or cache writes. This does
not add a GPU fence, alter command ownership, add memory or reduce game work.
The pinned Melee uniform-upload and checked-command patterns informed the
adaptation; the actual linked libctru GPUCMD_Add is the command oracle.

`test_uniform_packet_arm.py` runs both complete generated uploaders on ARM11
against the actual SDK command constructor. All 652 cases match raw command
bytes, application rows, dirty flags and reuse counters, including both shader
stages, signed-zero/NaN bit patterns, float24 constants, context loss, disabled
reuse, and the 64/65-row split. Nine invalid-capacity/null/cursor cases stop
before packet/cache mutation; exact-fit cases include 1/3/63/64/65/96 rows.
Synthetic instruction encounters total903,927 versus738,388 (18.31% fewer).
551 cases improve;101 regress by one instruction at most. These are not captured
gameworkload frequencies, CPU cycles or hardware FPS estimates.

The host retained-state suite also passes10,020 cases under ASan/UBSan and
rejects lost-constant/context-invalidation mutations. Its new decoder validates
and merges actual packet chunks into the older logical-upload oracle. It does
not substitute that logical comparison for the independent ARM raw-byte test.
The initial ARM harness used an unprefixed private cache symbol; the namespace
was corrected and the full check rerun successfully.

Shared local builds under `uniform-packet-v1-*-builds` are normalbf6c7238e439
(3dsxaae48bcd1e29/CIA83ab23ca0f9d) and replayfad3e79c9119
(3dsx66893e4cc6cf/CIAc1f960df6b67). They are **not deployed**. All619 source
identities match across variants; only the generator differs from the preceding
particle-static pair. All17 GPU programs and each parent's game archive are
unchanged. Old80/New124, setup recipes and worktree/archive restoration pass.
The inherited archives have identical payloads but differing ar timestamps;
this is recorded rather than claiming equal archive hashes.

The New Thorntail route finishes in70.45host seconds with normal replay state,
unchanged saves, drained logs and owned emulator exit. Capture1770 was inspected:
world, Fox, HUD and effects remain visible. The run uploaded2,423,391 uniform
rows and retained1,426,330. No compiler process was observed at timing boundaries,
but the prior baseline was contaminated and pose/workload counts differ.
**No net FPS speedup is established.** No extra Old replay or normal setup boot/
conversion was run for this SDK-only change; prior unchanged-path evidence is
kept separate from this exact-binary New run.

Sampled New heap49,230,472/71,303,168bytes, linear free16,629,120/25,165,824,
VRAM free4,690,944, measured stack73,044/524,288, process103,419,904/
130,023,424. The implementation adds no allocation. Detailed evidence is in
`uniform-packet-test/{result,host/host,build-verification,runtime}.json`.
Physical performance, whole-shadow stability, full visual parity and final
progression/save/endurance acceptance remain open.


15 October native camera ownership follow-up (local normal 573e360bde6b)

The latest deployed c12ba39fda77 still republishes seven camera rows at every
actor boundary. The native scene owner now compares against the SDK's existing
current rows and leaves their existing dirty/publication state intact when equal.
It retains no input pointers or second matrix copy. Full pipeline binds, external
scene invalidation and scaled world matrices retire the owner. The verified
ordinary/ambient shared ABI retains it. Compact shadows use the same helper.
This follows pinned melee-3ds native-3ds/port/3ds/uniform_upload.h installed-value
and ownership discipline, adapted to native little-endian SDK rows (w,z,y,x).
No GPU fence, flush, shader or game simulation change.

A duplicate matrix-cache prototype was rejected: 1,340,184 vs 1,241,007 synthetic
ARM instruction encounters. SDK-state v2: 1,173,768 vs 1,241,007 (-5.42%) across
1,200 cases, with identical command bytes and SDK values/dirty flags. These
cases include same-pointer mutations, changed projections, sign/NaN bit patterns,
world/foreign writes, dirty restoration and clearing. It is not a game frequency
model or FPS estimate; individual changing-camera cases cost more. Actual native
bind/invalidation/world snippets pass 10,000 ASan/UBSan lifecycle checks; three
missing-invalidation mutants fail. Other SDK calls in owner test are stubbed;
complete GPU correctness is not inferred from that test alone.

Isolated normal build scene-camera-retention-v2-console-setup-builds changes only
two scene sources; parent game archive, all shader binaries, install recipe and
Old80/New124 profiles remain identical. Root WIP and archive restored. ELF adds
240 text bytes and 32 BSS bytes (includes alignment), no heap/linear/VRAM.
Actual Old80 normal setup/movie/cockpit/Select check passes in 42.20 host seconds,
owned exit, preserved fixture/source/save sentinel, drained logs. Inspected game
capture shows Fox/cockpit/textures/menu and bottom-screen controls. 9,566 camera
requests reused 5,534 times (57.85%), avoiding 38,738 row writes; scene/actor draw
counts are retained. Particle phases differ from parent: do not claim whole-game
command-byte identity or exact image parity. Timing comparison explicitly false.
Memory: heap39,051,680/50,331,648; linear minimum free5,930,368/12,582,912;
initialized VRAM free4,723,712; stack72,936/524,288; process70,995,968/83,886,080.
These are sampled startup/cockpit values, not worst-case gameplay acceptance.

New exact-last-window instrumentation also worked: presentation split showed
callback11.716ms, publish1.754ms, copy.042ms, prep.021ms, other present.014ms in
this emulator window. That is not an explanation of the earlier physical18ms
OTHER value. It confirms the new logs can retain a matching component window.
No new physical performance claim or hardware request; Old SD remains c12,
New SD22e57. Local candidate not deployed. See the build acceptance.json,
scene-camera-retention-test/ and scene-camera-owner-test/ for reproducible proof.
