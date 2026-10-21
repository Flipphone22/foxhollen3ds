# 12 October: native startup texture encoder

`install_texture.cpp` reuses the reviewed Melee CMPR palette semantics, including
5:3 endpoint interpolation and the RGB carried by transparent samples. The new
startup converter produces existing PICA ETC1/ETC1A4 tiled data. It has no GPU
ownership, gameplay draw or per-frame conversion integration.

The separately ZLIB-licensed `rg_etc1` codec is pinned from devkitPro tex3ds v2.3.0
commit37220c696e873812df41af62d1db0b20c66661fe. Its license and original attribution
remain in the header. Three float-to-unsigned casts before a clamp were undefined
for negative refinement estimates; the adaptation uses signed int. Nine explicit
uint32_t `std::min` arguments make the code build with newlib's unsigned-long
typedef. Both changes are marked in the source. See vendor provenance JSON.

The adapter implements native bounded CMPR sampling, optional opaque-only
separable BOX reduction, and the PICA block byte/alpha ordering. The existing
tex3ds `load_image` composites onto a transparent canvas; fully transparent RGB
becomes zero before ETC encoding. Matching that step repaired the first alpha
texture byte mismatch. Original alpha coverage remains exact. No Magick/PNG or
desktop tex3ds frontend is linked into the native module.

Actual34existing private textures match all native output bytes and independently
decoded alpha/error metrics under ASan/UBSan. ARM compilation passes. No ARM
execution, complete pack coverage, on-device conversion time or renderer/FPS
acceptance is claimed. Full generation/ISO/menu integration remains unfinished.

# 6 October: object color fades and native precipitation

Pinned Foxhollow `shader_dolphin.c:addColorFadeStage` supplies RGB interpolation
with previous alpha preserved. Native model stage5 now implements that equation
using the pinned Melee renderer's independent RGB/alpha combiner pattern. Six
available PICA stages are respected; current layered model materials use at most
stages0..3 before the fade. No additional shader instructions or texture units.
Prepared movie and alternate implicit-shader behavior remain separate.

`dlls/engine/7/7.c:snowPrintSnowCloud` emits original rain/snow triangles into the
same owned native effect stream as expgfx quads. Retains every original cloud/
flake update, wind/drift calculation, random behavior, matrix, texture change,
signed UV/128 convention (GX_VTXFMT4), live C1 color and alpha blend/depth-test
state. Only FIFO emission changes. Shared61,440-byte linear stream; GPU applies
the original local-to-view matrix and camera projection. Texture and matrix
changes flush in original order; triangles and quads share bounded rollover.
Melee's synchronized storage/binding lifetime rules continue to apply. No new
worker, larger heap, secondary buffer or simulation/progression shortcut.
Lightning strands and other unsupported weather/effect families remain missing.

# Melee adaptations

2026-10-05: synchronous ordered native submission program consults pinned
Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/renderer.c raster
invalidation, indexed submission and stable texture bindings; gx_thread.c and
render_worker.c were reviewed for ownership boundaries. Existing movie fixture
and its deliberate overlap-order failure remain the correctness oracle.
platform_ordered_program.h lowers two bindings through the ordinary renderer,
pins their texture owners, packs geometry into the existing fenced stream,
and submits base/overlay pairs without triangle reordering. No borrowed source
or stack texture descriptor survives the synchronous call. This differs from
Melee's material-specific shader paths because SFA's fourth sampler requires
an ordered second pass. Citro3D 9f21cf7b380ce6f9e01a0420f19f0763e5443ca7
internal.h, base.c, effect.c and texenv.c define the POD effect/scissor snapshot
and SDK dirty flags; texture and combiner bindings still use SDK setters.
No new GPU transfer, thread, audio behavior, pool allocation or shader is used.
The two bounded stack recipes and vertex copies are retired by existing stream
fences. Mode3/--ordered-program is experimental and disabled by default.

2026-10-05: native vertex-program lowering consults pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/engine/gx.c's GXBegin dependency
scan, prepared fields and material-program/live-binding separation, plus
tools/test_geometry_sources.py's live-source and mutation oracle. The new
gx_vertex_program.h executes current indexed attributes directly into the
existing fenced PICA stream; it does not retain pointers across draws or change
buffer ownership, shaders, cache flushing, OS services or pool sizes. SFA's
little-endian array rules and CPU lighting/emboss semantics remain in effect.
Backward texture-coordinate dependencies are lowered before execution; only
material-consumed outputs are packed. The existing checked native-decode mode
compares against the original scalar decoder and renderer before submission.
This first CPU-lit slice is not a broad model backend or a whole-frame speedup
claim. GPU-lit/fur packets and general model submission still use the existing
paths; eliminating this slice alone cannot meet the performance target.

2026-10-05: optional movie pass grouping consults pinned
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/engine/gx.c's owned immediate
batches and port/engine/geometry_bounds.h's propagated PICA rounding margins.
SFA's fourth texture still requires two passes. New fh_movie_disjoint.h admits
whole-pass grouping only when triangle pairs have a conservative separating
edge in the actual PICA projection, with exactly shared GPU endpoints allowed
to retain ordinary edge ownership. Unknown/mixed endpoints, clipping through
w=0, overlap and uncertain separation retain ordered per-triangle passes.
No primitive, material term or simulation work is dropped. This is SFA-specific
code, not an imported Melee batching implementation. The bounded projection
scratch is 6144B plus local variables on the existing renderer stack; no new
heap, linear memory, VRAM or shader storage. Host and emulator checks do not
establish physical raster precision; default stays off pending useful evidence.

2026-10-05: the optional shared audio/GX executor adapts core-2 affinity,
exclusive translator ownership, source retirement and the late APT drain hook
from pinned c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/gx_thread.c, with port/engine/gx_fifo.c typed copied arguments and
port/3ds/render_worker.c source lifetime as references. SFA retains its little
endian runtime, 256 KiB command ring and audited fur-loop leases. It does not
import Melee's BE8 bridge or multi-megabyte snapshots. Audio and bounded GX
slices share the existing thread instead of competing workers on core 2;
synth try-lock and FP-mode save/restore keep renderer and mixer contracts
separate. Game/movie/DVD callbacks remain on the producer. Attached rendering
is drained before audio shutdown and before Citro3D's suspend hook. Explicit
--shared-worker-console produces a physical validation candidate with a
same-binary shared_worker.txt=0 fallback. Emulator lifecycle and menu checks
do not establish physical speed, cache behavior or suspend correctness.

The optional producer TEV mirror follows gx_fifo.c's typed-state principle,
with exact integer overwrites and explicit alias invalidation. It remains
off in the physical candidate: its measured emulator gain was negligible.

2026-10-04: a rejected experiment adapted the exact-key plane-cache approach in
pinned Melee port/engine/geometry_bounds.h (MPGeometryPlaneCache), reviewed
with tests/geometry_bounds_tests.c. SFA keeps its existing .008 propagated
margin and separate GX/PICA tests, six candidate planes, mixed-matrix handling,
and invalid-value fallback.32entries/11904B retain only model/projection-derived
coefficients; full source bytes plus ARM rounding/FZ/DN state guard reuse.
Plane preparation is lazy, preserving the first-rejecting-plane shortcut.
No mesh values or prior visibility decisions were cached. Matched emulator
CPU time regressed by 5.84 ms; removed from production. Source/evidence remain
in artifacts/experiments/plane-cache and artifacts/native/title-plane-cache-*.

2026-10-04: platform_graphics.c batches disjoint DMA capture rows behind a
producer fence and a final consumer fence. Reviewed pinned Melee renderer.c
gpu_efb_copy, tools/async_renderqueue.py and tests/async_queue.c; retain its
four-entry queue reserve, callback joining, and GPU-owned capture lifetime.
Departure: hardware rejected Melee-style TextureCopy here, so retained DMA
submissions use bounded capacity waits before the existing FrameSplit reserve.

The subsequent exact visibility memo experiment was also rejected and archived
under artifacts/experiments/bounds-memo. Only1179of10431calls reused; fur-layer
scale changes alter live matrices. Matched CPU108.027ms versus107.349ms.

fh_bounds_hint.h instead adapts Melee's first-rejecting-plane approach to mixed
SFA bone matrices: one1024-byte table field remembers useful plane bits, and
every live box/model is retested before a rejection. All candidate projection
rows are validated before narrowing to a hint. Failure runs the original
common-plane intersection and updates the hint; no previous decision is reused.
Checked mode compares the result with that original calculation on every call.

`source/platform_cpu.c` adapts `native-3ds/port/3ds/cpu_speed.c` from
2gifts/melee-3ds commit `c2ebe7c769b3287f2b3748173e8fe5ec24ef408e`, under
GPL-3.0. The license is in `melee-GPL-3.0.txt`; the upstream scope statement is
in `melee-LICENSE-SCOPE.md`. Function/logging names change; the bounded CPU
configuration/readback/retry/restore policy is preserved.

`tools/test_cpu_speed.py` generates an adapted copy of the pinned project's
`tests/cpu_speed_tests.c` into private test artifacts, using its `tests/cpu_sdk`
mock header, to exercise the actual native adaptation. The same provenance and
license apply to that generated test source.

`source/efb.c` adapts the pinned `renderer.c::efb_copy_cpu_reference`;
`include/fh_efb_rgb565.h` adapts its `port/3ds/efb_rgb565.h` row/pixel reuse.
Both remain GPL-3.0. The native content height is 264 instead of 240, and the
caller retains strict rectangle bounds, odd-origin filtering and byte-safe
unaligned output. GPU synchronization/cache behavior remains in the existing
renderer. Additional helper provenance is in `vendor/melee/PROVENANCE.md`.

`source/fragment.c::fh_texture_pack_fur` adapts the same revision's
`texture_decode.h::mp_cmpr_to_native` palette reuse per 4x4 block, under
GPL-3.0. Foxhollow retains independent per-texel mask sampling and edge padding;
the packed texels remain byte-identical to the original per-pixel decoder.

`include/fh_pica_texture.h` adapts the ETC1 bit layout/modifier decoding from
`reference/azahar/src/video_core/texture/etc1.cpp`, Copyright 2017 Citra Emulator
Project, GPL-2.0-or-later (`citra-GPL-2.0.txt`). The native C version uses byte
loads, avoids bitfield ABI assumptions, and includes PICA 8x8 subtile addressing.
It provides scalar fallback for privately prepared movie textures. Native
texture ownership/cache/flush behavior continues to follow pinned Melee.

`source/platform_efb_color.h` and `tests/efb_gpu_device.c` adapt
`renderer.c::gpu_efb_copy` from the pinned Melee revision above, GPL-3.0.
The 264x400 framebuffer needs a 512x512 tiled capture. Unlike Melee's linear
capture, this capture stays in VRAM; raw copies to CPU/linear remain forbidden.
Only the audited persistent RGB565 reflection destination is eligible. A
separate RGBA8 render pass and the established DisplayTransfer contract supply
CPU bytes on demand. The ordinary GXCopyTex contract and depth path are unchanged.

`source/platform_efb_depth.h` extends that ownership model for the audited SFA
reflection Z8 destination. Melee's `renderer.c::efb_copy` omits depth payloads
for its coverage-overlay implementation; that behavior is not applicable to
SFA's depth comparisons. This port retains opaque D24S8 bytes in a 422,400-byte
VRAM allocation and converts them through the guarded DisplayTransfer path
when consumed. It does not sample depth as a color texture or raw-copy to CPU.


The five-stage layered family in `source/material_layers.c` applies the pinned
Melee renderer's PICA multiply-add/interpolate approach to SFA's distinct GX
register equations. It reuses this port's owned RGB/alpha texture packing,
retains sum saturation and final transparency, and declines incompatible state.
Retained texgen keys now cover eight GX coordinate generators with complete
source snapshots; the arena remains bounded at 4 MiB. The shared linear reserve
is enforced by synchronized eviction of unpinned textures after optional native
geometry is reclaimed, following Melee's GPU-lifetime-aware cache ownership.


`source/platform_efb_shadow.h` adapts pinned Melee `renderer.c::gpu_efb_copy`
for the audited `newshadows.c` R4 shadow outputs: affected VRAM tile rows,
RGBA4 render targets, bounded cache eviction, and retained target objects
reattached with C3D_FrameBufTex (Citro3D forbids target deletion in-frame).
Departures: a512KiB VRAM cap plus1MiB free reserve; exact CPU materialization
instead of image tags; existing shared transfer scratch; a256-intensity GPU
calibration selects direct quantization or a two-stage correction for rounded
normalized storage, otherwise retaining the original CPU copy. New shadow
outputs do not change game rendering/progression or omit shadow work.
The underlying Melee GPL notices and attribution remain in this directory.


The separately generated game_sphere.v.pica follows the pinned Melee
vertex-dual.v.pica arithmetic structure for specular facing, separate lighting
accumulators and directional-position scaling. It is a new SFA-specific program:
six lights include independent alpha spot lights, and channel1 uses the
existing sphere material's interpolated identity-texture encoding. The uniform
layout remains our FhGpuLighting ABI. Separate DVLB binding avoids exceeding
512 instruction slots; unsupported states retain original ARM lighting.

`include/fh_texture_8.h` specializes pinned Melee `texture_repack.h`'s I8 tile
mapping using aligned four-byte row loads and pairs of 16-bit row segments.
Partial tiles and replicated borders retain the original byte addressing;
unaligned sources retain the original helper. No additional cache or heap
allocation is introduced. All source bytes remain CPU-authoritative after the
existing guarded depth DisplayTransfer; raw depth readback remains forbidden.

Emulator phase wrappers follow pinned Melee renderer.c timing around preserved
calls. Object, environment, collision, scene and UI totals are inclusive;
scene rendering overlaps GX/renderer counters. Exact Foxhollow ABI signatures
are preserved. They introduce no console calls, state changes or CPU override.

The optional Z8 admission check follows Melee renderer.c GPU/CPU ownership
and explicit cache visibility, and the existing SFA R4 backend admission
pattern. It compares a sampled GPU depth texture against the retained safe
DisplayTransfer result. Both use pre-existing scratch buffers; no destination
write, raw CPU depth copy, game-state modification or approximation is allowed.
Failing or insufficiently varied images retain the CPU path. The separate
Azahar correction is isolated from this renderer and from the pinned checkout.

The incremental stream cache flush in platform_graphics.c adapts pinned
Melee renderer.c::flush_dynamic and its queue-completion cursor resets.
This version tracks unrounded byte ends, explicitly aligns cache-line ranges,
and handles speculative indexed-decode rollback. EFB helper vertices use the
same ownership rule. Host tests model noncoherent GPU memory and reject a
rounded-cursor mutation; per-frame counters expose the avoided service work.

Early GPU submission imports pinned Melee port/3ds/early_queue.c/.h,
tools/async_renderqueue.py and tests/async_queue.c (GPL-3.0). Symbols use fh_
and the build uses the existing pinned Citro3D headers. SDK files stay intact:
a private archive replaces renderqueue.o with generated publication/join and
capacity-reservation fixes. Host interleavings exercise the actual generated
functions and reject the unpatched SDK. Renderer submission uses immutable
prefixes after stream flush, retaining all buffers until the normal completion
fence. A256-byte device-fixture threshold stresses overlap; native default is
16384bytes. The feature is opt-in while being evaluated. With prefixes active,
Citro3D gpu_ms reports remaining queue tails; overlap span is not GPU time.

NativeGeometry CPU keys follow pinned Melee renderer.c's ordinary-RAM metadata
ownership. SFA's exact dependency key is variable-sized: common GPU packing
keeps only consumed primary/coordinate/size/fur fields, while scalar packing
retains the complete material. Keys have separate bounded normal-heap admission
and accounting; the linear payload contains only GPU vertices and indices.
The adapted early-queue helper preserves this renderer's pending compatible
draw instead of forcing it into each prefix. All queued resources still wait
for the existing completion fence before overwrite, eviction or reuse.

gx_compiled_fast.h follows pinned Melee gx.c's prepared field/raw-attribute
separation, with an SFA-specific specialization for dynamic GPU-lit meshes.
It reads live arrays, preserves SFA endian and bounds checks, and falls back
to the original scalar decoder for unsupported or exceptional vertices.
Register colors are prepared once per batch; accepted output is compared
byte-for-byte against the original decoder in emulator validation mode.
No Melee BE8 convention, extra memory arena or shader change is introduced.
Melee geometry_bounds.h inspired two measured visibility cache experiments;
both regressed this workload and were removed from production.

fh_alpha_cache.h adapts Melee renderer.c::draw_with_alpha's64-entry exact
predicate cache at the same pinned revision. The256-value alpha reduction is
unchanged; collisions and non-reducible predicates are retained correctly.
SFA additionally avoids reduction when its already-validated raster alpha state
has not changed. This uses1,280normal-RAM bytes and no GPU allocation.

Single-matrix visibility follows pinned Melee geometry_bounds.h's first
rejecting-plane exit. SFA preserves its propagated float24 margin and requires
separate GX/PICA exclusions. Mixed-matrix boxes retain the full common-plane
intersection; all candidate projection rows are validated before early return.
No visibility cache or additional allocation is introduced.

fh_material_register.h follows pinned Melee gx.c::material_state_matches and
material_recolor's separation of changing colours from TEV analysis. This port
recognizes complete single-stage register-copy and texture-times-register
expressions, then reconstructs the exact existing FhMaterial. It does not copy
Melee's game-specific shader plan or assume unchanged register values. Other
expressions retain the general symbolic compiler. Host mutation tests compare
the complete accepted plan byte-for-byte with that compiler, including zero,
clamp, texture dependence and exceptional-value fallbacks. No new allocation,
shader, GPU work reduction, or draw reordering is involved.

Compiled animated-mesh admission extends this port's existing bounded cache
backoff, after reviewing pinned Melee gx.c's owned source snapshots and changing
source handling. SFA still verifies every actual cache hit against owned bytes;
it has no Melee write-epoch assumption. After two misses, a mesh periodically
retries recording instead of filling CPU/GPU caches with single-use packets.
The 128-entry history holds only admission metadata (2,560bytes on ARM), never
vertex values or a reusable identity. All skipped admissions perform the full
current-attribute decode and draw. Verified hits restore normal admission;
collisions can only reduce caching efficiency. GPU/CPU arena limits are unchanged.

Point side rejection extends pinned Melee port/3ds/point.g.pica's W/depth
admission. SFA tests the full expanded box with a1% outward float24 rounding
guard before emitting its four vertices. It preserves wide points crossing
screen edges, color/UV/Q and draw ordering. This adds8shader instructions and
one geometry constant;96CPU/GPU fixture cases include visible samples at all
four edges. No console GPU performance claim is made.

The register-material specialization also supplies a28byte canonical cache key
for these complete expressions, following pinned Melee port/engine/gx.c's
material_state_matches/material_recolor dependency separation. Other forms use
the full prior key. Existing owned cache storage protects plans through nested
texture conversion; no shared mutable temporary plan or new arena is used.

2026-10-04 physical queue timeout (console 9561746b785b): reviewed pinned
Melee renderer.c gpu_efb_copy's tile-row TextureCopy and async_renderqueue.py,
and Citro3D renderqueue.c's in-frame transfer submission. The observed first
unretired command copies 264-wide tiled RGBA8 into a 512-wide padded texture,
with output gaps but flags 8. Unlike the pinned Melee example, this port now
sets flags 12 on all gapped copies, including presentation and shadows, as
required by the hardware register reference:
https://www.3dbrew.org/wiki/GPU/External_Registers#TextureCopy
The contiguous VRAM depth snapshot keeps flags 8. Azahar's sw_blitter.cpp
TextureCopy and rasterizer-cache path do not enforce the gap-mode bit; a
production descriptor guard and actual-submission fixture reject its absence.
The guard also checks minimum size, alignment, occupied spans against owned
allocation capacities, VRAM endpoints and overlap. No allocation, queue
ordering, game work or CPU readback contract is changed. Physical resolution
of the hang remains unverified until a new console run.

Follow-up physical313849a90958 still stalled with the corrected0x0c descriptor.
The gap flag was not a sufficient fix. Rechecked pinned Melee renderer.c
gpu_efb_copy: its efb_capture uses C3D_TexInit (linear), not C3D_TexInitVRAM.
Restore that allocation/ownership pattern, including one initial zero/flush
before GPU publication. Subsequent accesses are GPU writes and GPU sampling;
ARM materialization continues through a distinct output/readback pair. A
registered allocation range admits this sampled-linear destination without
admitting the former raw depth-to-CPU readback. New negative control restores
the padded VRAM allocation and must fail before submission. This transfers
1MiB of budget from VRAM to linear with the existing4MiB linear reserve, not
an increase in total allocated bytes. Console recovery still needs validation.

9a614627c455 also stalled with the linear capture. Neither allocation nor
gap flags is established as the cause. Revisited the same pinned Melee
gpu_efb_copy/async_renderqueue adapter and Citro3D base.c/renderqueue.c.
The in-frame SyncTextureCopy API queues work without a completion wait.
For physical isolation the port temporarily drains producer commands and
then each individual TextureCopy through the existing libctru queue, using
bounded phase-specific waits. No queue clearing, buffer recycling, draw
reordering, descriptor change or CPU access to GPU-owned storage is added.
This departs from Melee's asynchronous copy schedule to distinguish a
producer stall, transfer stall or ordering issue on the next hardware run.
It is not a validated hardware fix or performance improvement. The libctru
queue has up to three outstanding commands, so earlier pending-tail traces
alone do not establish which engine stalled.

The isolated947aeefb9197 run completed its producer fence and stalled with
exactly one padded color transfer outstanding. The replacement keeps Melee's
GPU-sampled capture and tile layout, but decomposes output gaps into contiguous
row transfers with explicit nonzero line widths. This departure follows the
measured physical failure; the internal reason remains unknown. Retain bounded
fences and allocation checks; no CPU access to GPU-owned tiled storage. The
submitting fixture rejects the former single output-gap transfer even though
Azahar accepts it. Host tests verify every copied byte and untouched padding.

An optional fused decoder follows pinned Melee port/engine/vertex_decode.h and
gpu_vertex.h's prepared attribute fields and raw GPU vertex layout. SFA retains
its own88byte native layout, little-endian array handling, live shader uniforms,
exact source probes and scalar fallback. It removes the intermediate148byte
vertex only on eligible animated cache misses. Current defaults keep it off;
host equivalence alone is not native game or hardware validation.
# 2026-10-04 sampled capture DMA departure

Consulted Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c
gpu_efb_copy and tools/async_renderqueue.py, plus Citro3D
9f21cf7b380ce6f9e01a0420f19f0763e5443ca7 source/renderqueue.c.
Physical2f02676c2b22 stalls even on a contiguous raw VRAM-to-linear row after
the producer drained. The port keeps Melee's sampled linear capture ownership
and tiled bytes but uses libctru GX_RequestDma row copies rather than raw
TextureCopy for that destination. This is an original adaptation; Melee is
not evidence of DMA hardware success. Retain FrameSplit before each submission
for the pinned queue capacity reservation, as SyncTextureCopy did internally.
Only the registered sampled allocation is admitted; CPU raw depth readback
remains rejected. Initial CPU flush, GPU ownership, producer and transfer waits
prevent stale CPU writes and live-buffer reuse. Physical validation pending.

The485f0ff97597 hardware run subsequently confirms completion of all sampled
color DMA rows and consumer rendering, then stalls on the distinct contiguous
VRAM depth snapshot TextureCopy. Extend DMA to that separately registered VRAM
snapshot, with the same copy-time depth semantics and unchanged CPU conversion.
Melee's omitted depth payloads cannot be used for SFA, which consumes them.
Bounds and ownership validation permit only these two owned DMA destinations.
Production game checks are not weakened. Hardware depth-DMA validation pending.

Azahar9e6f523a57fac9564ac0bf8286db3c3702d301ec OpenGL D24S8-to-RGBA8 shader
reconstructs integer bytes through floating-point depth sampling. Independent
surfaceless OpenGL checks find10485759byte-mismatched pixels among all16777216
packed depth values on llvmpipe; a24-bit rounding expression still mismatches
5242879. Packed GL_UNSIGNED_INT_24_8 readback has zero mismatches. Custom Azahar
now declines this accelerated reinterpretation so the existing rasterizer cache
flush/upload path preserves packed bytes. No PICA/game work is omitted, no
emulated clocks changed. Host rendering cost changes; timings across emulator
variants are not performance evidence. Prior binaries/manifests are archived.

## 2026-10-04 experimental typed GX state handoff

Consulted Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
`port/engine/gx_fifo.c`, `tools/generate_gx_fifo.py`,
`port/3ds/gx_thread.c`, and `port/3ds/render_worker.c`, including the APT
suspend drain and hardware finding that rendering on the system core is slow.
The generated recorder/replay approach, explicit pointer classifications,
monotonic ring counters, release/acquire publication, core-2 affinity and
256 KiB worker stack follow those implementations. Derived generator/header
code retains GPL-3.0 provenance. The bounded SFA ring and platform adapter
are new adaptations, not Melee's BE8 ABI or HSD source-write contract.

Departure: this first stage records state only, within the existing immutable
fur scope. Draws, getters, immediate vertex writes, cache events and scope end
drain the ring before executing on the main thread. Matrices, light/texture
objects and optional filters are copied into packets. GXSetArray records the
address but the worker does not read its asset contents. Rendering, texture
data and palette data are not yet asynchronous. Core 2 cannot simultaneously
host the current audio worker. Ring storage is 256 KiB; no desktop-sized FIFO
or source snapshot allocation is imported. This is an explicit emulator-only
build/run experiment, disabled in console builds, with bounded failure/exit
handling. It establishes no console performance improvement.

The next emulator-only mode also translates model display lists on core 2.
SFA `objprint_dolphin.c::objRenderFuzz` explicitly registers its primary model
lists after the first pass and borrows the four existing immutable attribute
ranges until scope end. This is narrower than Melee's across-frame HSD source
epochs: simulation never advances while a lease is live. All indexed arrays
must belong to those ranges; unknown lists remain synchronous. Matrices and
texture/light objects remain copied records. Texture pixels/palettes stay
alive through this audited render-only loop; arbitrary post-render callbacks
end the lease, noise allocation drains first, and movie decoding drains before
writing/reallocating pixels. Reviewed `shader_dolphin.c`, `newshadows.c`,
`texture.c`, `main/thp/n_options.c` and `thp_shim.c` for those source lifetimes.

Joint-matrix sources have a separate explicitly registered range. GX loaders
already copy their coefficients, so cache notifications for that range can
replay as ordered metadata invalidations, without borrowing/dereferencing it.
Other cache operations and readbacks still drain. This preserves every cache
operation and draw. The renderer starts its frame on the producer before
handoff; game/audio callbacks remain on the producer, including while it waits.
The existing Citro3D context has one CPU owner at a time, with release/acquire
handoff. FPSCR arithmetic control follows the producer at each scope. Added
256-entry list metadata, bounded worker stack probes and barrier counters;
existing GPU allocation and lifetime rules remain in force. Console promotion
still requires further correctness/performance evidence and a hardware test.


## 2026-10-04 isolated render LTO and synchronous model source scopes

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
`tools/engine_build.py` selective ThinLTO and misaligned-load hardware notes.
`tools/build_render_lto.py` is a new isolated little-endian LLVM18 merge/O2/lower
pipeline for objprint_dolphin, shader_dolphin and three native GX helper units.
Unlike Melee it needs no BE8 conversion or libc bridge; existing Foxhollow ABI,
signed-wrap and noncontracting floating-point flags remain. SDK/platform units
and display-list translation remain separate, preserving profiling boundaries.
Original archives are hash-checked unchanged; replacement members must be unique
and retain all exported symbols. Emulator-only opt-in; no hardware promotion.

Consulted Melee `port/engine/gx.c` source_valid, mp_gx_source_write and source
clock/epoch invalidation. SFA does not have equivalent complete source-write
coverage. The new opt-in model scope therefore lasts only through the synchronous
modelDoRenderInstrs instruction loop, after animation, morphing, skinning,
after-bones callbacks and matrix preparation. Only its four bound attribute
ranges qualify. Models with a per-material render callback are excluded;
post-render callbacks end the scope. Nested fur scopes and worker dispatch are
excluded. Sources still compare on their first use in each scope, and keys/lists
remain compared for each draw. No identity survives across animation or frames.
Range invalidation first excludes writes outside all declared ranges; overlapping
sources still lose verification. Null invalidation still clears all verification.
Checked mode compares every would-be reuse. Existing PICA ownership is unchanged.


## 2026-10-04 experimental compact dynamic vertex attributes

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
`port/3ds/renderer.c`: vertex_attributes, clamped_attributes, vertex_pointer,
matrix_vertex_pointer, indexed_draw and stream flushing. In particular, its
explicit64-bit attribute permutation fixes the SDK int-shift bug with higher
attribute slots. Also inspected pinned Citro3D attribs.c loader/fixed binding.

New SFA adaptation gathers signed16position and signed8normal components into
52-byte vertices, keeping existing float UV arithmetic and moving constant
color/scales to fixed attributes. Input0short4,1fixed,2-4float3,5short2,
6signedbyte4,7fixed; explicit permutation0x76543210, loaded permutation0x654320.
Eight inputs fit PICA's12attribute limit. Position and normal conversion uses
four additional shader instructions in previously unused r14/r15; scale bounds
and exact powers of two preserve the float24 input values. Picasso rejected
an initial two-input-register multiplication; generated shader now moves to
temporaries first. No shader uniform/boolean allocation grows.

Only simple GPU-lit, integer-position/normal layouts admitted by existing
FastCompiled qualify; floats/NBT/eye-space texgen/CPU lighting retain fallback.
Live source arrays are read every draw with existing bounds checks. Stable
geometry still uses the existing retained cache. No CPU animation is skipped.
The 52-byte payload lives within the existing88-byte-per-vertex stream
reservation; this first experiment reduces packing, not allocated linear RAM.
Indices are local under an isolated buffer binding. Prior pending draw flushes
before switching; compact draw emits fully before restoring ordinary layout.
Existing cache flushes and GPU retirement own the entire reserved span.
Checked mode expands each compact input and compares it to the actual scalar
decoder plus renderer. Its scratch uses the unused tail of the reserved span,
with an explicit nonoverlap assertion. Build/run opt-in, emulator-only; console
shader and attribute layout remain original when compile flag is absent.


## 2026-10-04 ordinary post-only texture coordinates (experimental)

Reviewed pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/engine/gx.c::texgen_renderer and geometry_key: retain original texture
attributes and evaluate animated texture/post matrices through live uniforms.
SFA adaptation extends its existing ordinary 2x4 UV generator to post-only
matrices. It reuses the existing three uniform rows, shader branch, vertex
layout and GPU fences. No new shader instructions, buffers or OS services.
Unlike Melee's separate primary/post banks, this path admits exactly one
transform. It never multiplies two matrices together or reorders arithmetic.
Normalization, generated coordinates, projective post rows, per-vertex texture
matrices and unsupported materials keep the scalar path. Post-only output q
must stay one because secondary PICA samplers lack the primary q output.
Flag fh_post_uvgen_enabled defaults zero; only headless --post-uvgen currently
enables it. Console package349 remains unchanged. Host selector/fallback tests
cover 60000 exact coordinate components with changing matrices plus rejected
nonfinite/projective/composed inputs; those tests do not emulate float24.


## 2026-10-05 paired live bounds and collision alignment

Pinned Melee gx.c and geometry_bounds.h reviewed for paired bounds. Only
current box validation/model error magnitudes are shared between SFA's GX
and PICA rejection tests; both tests and propagated margins remain. No
plane/visibility cache, shader, allocation or scheduling change. Opt-in only.

Physical dump184 shows VFP read at gTrackGridOrigin+0, address0x007a3de1,
from trackGetIntersect2. Instruction bytes match collected5bf7b640444d. The
byte array is cast to s32* by several collision routines but had byte alignment.
Generated ARM source now explicitly aligns its declaration/definition to4bytes
and asserts that alignment, preserving all260bytes and collision arithmetic.
Melee tools/build_game.py similarly aligns byte containers consumed as words;
Melee's BE8 byte access wrappers are inapplicable to this little-endian buffer.
Vendor source untouched. Host test executes the actual linked ARM entry with
a strict VFP memory hook: unfixed ELF faults, fixed ELF reads both coordinates.
Unicorn's default permissive behavior alone is not used as alignment evidence.
No change to gameplay or progression logic, and no claim of improved FPS.


## 2026-10-05 CPU-only movie PCM cache notifications (experimental)

Reviewed pinned Melee port/engine/os.c DCFlushRange/DCInvalidateRange mapping,
port/3ds/audio.c owned NDSP copy and cache visibility, and port/3ds/gx_thread.c
core ownership. SFA's THP callback performs cache operations on native CPU PCM
rings; routing these into GX scans hundreds of unrelated texture/geometry
entries. A native wrapper recognizes only ranges wholly inside the private
MusyX mix_buffers or the1280-byte movie DMA ring. Such notifications require
no graphics visibility action: both producers and the movie mixer run on CPU,
and platform_audio.c copies/flushed owned linear NDSP buffers separately.
Unknown, zero-length, partial and overflowing spans keep the original GX path.
Only the three notifications inside AttractMovieAudio_DmaCallback are adapted;
no general game cache operation, audio sample, callback or queue is removed.
Movie decode and its callback stay on main; worker ownership is unchanged.
Flag defaults0 pending evaluation. Production-function ARM test covers162919
checks with native ranges, off/on, boundaries, both read/write fallbacks and
32-bit overflow. No shader/storage/heap changes; counters add12B BSS.
# 2026-10-05: pipelined movie synth blocks (optional)

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
`port/3ds/audio.c` (owned NDSP copies and flush-before-publication),
`port/3ds/gx_thread.c` (core-2 ownership, joined stop and suspend barrier), and
the existing Foxhollow recursive synth lock. SFA's movie DMA callback cannot
move wholesale to a worker: it touches cooperative DVD/movie queues.

The optional native audio-synth-jobs path leaves that callback on main and
posts only its next `sndPCRender` into one lock-protected slot. That output is
two ring positions beyond the current DMA source. A pending job prevents
another DMA callback until complete; every synth API entry drains an unstarted
job before mutation. Movie PCM mix, callback order, source delay and final NDSP
copy/flush remain. The existing worker consumes the job under the same synth
lock; no new thread, stack or linear allocation. Stop/suspend drain pending
work. One 8192-byte normal-memory packet replaces transient stack packet
storage, retaining the original four-slice/640-frame NDSP submission size
across producer polls. All unrelated callbacks keep synchronous mixing.

This is not permission to run the movie/OS callback on core 2. Default is off
pending matched emulator and console evidence. Host production-DMA oracle
checks sixteen delayed schedules, both movie source-delay modes, exact PCM,
callback ordering, synth mutation ordering and completion accounting under
ASAN/UBSAN. Console timing and hardware lifecycle validation remain open.

The final synth candidate retains seven NDSP packets rather than four, with
synchronous refill below that threshold, and1ms renderer service cadence.
Console dbbcba23f7d2 enables it after evaluated offline tests; zero-file overrides
retain fault isolation. No new worker or DSP flush mechanism.

2026-10-05 log-I/O follow-up consults pinned Melee port/3ds/log_io.c. The queue
retains64KiB owned payload but sends a complete stdio block as one request
instead of four16KiB writes. File offsets/flags/errors and close/shutdown joins
are retained; no save/asset paths enter the queue. Unlike Melee, full queues
still backpressure rather than drop records. This experiment follows measured
late SD waits; fewer calls is not a demonstrated physical speedup.


2026-10-05 uniform-row experiment: consulted pinned Melee
native-3ds/port/3ds/uniform_upload.h and renderer.c at
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e, and Citro3D uniforms.c at
9f21cf7b380ce6f9e01a0420f19f0763e5443ca7. Adapt the rule that cached uniform
rows represent values actually uploaded, not unused inputs from the previous
draw. New native little-endian helper uses exact bits,96rows/1632B ordinary
BSS, no BE8 conversion or Melee shader/register layout. Invalidate on every
program bind to cover shader constants and helper passes. All existing row
requests and shader math remain; only duplicate SDK setters are suppressed.
Feature defaultoff; emulator --uniform-rows and matching SD config enable it.
Synthetic independent register-bank oracle covers10008draws, constant clobbers,
full/flat/transform-only transitions, signed zero, infinities and NaN payloads
under ASAN/UBSAN. Whole-scene and physical benefits not yet established.


2026-10-05 palette eligibility experiment follows pinned Melee
port/engine/gx.c geometry_key/GeometrySource and port/3ds/uniform_upload.h
(c2ebe7c769b3287f2b3748173e8fe5ec24ef408e): reuse only exact owned input
values; live lighting/material registers and every shell still execute.
The original SFA normal/model bounds and Gershgorin checks are retained as
the defaultoff reference. Optional palette_validation modes1/2 use an eight-entry
cache of all480normal+480model bytes, FPSCR rounding/flush controls and result.
No borrowed matrix pointer or per-vertex lighting cache. Model palette comes
from begin_primitive's original copy of the same first ten GX position matrices.
Checked mode recalculates the scalar proof on every hit. Roughly7.8KiB bounded
normal BSS, no linear/VRAM allocation, shader changes or new thread. Host
ASAN/UBSAN240000exact decisions exercise every coefficient lane, invalid floats,
eviction and distinct FP-control keys. Actual ARM GX/image/performance checks
remain required; no physical benefit claimed.


Palette cache rejected: acbecd1268bfa837ecbde1797e577ec87f9d2d174feabfd17557da34fdd45fb8
menu-palette-validation completes200retraces42.247s, drained owned exit.
Only58hits/9369misses (0.62%reuse). Window130-167CPU153.804ms,interval180.193ms,
379.474draws/95347.474vertices. Prior different-binary reference151.959/176.129ms;
this is not a matched speedup experiment, and the measured reuse is itself
insufficient to keep the design. Do not spend another boot on this rejected
cache. Full source archive and hashes artifacts/palette-validation/experiment;
result.json, host.json, arm-gx/summary.json preserve evidence. Actual ARM
existing GX oracle passed92101checks with244checked palette proofs.

2026-10-05: experimental shared audio/GX executor consults pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/gx_thread.c and render_worker.c.
Keep one application-core2 owner and the late APT source/queue drain; continue
using existing GPL-3.0 typed GX packet/lease adaptation. SFA departs by attaching
bounded64-packet slices to its existing MusyX thread with a256KiB stack, instead
of adding a second owner or importing Melee's snapshot arenas. Audio uses a
nonblocking synth lock on the shared owner and its own saved FP controls;
game/stream callbacks remain on the producer. GX source joins and GPU buffer
fences stay distinct. This scheduling code is local implementation, with no
BE8 bridge imported and no physical-console correctness claim from HLE tests.
Removed the cache and7808B BSS from active production code and removed its
runner mode. Standalone test points to archived candidate. No console upload.

Next structural dependency audit: bounds data contains consumed matrix masks,
but list_outside can return after its first visible vertex before a complete
mask exists. Do not use that partial mask to omit lighting validation. Complete
owned compiled packet metadata is the safer candidate for deriving exactly
which position/normal matrices a whole list consumes. Pinned Melee
uniform_upload.h matrix_mask principle applies, with SFA palette and fallback
semantics requiring independent coverage before adoption.


2026-10-05 consumed-matrix scope candidate: pinned Melee
port/3ds/uniform_upload.h uploads only matrix_mask rows. Adapt its dependency
principle to SFA lighting/normal-texgen eligibility, retaining all actual draw
work. The compiled topology now owns the union of first-byte PNMTX indices
from every complete input packet, including discarded triangle/quad tails.
Only a current direct PNMTX descriptor at bytezero may consume that metadata;
invalid rows conservatively yield allten. Complete list bytes remain compared
on reuse; changed lists regenerate the union. Matrix values remain live and
are checked each call. Optional palette-scope modes1/2, defaultoff, narrow
normal/model/texgen palette checks after complete compilation. Checkedmode
independently walks every original packet. No partial bounds mask is reused.
1024B extra compiled metadata plus small counters; no arena/linear/VRAM growth
or shader change. Reference mode preserves previous preparation. Previous
failed whole-palette cache remains archived, not reintroduced.

2026-10-05 optional ordinary-model GX handoff. Reconsulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/engine/gx.c owned geometry inputs
and port/3ds/gx_thread.c sole translator ownership and source drains. The SFA
adaptation extends its existing bounded typed FIFO and four-range leases to
modelDoRenderInstrs after animation, morphs, skinning and after-bones callbacks.
Models with either material callback are excluded. At most256 display lists
are registered before commands publish. renderOpMatrix arguments are copied
by the existing GX matrix packets; original arrays/lists remain borrowed only
until the loop-end reader join. No frame snapshots, arena growth, extra worker,
BE8 bridge or shader change. The producer cannot advance beyond that join.
Enabled only by model_geometry_scope.txt=1, still OFF in the installed console
candidate. Existing audio/GX shared-owner lifecycle rules remain unchanged.

2026-10-05 live vertex-color compiled decoding: reconsulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/engine/gx.c prepared VertexField,
raw attributes and texgen_renderer separation. Extend the existing SFA decoder
rather than import BE8 color loads: six original GX color formats and native
array endian rules still use the production decode_color/indexed functions.
GPU-deferred and unlit material channels may consume current vertex colors;
constant CPU channels retain their precedence, and variable CPU-lit channels
remain scalar. No animation values are retained by this fast path. Native
packing selects the same primary color components directly into the existing
88-byte PICA layout. Compact integer layout rejects color-dependent packets
because its color is fixed per draw. Existing shader, stream allocations,
cache flushes and GPU fences are unchanged. FastCompiled adds12bytes of local
metadata on ARM; coverage counter adds4bytes BSS, no heap/linear/VRAM arena.
The colored path additionally admits absent normal arrays only for flat GPU
lighting, preserving the scalar default(0,0,1). Affine UV postmatrices whose
last row leaves q=1 use the same ordered transform arithmetic with current
coefficients. Nonfinite sources/coefficients and projective postmatrices keep
the scalar path. Compact fixed-attribute packing excludes these new postmatrix
cases; it has not gained a second implementation of that transform.

2026-10-05 direct compiled output and CPU-lit native packing: reconsulted pinned
Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/engine/vertex_decode.h and
gx.c vertex/texture_coord. Its prepared-field/raw-input boundary is adapted to
SFA's existing little-endian scalar decoder and 88-byte PICA stream. The compiled
caller ALREADY disabled the packet cache; no removal of live hashing overhead
is claimed. Compiled decode now writes the caller's vertex directly, avoiding
one intermediate148-byte copy. Expanded primitive caching remains unchanged.
Optional native_decode now also admits raw1 CPU-lit/emboss meshes, executing
all original per-vertex lighting/UV work into one stack vertex before writing
the fenced GPU stream. It does not first store a full148-byte staging array.
Projective UV division, primary-color selection and zero GPU normal attributes
match the existing renderer. Failed late matrix admission publishes nothing
and restores logical decoder counters/identity before normal fallback. Checked
mode compares every native packet with actual scalar decode and emit_vertices.
GPU-light counters are now gated by raw-position bit2, so CPU-lit packing cannot
misreport itself as GPU lighting. No shader, heap arena, GPU cache visibility,
command ownership, GPU copy path or fence changes; optional mode remains off
for installed console builds. The existing topology/compiler and source-byte
validation remain intact, including full index order and all game work.

2026-10-05 material lookup without temporary key copies. Reconsulted pinned
Melee gx.c ShadeKey/shade_cache and material-program binding ownership. SFA's
key is much larger and includes texture metadata, so its exact existing key
format is retained. A local23-span view reads the same current state fields;
all span lengths end on32-bit boundaries, preserving the original word-wise
hash across concatenation. Cache hits still compare every serialized byte.
Only misses copy bytes into the existing owned128-entry cache. No state epochs,
address-only hits, compiler simplification, stale register reuse or cache budget
increase. The compact register-plan key remains canonical and locally owned.
The three material compilers and fragment admission are synchronous pure state
readers; the view never survives material(). Lazy texture conversions still
occur later and cannot retain pointers into this stack view. Compared with
building the old maximum-size stack key, the view requires244bytes on ARM and
copies only24bytes of reordered raster fields for a general lookup. Hashes and
cache replacement/probe/fallback behavior remain unchanged. Two diagnostic
counters distinguish owned miss-key copies from avoided hit-key temporary
copies; they do not claim memory-bus measurements or console speedups.

Material-view disposition: the combined game test did not improve cadence and
material lookup measured slower. Its implementation/fixture/patch are archived
under artifacts/material-key; the active renderer restores the prior contiguous
key implementation. The standalone view header/test are retained as experimental
evidence but are no longer included by the native renderer. No console upload.

2026-10-05 synchronous model source verification: reconsulted pinned Melee gx.c
GeometrySource/source_valid and rolling write tracking at c2ebe7c769b3287f2b3748173e8fe5ec24ef408e.
SFA still does not assume that cache-flush notifications cover every game write.
Instead, mode2 of the existing opt-in model scope uses the same audited bounded
immutable model loop after animation/skinning and excluding arbitrary callbacks.
It compares each source completely before its first reuse and ends ownership at
the existing loop boundary. Unlike mode1, it starts no GX worker scope and does
not add a reader join; original fur scopes retain their existing queue lifetime.
Range writes invalidate verifications, and checked mode still compares every
reuse. No source epoch survives this synchronous call. This separates source
verification reuse from the earlier more costly model dispatch experiment.
One ownership flag and a cumulative reused-byte counter are added; no arena,
linear/VRAM allocation, shader, GPU copy, cache flush or thread changes.
Source acquisition now uses the same source_valid contract as replay, including
complete first comparison, token/range ownership, checked mode and invalidation.
It no longer unconditionally repeats a full comparison of an already verified
binding within the audited scope. No cross-scope or frame-wide write assumption
is introduced. Host tests include reacquisition after an explicitly invalidated
in-place write; real-dispatch tests stall the reader after an asynchronous scope
and require the following source-only model to complete directly with no new
queued records or reader waits.
2026-10-05 measurement clock and exact console executable replay.
Consulted pinned melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e,
port/3ds/services.c mp_native_ticks and port/3ds/early_queue.c raw timing.
Keep Foxhollow's existing 40.5 MHz game clock unchanged. Emulator-only
GX phase/shading measurements now accumulate svcGetSystemTick directly,
following Melee's raw queue measurements, avoiding repeated software division
inside each instrumented draw. JSON records the 268123480 Hz counter explicitly;
host decoder fixtures keep their synthetic clock. This reduces instrumentation
overhead, not console game work. No console executable or GPU path change.

Custom Azahar HID replay uses the pinned 9e6f523a57fac9564ac0bf8286db3c3702d301ec
HID/movie publication point: ordinary pad and calibrated circle values are
replaced only with explicit FH_HID_SCRIPT opt-in. The platform retrace symbol
is read, never written; normal shared-memory deltas, direction bits, timestamps,
polling and guest execution remain in Azahar. This departs from Melee's
MP_SMOKE_TEST in-process input injection to exercise the exact packaged console
executable and real hidScanInput/Select shutdown path without native test hooks.
It does not validate C-stick, ZL/ZR, touch, physical input, CPU/GPU timing or
hardware synchronization. Source/binary backups, patch, base provenance and
restoration hashes are in artifacts/emulator-build/scripted-hid. New helper is
GPL-2.0-or-later for linking into Azahar; no upstream license is replaced.
2026-10-05 bounded GX command retirement.
Reconsulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e,
port/engine/gx_fifo.c: producer argument copies/release publication and consumer
cursor/released split with CONSUME_BATCH=4096words. Our optional dispatcher
batch acquires one published prefix and retires only commands already executed.
The same maximum64records per existing audio/GX worker slice is retained; ring
remains256KiB and no source lease, APT join, core affinity, shader or GPU fence
changes. Later publication waits for the next slice. Empty/wrap-only prefixes
retire correctly. Producer cached retirement is a conservative lower bound;
an acquire refresh is mandatory before claiming additional free space.
Departure:64records instead of Melee's4096word release interval, and release/
acquire instead of Melee's stronger seq-cst sleeper protocol. This port retains
its existing timed event wakeups and polls; it does not import the sleeper
handshake or remove a wait. Default off, opt-in --gx-dispatch-batch for emulator
comparison. Tests cover concurrent wrap, unpublished data, withheld retirement,
new publication during a batch and actual leased-source end joins, including
intentional early-retirement and missing-join negative controls.
2026-10-05 SDK memcpy link experiment: reread pinned Melee
port/engine/libc.c and byte_memory.h for bounded CPU copying, native byte
preservation, alignment peeling and non-overreading tails. --sdk-memory-copy
links memcpy to the existing original fh_memory_copy ARM implementation via
__wrap_memcpy; the alias also receives SDK __aeabi_memcpy/4/8 tail calls, verified
in the linked ELF. No imported BE8 bridge or SIMD/VFP assumption. Arbitrary
alignment retains byte handling; aligned copies use existing LDM/STM. Cache
flushes, GPU transfers, fences and ownership are unchanged. The option is
emulator-only pending evidence; it does not replace memmove or promise overlap
semantics. ARM guard-boundary tests and menu comparison are recorded under
artifacts/sdk-memory-copy; console performance remains unverified.

2026-10-05: display-list material preparation scope (opt-in).
Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e,
native-3ds/port/engine/gx.c prepare_material_program (structural plans,
live bindings, independent validation) and port/3ds/renderer.c raster-state
invalidation before copies/target changes. This is an original adaptation:
Foxhollow's decoder accepts only primitive vertex commands, so a single
GXCallDisplayList provides a bounded interval for repeated material queries.
The scope owns one FhMaterial copy; alternate draw-state pointers bypass it.
Probe-null and CPU-fragment draw results are distinct. Non-triangle primitives,
nested lists and main-thread game/audio callbacks invalidate reuse. Producer
callbacks publish an atomic invalidation epoch without touching worker-owned
cache storage; the renderer still relies on existing source leases and joins.
No new thread, source lifetime extension, GPU fence change or memory pool.
Checked mode invokes the ordinary material preparation and fur classifier on
every hit and rejects differences. Scope exit uses compiler cleanup on all
normal returns. Mode zero remains the default, including console builds.
This does not reuse a material across shell draws, discard shells, or cache
animated uniforms/texture content. Host sanitizer fixtures and game evidence
are under artifacts/material-scope and docs/IMPLEMENTATION.md.

2026-10-05 lazy CPU texgen setup and direct compiled-fur bindings.
Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/engine/gx.c texgen_renderer/geometry_key/GXBegin: distinguish attributes
already resident on the GPU from live uniform work and scalar fallback work.
The original SFA adaptation resolves the CPU texgen result-cache matrix key only
when scalar decoding consumes it. Matrix binding validation, dependency scans,
GPU admission and complete geometry/source keys remain in their existing places.
--lazy-texgen checked evaluates cache hits through the scalar arithmetic and
compares complete coordinate bytes; off retains eager preparation. No texgen,
shell or shader work required for output is dropped, and no pointer lifetime
extends beyond the existing primitive/source ownership. Added32B of preparation
metadata; existing bounded result-cache capacity unchanged.

A rejected compiled-fur experiment used fh_fur_mesh_pipeline_direct to map
samplers back to original GX texture slots and avoid full FhDrawState copies.
Complete material/binding sanitizer checks passed, but integration increased
vertex repacking and regressed cadence. Its precise cause remains unresolved;
binding equivalence alone does not establish renderer lifetime/cache equivalence.
Restored the original compiled-fur state snapshots and archived the experiment
under artifacts/experiments/fur-direct-binding. CPU fallback snapshots were never
changed because their pointer identity also controls native geometry recording.
No extra worker, GPU transfer, fence change or Melee-specific BE8 code is added.
ARM scalar tests and integration evidence are recorded in artifacts/lazy-texgen
and docs/IMPLEMENTATION.md.

2026-10-05 optional second retained-geometry set.
Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/engine/gx.c geometry_set, geometry_find and geometry_start: bounded
associative lookup, owned-source validation, and oldest-entry replacement.
That reference currently uses 256 sets with eight ways. This port keeps its
existing 128 sets with four ways and fixed 4 MiB CPU arena. The experimental
--retained-two-sets option checks another distinct four-entry set only after
the original lookup misses; free primary entries remain preferred. This is an
original placement-policy adaptation, not copied Melee code. It addresses a
possible source of address-layout sensitivity without enlarging either cache.
Key, list-byte and source-token comparisons, scope invalidation, arena overlap
retirement and GPU ownership are unchanged. Four scalar counters add 16 B.
Default remains off; the option is selected only by the emulator test config.
The ARM fixture exercises five colliding lists with exact hit/miss assertions,
source/list mutation, alias bindings, matrix/texgen changes and source sharing.
Actual game results and limitations are recorded in docs/IMPLEMENTATION.md.

2026-10-05 producer matrix register mirror.
Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/engine/gx.c GXLoadPosMtxImm/GXLoadNrmMtxImm/GXLoadTexMtxImm and the
existing typed FIFO adaptation in tools/generate_gx_dispatch.py. Matrix inputs
are copied into owned register storage, and a 2x4 write preserves the last row.
The new optional producer mirror compares original bytes before queue reservation
and suppresses only a register overwrite with identical known bytes. No floating
point arithmetic, pointer identity inference, skipped draw, or retained borrowed
matrix is used. Position/low texture ID aliasing and post-matrix IDs follow this
port's translator semantics, which differ from Melee's simpler texture bank.
An identity helper invalidates position slot9; direct-call barriers and new source
scopes invalidate the mirror. Epoch wrap clears all knowledge. FIFO publication,
reader joins, copied changed payloads and GPU submission are unchanged.
The fixed producer-owned mirror costs14340B normal BSS; there is no new allocation,
thread or linear/VRAM use. --gx-matrix-filter is opt-in for emulator evaluation.
The real GX setter fixture compares complete state after filtered/unfiltered
commands, including every bank/slot, partial writes, aliases and reset.

Host profiler extension: tools/build_azahar_profile.py can also record LR and
R2 at selected memory-function entries in bounded tables separate by core and
function. It saves/restores host registers and flags and changes neither guest
code nor guest ticks. Requested lengths do not count early-exit compare bytes.
This is an original diagnostic on the pinned Azahar/Dynarmic tree; the tested
ordinary emulator and original sources are restored after the custom build.

2026-10-05 consumed source ranges (experimental, disabled by default).
Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e,
port/engine/gx.c geometry_start/geometry_finish and source validation. Adapted
the principle of retaining only safely bounded source coverage. Foxhollow's
new fh_gx_source_ranges.h parses vertex-only lists at recording time because
its decoder has no matching Melee field-range accumulator. NBT3 offsets,
array bounds and unknown commands are checked; failures retain full arrays.
Melee's optional prefix sharing and write-epoch verification are not assumed.
Exact owned byte comparisons and source token retirement remain in force.
The game integration regressed despite fewer requested source bytes; keep off.

2026-10-05 unmasked fur alpha family.
Consulted pinned Melee port/3ds/renderer.c layered/shield TEV setup, including
independent alpha sources and stage ordering, plus the existing imported
texture decoder. The new SFA-specific family is identified by complete TEV
state, never object/asset identity. Its final alpha is clamp(noise.a + A2),
where the existing family uses clamp(noise.a * bump + A2). Retain the same
indirect coordinate reads and RGB equation; omit the unused packed bump-alpha
sampler dependency for this family. CPU scalar fur and PICA plans both reflect
the distinction. Base/mask UV/filter equality is required only when packing
the bump mask into base alpha. No new GPU transfer, allocation, service,
threading or hardware timing assumption. General fragment evaluation remains
an independent comparison route in the private captured-draw fixture.

2026-10-05 compact retained dependency keys (opt-in experiment).
Consulted pinned Melee port/engine/gx.c geometry_key/geometry_find: raw GPU
attributes omit live uniforms, while baked outputs retain every dependency.
SFA's previous2080B dense key remains the oracle. The new original serializer
retains its common header, descriptors, active format/binding metadata and
conditional CPU lighting/texgen fields, omitting zero-filled inactive storage.
Checked mode independently constructs the dense key then serializes it for
exact byte comparison. Full original display-list/source-byte verification,
source generations and GPU ownership are unchanged. Arena offsets include
the exact key length and representation version. Existing512 entries gain
8B each (4096B BSS), with no added heap/linear/VRAM pool. Enabled only by
--retained-key fast|checked in emulator runs until meaningful benefit is shown.
2026-10-05 model matrix block candidate: reconsulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/gx_thread.c reader ownership,
draining before APT suspend, and port/engine/gx.c owned matrix/state inputs.
The SFA little-endian typed ring now has one optional owned matrix block per
renderOpMatrix instruction. The producer writes coefficients directly into
reserved FIFO storage and publishes only after all entries are complete.
Position/texture/normal order and aliasing are preserved per entry. A shared
normal/texture matrix is stored once, then copied to both original banks by
the sole consumer. An unfinished block rejects other commands/barriers.
The original game matrix calculations and all draws remain; only calls and
transport are grouped. Outside a worker scope, the same protocol applies
synchronously using1208B normal BSS scratch. Counts above12 keep the original
setters. No new thread, borrowed source lifetime, GPU copy, linear allocation,
shader or suspend behavior. Opt-in only; hardware speed is unestablished.
2026-10-05 zero second indirect matrix in unmasked fur. Pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/engine/gx.c geometry_key and
port/3ds renderer ownership are the reference for separating immutable vertex
coefficients from live shell uniforms. Peppy's captured SFA material uses the
first diagonal matrix at exponent-1 and an all-zero second matrix at-15.
Admit precisely that additional family and bake only its first sampled offset
coefficient. Every shell, lighting operation, texture read and alpha equation
remains. The owned native-geometry key's existing fur word distinguishes the
two coefficient forms, preventing aliasing when lists/textures match. Frame
and source-write invalidation remain intact. No new shader, arena, memory pool
or GPU transfer; unsupported matrices retain ordinary coordinate evaluation.

2026-10-05 sampled physical renderer attribution: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/render_worker.c, including its
one-in-eight timed worker jobs, explicit core2 ownership, retirement fences,
and recorded system-core slowdown. Our sampling uses complete presentation
intervals (one in16 by configuration), so disjoint direct-draw stages can be
related to inclusive GX/shading stages without extrapolating per-job weights.
Counter access stays under existing GX ownership; presentation changes sample
selection only after the existing CPU reader fence. No new threading, GPU
commands, SDK data layout or cache lifetime is introduced. Raw timer ticks
and the pinned SYSCLOCK_ARM11 frequency are serialized through the existing
buffered diagnostic policy. This is new instrumentation code, not a Melee
renderer import, and does not establish physical performance or correctness.

2026-10-05 producer-owned GX state patches: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/render_worker.c,
render_snapshot.h and tests/render_worker_tests.c, plus its worker failure
handoff test. Adapt the owned-payload, bounded release/acquire publication and
CPU-retirement contracts to the existing SFA typed GX ring. Unlike Melee's
BE8 draw records, these patches contain native little-endian FhGx word runs.
The original SFA setters are compiled against a separate producer state; their
validated results are copied, so no stack matrix/light/texture object is
borrowed by the worker. Existing list/array leases and joins are retained.
Texture pixels keep the existing scope lifetime, not a new asynchronous claim.
No Melee source is newly imported. The dirty-page encoder and generated setter
write footprints are new project code. No new GPU/service/thread mechanism,
launch entitlement or cache-flush policy is introduced. Host differential and
negative-control tests mirror Melee's separation of ownership and threading
checks; physical correctness/performance remains unvalidated. Default off.

2026-10-05 owned GX source transactions: consulted the same pinned Melee's
port/3ds/source_snapshot_cache.h, render_snapshot.h and render_worker.c.
Adapt its separation of original source identity, immutable copied payload,
producer-only metadata and CPU-reader retirement. New SFA code uses one fixed
256 KiB normal-memory arena, 128 source slots and eight in-flight draw leases;
it does not import Melee's malloc-per-entry allocator or assume Melee's source
write tracking. Every source acquisition compares bytes. Changed versions may
coexist while pinned, and exhaustion declines without overwriting live inputs.
Source data remains CPU-only; GPU submission retains the renderer's existing
linear buffers and GPU fences. Deferred EFB ranges are excluded before source
capture, using an exclusion snapshot taken while the renderer is joined.

The first integrated path remaps copied display lists, indexed arrays, texture
pixels and palettes only during worker translation, then restores original GX
source addresses. Reused arena ranges explicitly invalidate the renderer's
texture cache. Existing model joins remain until cross-model GX state, resource
identity and barrier ownership are implemented. No new service, thread, shader,
GPU copy path or console upload. This is disabled by default and carries no
physical performance claim. All new ownership code is original project code.

2026-10-05 source identity follow-up: the same pinned Melee source_snapshot_cache.h,
render_snapshot.h and render_worker.c distinguish original source identities
from copied readable payloads. Apply that contract to SFA cache keys through a
renderer-owned temporary source view. CPU verification and texture conversion
continue to read copies. Version changes invalidate original ranges in draw
order; overlapping versions and intervening borrowed draws cannot reuse stale
observations. This differs from Melee's write-tracked lifetime model: SFA still
compares acquired bytes and retains model joins. No SDK, GPU fence, service,
thread or shader changes. Local integration regresses overall emulator timing;
it remains disabled and provides no physical performance claim.

2026-10-05 model-owned geometry snapshots: reconsulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/source_snapshot_cache.h,
render_snapshot.h, render_worker.c and tests/source_snapshot_cache_tests.c.
Adapt immutable snapshot sharing and independent reader retirement to SFA's
audited model scopes. A producer model lease acquires each geometry source
once; child draw leases pin that exact snapshot, without rereading originals.
Mutable texture bytes still receive fresh acquisition checks. CPU barriers
release the model owner, while child pins independently protect queued data.
The checked scope mode rejects a changed original source before reusing it.
Unlike Melee's persistent write-tracked cache, sharing ends at each SFA CPU
barrier; complete resource-write tracking has not been established.

The fixed256KiB arena now searches bounded gaps between pinned allocations;
it never moves live copies or evicts a reader. This handles model lifetimes
longer than individual draw lifetimes without increasing the allocation.
The renderer's existing conservative TEV/indirect texture dependency set is
shared by ownership, CPU materialization and private draw capture. Every
declared stage remains a dependency; no color-channel dead-code assumption.
Unused binding metadata remains in GX state but its pixel bytes are not read.
Generated layered lookup images remain renderer-owned. Deferred GPU sources
still decline whenever actually referenced. No Melee source was imported;
no GPU transfer, shader, worker affinity, SDK service or launch change.

2026-10-05 ordered texture resource versions: reconsulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e render_worker.c's texture dirty,
invalidate and frame-visibility handoffs; source_snapshot_cache.h and its
write-overlap/pinned-generation tests; texture_visibility.h. Reusable source
references now end at CPU barriers or explicit ordered writes. Partial writes
drop producer references while queued readers retain their immutable versions.
Matrix metadata packets also retire aliasing source references before publishing
the write. Checked mode compares source bytes on resource reuse and rejects an
unreported write. GPU-only sources still decline before snapshot acquisition.

SFA adaptation is deliberately scoped to existing model joins: gx_texture.c
fences non-matrix cache notifications, generated GXInvalidateTexAll/copy calls
are barriers, and arbitrary post-render callbacks end the geometry scope.
Warp-noise allocation already fences before initialization and DCFlushRange
publishes its bytes. Model-loop admission excludes arbitrary callbacks. Unlike
Melee, this does not retain source ownership across frame visibility events or
claim complete game-wide write tracking. Fallbacks continue comparing bytes.
One additional32-source owner lease uses normal BSS; the256KiB arena is unchanged.

Retained geometry validation recognizes immutable owned versions independently
of the producer's current model scope. Replacing a retained source explicitly
clears its owned-version observation. This separates one consumer cache lifetime;
it does not make all renderer state safe for cross-model asynchronous submission.
No new service/thread, GPU resource ownership, shader or hardware path.

2026-10-05 ordered reader scopes: pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/render_worker.c (ordered
JOB_BEGIN/JOB_END, borrowed-source retirement), render_snapshot.h, and
tests/render_worker_tests.c (delayed readers, backpressure and borrowed gates)
are the reference. SFA's GX translator now receives scope range descriptors
and ARM floating-point control in begin packets, followed by ordered end
packets. Retained geometry reader verification uses its own scope storage;
producer admission no longer shares that mutable metadata with the reader.
This differs from Melee's already-lowered draw snapshots: SFA still queues GX
translation and retains the model-end CPU join. These packets do not extend
the lifetime of borrowed bytes or establish game-wide write tracking. The
change adds 44 queue bytes per dispatched scope (40 begin, 4 end), reuses the
fixed ring, and adds four producer range descriptors in normal BSS. No SDK,
GPU cache/transfer, core affinity, shader, or GPU allocation change.

2026-10-05 owned scope completion experiment: reconsulted the same pinned
Melee render_worker.c publication/acknowledgement and retire_geometry paths,
with render_worker_tests.c's delayed borrowed readers and backpressure gates.
Owned mode2 may return from scope end with copied inputs still queued; producer
parent references retire separately from child reader pins. Any outstanding
borrowed draw forces the original join. Unlike Melee's frame-wide lowered draw
queue, SFA still joins before starting another scope and before synchronous
GX, VI, presentation or completion access. All65 generated direct setters now
check that deferred join; caller-owned light/texture descriptor helpers remain
independent. A delayed-reader test found the initially missing direct-setter
join before any console deployment. Added normal BSS is32bytes; queue, source
arena, GPU resources, SDK services, shader and core affinity are unchanged.
Real menu admission was zero deferred scopes, so this remains disabled and
does not establish a performance benefit or a cross-model rendering pipeline.

2026-10-05 model-file resource directory: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/source_snapshot_cache.h,
render_snapshot.h, tests/render_snapshot_tests.c and citro3d_fix.c. Adapted
the distinction between resource identity, content freshness and reader lifetime.
Foxhollow now optionally registers contained geometry ranges after model-file
relocation. The directory assigns generations, consumes existing ordered write
notifications and permanently revokes a file when an unaudited mutable pointer
or callback escapes. Actual heap release revokes the owner before address reuse.
The two allowlisted fur callbacks only read geometry; other callbacks remain
outside this experiment. Per-instance deformation buffers are not registered.

Departure: this is a bounded 128-owner/1024-range directory over SFA's native
little-endian allocations, not Melee's BE8 resolver or copied snapshot allocator.
Existing renderer snapshots remain the comparison oracle. Checked mode compares
every generation hit and fails on an unreported write. Default mode is OFF;
the whole-game alias audit is incomplete. Producer metadata updates join CPU
readers; existing model/scope joins remain. Post-write notifications grant no
permission to queue borrowed sources past a game mutation. No asynchronous
lease, GPU ownership, SDK service, shader or core-affinity change is introduced.
No Melee code imported. Native directory storage is bounded at compile time;
runtime memory accounting also includes added observations in existing caches.

2026-10-05 packed-texture dependency correction: reviewed the pinned Melee
texture_visibility.h and tests/texture_visibility_tests.c, especially secondary
palette aliases and overlap arithmetic near the address-space boundary. SFA's
mode3 packed fur texture samples a second image through fh_texture_pixel, which
can read a palette. Its previous invalidation path checked secondary texels but
omitted that palette. Both inputs now use the same complete texel/palette range
test; range comparisons use subtraction instead of potentially wrapping ends.
Native fur geometry identity uses that helper as well, retaining GPU allocation
lifetime until the existing fence. This is a dependency correction, with no
new cache, thread, GPU command or presumed performance benefit. Helper code is
original SFA adaptation; no additional Melee source was imported.

2026-10-05 resident native draw programs: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e native-3ds/port/3ds/citro3d_fix.c,
render_snapshot.h and tests/render_snapshot_tests.c, and the pinned Citro3D
effect/texture/environment implementation. Adapted descriptor-content binding
checks and explicit ownership of prepared state. SFA's optional native program
keeps resolved ordinary texture-cache slots, owned sampler descriptors and
lowered raster/combiner fields. It verifies current residency, complete source
descriptors and GPU storage descriptors before reuse. Lazy framebuffer aliases,
movie/CPU-fragment materials, points and owned-source snapshots are excluded.

Departure: this is a synchronous renderer-owner program for already retained
GPU geometry, not a queued draw snapshot or raw GPU command replay. Live
transforms, lighting, texgen, affine uniforms and fog still update each draw.
Only raster fields written by the original lowering are restored; stencil,
depth mapping, blend colour and unrelated SDK state are preserved. Bindings use
stable bound_gpu storage and normal SDK dirty flags. No change to retirement,
fences, threads, services, shaders or game work. The 64-entry normal-heap pool
is bounded at 160 KiB, checks the existing 15% reserve before allocation, and
is off by default. No Melee implementation code imported. Checked mode compares
against fresh lowering; fast mode requires differential image coverage.

Outcome: native draw-program experiment removed after the matched comparison
showed no overall benefit. Its sources and test fixture are preserved under
artifacts/research/rejected-native-program/, with evidence in
artifacts/native-program/. The adaptation description above is historical;
no native-program pool, bindings, configuration or counters remain active.

2026-10-05 texture-backed EFB experiment: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e native-3ds/port/3ds/renderer.c
gpu_efb_copy, efb_copy_cpu_reference and tests/efb_rgb565_tests.c, together with
pinned Citro3D source/renderqueue.c, framebuffer.c and base.c. Melee samples a
padded copy of its framebuffer. SFA now optionally renders the same 264x400
viewport into a 512x512 RGBA8 texture-backed target, allowing color and shadow
copies to sample that target directly. The SDK owns attachment changes,
framebuffer flush/invalidate and texture-cache invalidation. No raw PICA
register sequence, shader or thread change is introduced.

Departure: larger VRAM backing replaces the GPU-sampled linear capture. The
depth snapshot also becomes512x512 and retains its existing fenced GSP DMA
at copy time; its RGBA8 view can be sampled without an intermediate capture.
This preserves immutable copy-time depth and the mandatory Z8 admission check.
Depth/color CPU reads remain equal-dimension, flags-zero DisplayTransfer into
aligned linear storage. After completion and cache invalidation, CPU rows are
compacted to the existing264x400 conversion layout. Those newly dirty CPU
lines are flushed before every later GPU write to the shared readback buffer,
including screen captures. The former raw-depth-readback rejection remains.

Presentation keeps its validated VRAM-to-VRAM input-gap copy, with the larger
source stride/capacity. Color texture ownership belongs to the main target's
separate C3D_Tex; reflection descriptors only borrow it and never free it.
Default is OFF (texture_efb.txt / --texture-efb). Physical RTT coherence,
larger depth-snapshot DMA, memory peaks and performance remain unvalidated.
No Melee code imported. This changes framebuffer storage/transfer architecture,
not game resolution, draw order, depth tests, effects or simulation work.

Integration exposed a raster/storage origin mismatch: increasing backing
height from400 to512 places the unchanged viewport112 storage rows later.
Presentation cropping, CPU compaction and color/depth/shadow UVs now share
that offset. Independent known-depth and presentation-coordinate fixtures
cover the defect; differential color/depth comparisons alone shared the bug.

Outcome: the texture-backed EFB experiment is removed from active production
source, configuration and runners after failing to establish substantial
overall benefit. Exact corrected source/binary overlay and manifest are in
artifacts/research/rejected-texture-efb/. The independent presentation-origin
test remains active. The preceding EFB adaptation entries are historical,
not a description of the current framebuffer allocation or transfer path.

2026-10-05 dynamic model ownership: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
native-3ds/port/3ds/render_snapshot.h and source_snapshot_cache.h, plus
tests/source_dirty_tests.c. Adopt the requirement that CPU readers retire
before source mutation/release, and that incomplete mutation history cannot
authorize reuse. The source_dirty test's included experiments/source_dirty.h
is absent in this checkout; no inspection or execution of that header is claimed.

Foxhollow departure: model instances are embedded inside game-object heap
allocations. Generated adapters declare their actual vertex/normal spans,
notify before morph/vertex/normal deformation, and revoke ownership when
mutable getters or unaudited callbacks expose the file or instance. Re-layout
cannot restore trust in an escaped overlapping region. heapFree retires all
owners overlapping the containing allocation before allocator coalescing.
Normal/vertex chunk notifications preserve the pinned memcpyToCache convention
that an encoded zero word count copies 4096 bytes. File aliases remain owned
by the model file rather than by an unrelated instance allocation.

This is a synchronous source-version contract, OFF by default. It does not
permit deferred raw game pointers, transfer GPU ownership, or prove all
untyped aliases have been audited. No worker, GPU/cache protocol, service,
shader or extra pool is introduced. The existing bounded directory remains
15360 bytes on ARM. No Melee implementation code is imported. Checked menu
evidence and its final-source caveat are recorded in IMPLEMENTATION.md.

2026-10-05 queue-ordered GPU copies: revisited pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e tools/async_renderqueue.py,
tests/async_queue.c and the previously adapted renderer GPU-copy ownership.
Also inspected pinned Citro3D renderqueue.c and base.c::C3Di_SplitFrame:
splitting a used draw emits framebuffer flush/invalidate before appending the
GPU command list. libctru's bound command queue orders later DMA/PPF commands
and GPU consumers. Existing callback-publication protection and joined queue
recycling are unchanged.

Departure from the previous SFA crash-isolation policy: opt-in
queued_gpu_copies.txt=1 removes CPU waits immediately before/after GPU-only
copies. The same producer, transfer and consumer remain ordered in one queue.
Before splitting, capacity pressure still gets a bounded completion wait;
the existing SDK adapter then joins callbacks before recycling queue entries.
CPU materialization, vertex-buffer reuse, GPU allocation retirement and next
frame startup retain their original completion joins. Sources/destinations,
the 50 padded DMA rows, depth-snapshot DMA, presentation PPF transfer, cache
visibility operations and raw-depth rejection are unchanged. No new memory
pool, clock setting, shader, pixel format or game-work shortcut is introduced.

Default remains the previously fenced path, including console builds. The
new mode needs hardware validation; emulator queue ordering does not prove
hardware transfer/coherence behavior or timing. Host tests exercise the actual
copy function under delayed/stalled completion and storage reuse, separately
from the original isolation-mode regressions. Device fixtures cover color,
depth, independent presentation coordinates and shadow retirement. No new
Melee code is imported by this change.


2026-10-05 integrated console preparation candidate: revisited pinned Melee
port/3ds/render_worker.c (immutable command ownership, CPU completion at End,
independent GPU lifetimes) and render_snapshot.h. Promote the previously
checked SFA producer-state deltas/matrix blocks/material and geometry paths
as one console candidate, retaining the existing end-of-scope reader join.
Melee GX/native geometry separation informed the existing constituent changes;
no new imported code. SFA-specific state patches and synchronous model-array
scopes differ from Melee snapshots. No speculative deferred model alias lease,
new worker/core, clock, service, transfer, shader or GPU allocation is enabled.
The same-binary legacy override is for controlled comparison, not a workload
shortcut. Emulator checks do not establish physical performance or correctness.


2026-10-05 empty lease retirement regression: inspected pinned Melee
render_worker.c::wait_until/retire_geometry and render_snapshot.h. Melee avoids
already-acknowledged/inactive bookkeeping while preserving live CPU-reader
retirement. SFA's source snapshot extension was unnecessarily clearing a
128-entry scratch table and walking/writing all128 pool pin counters when
releasing an empty lease, twice at every GX barrier even with that experiment
OFF. Return success after argument validation when count is zero, in release
and forget. Active leases retain the exact validation, pin updates and fences.
No GPU/source lifetime, renderer work, shader, worker, pool or ABI is changed.
The ASan fixture poisons the directory during empty release/forget with a live
unrelated lease present; deleting the early release check reproduces an access
and is rejected. Existing delayed-source and invalid-lease cases still apply.

The same audit found idle GX barriers unnecessarily publishing and waking the
shared worker. Adapt Melee render_worker.c::wait_until's acknowledged-prefix
rule: after flushing any state, return only outside open/deferred scopes,
without an allocated copied-source pool, and after an acquire read confirms
read==write. Every pending prefix and active source retirement keeps the
original publication/join path. Tests assert8000 idle barriers do not signal,
while delayed-reader, missing-join and early-release controls remain effective.
This changes CPU bookkeeping only; it does not skip a pending GPU/CPU fence.

2026-10-05 sampled production component clocks (c83dfe72923b): pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e port/3ds/renderer.c native_detail_begin/end
makes detailed clocks no-ops in production and samples smoke profiling1/16.
Adaptation: sample30 hot component-clock sites on complete renderer frames,
using the existing attribution period (default16), and emit only complete
sampled component rows. Unlike Melee, retain periodic console attribution;
FPS/workload and operational audio/watchdog/SD clocks remain live. Emulator-test
builds retain full component measurements. No graphics effects, draw ordering,
buffer lifetime, shaders, memory pools or game work changed. Exact-console
headless evidence and limits: artifacts/native/console-component-sampling.

2026-10-05 ARM shadow scratch alignment: pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e,
port/engine/byte_memory.h, tests/memory_tests.c, port/3ds/bottom.c,
raster_state_verify.h and main.c preserve or prove alignment before typed
access. Our generated ARM adaptation preserves the six existing ALIGN_DECL
annotations erased by Foxhollow's desktop global.h (shadow, object matrices,
distortion). No Melee source imported. Five compile-time and six linked-symbol
checks include the previous grid-origin fix. Strict linked-instruction probe
reproduces physical dump185 and verifies corrected loads. No shadow effects,
gameplay, timing, buffer lifetime or worker behavior changed. Physical fix
validation remains pending; see docs/IMPLEMENTATION.md and shadow-alignment.

2026-10-06 depth admission after flat intros: revisited pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c::efb_copy and
efb_discard_verify.h. Melee can omit depth payloads for its coverage overlays;
SFA consumes them, so preserve the snapshot, exact materialization and fences.
Previously eight flat, byte-matching probes permanently disabled GPU Z8 sampling.
Now flatness remains inconclusive; existing CPU materialization supplies only a
hint to retry on varied content. Full sampled RGBA equality and two-axis depth
variation still gate admission; a mismatch permanently selects CPU fallback.
No new transfer type, raw readback, clock, pool, or thread is introduced.
The device fixture now starts with twelve flat frames before varied depth.

Seam-aware offline mesh reduction uses the same pinned Melee geometry_dirty.h
and renderer ownership reference as the prior model pack. This remains an
SFA-specific host adaptation using MIT meshoptimizer, with original material
boundaries, joint boundary vertices, collision and animation data retained.

2026-10-06 experimental sampleable EFB: pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
renderer.c gpu_efb_copy and efb_copy retain sampled copies and explicit GPU/CPU
ownership. Reviewed Citro3D texture.c (binding and deletion), renderqueue.c
(CreateFromTex ownership, queue splits, target lifetime), and framebuffer.c.
SFA departure: allocate a512x512 main texture with a512x400 framebuffer extent
and264x400 viewport. A borrowed view samples this storage after producer
completion; sampling must finish before source overwrite. This removes the
intermediate padded capture copy and its linear allocation. Depth still needs
an immutable VRAM snapshot; no Melee coverage-only depth discard is imported.
Safe DisplayTransfer readback and exact admission remain mandatory. Alternate
layout1 retains a single contiguous DMA to owned linear sampled storage.
The former raw depth/capture readback and unfenced routes remain rejected.
Both layouts are optional local experiments; console performance unestablished.

Optional reduced fur surface uses the same reviewed Melee renderer.c material
combiner/state ownership reference. The expression and original texture
sampling stay on PICA; the SFA-specific visual departure removes indirect
noise-coordinate perturbation only for the audited fur signature. It retains
shells, dynamic lighting, mask alpha, blend/depth behavior and game callbacks.
This follows the user's explicit reduced-detail authorization and is not an
exact GX emulation claim. All other materials keep their established path.

Prepared compressed texture companion (6 October 2026): reviewed Melee pin
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c texture_decode_into,
allocation retry/eviction and C3D_TexFlush publication, plus imported
texture_decode.h/texture_storage.h. The host preparer reuses its exact GameCube
CMPR decoder. Departure: eligible CMPR GPU uploads may use SDK tex3ds ETC1/A4
instead of RGBA8, as an explicitly authorized RGB-detail reduction. Alpha must
match exactly; original dimensions/source assets and CPU sampling remain.
The existing Azahar-derived ETC1 decoder and its license notice remain in place;
the host A4 bridge unpacks the documented adjacent alpha nibbles. Melee's GPU
ownership/flush and retirement-fence rules are retained. The new reader owns
only a bounded CPU directory and 4KiB read staging, verifies full source bytes
and GPU payload CRC, and falls back transactionally on corruption. No BE8
bridge, game-specific Melee material semantics or raw VRAM readback is imported.

Reduced lighting and reflection experiments (6 October 2026): reviewed the
same Melee pin's renderer.c, lighting_uniforms.h and point geometry shader,
including uniform limits, alpha/depth restoration and ordered submissions.
SFA-specific departures: optional two spatial lights with diffuse directional
fill; optional five-combiner reflection using one sample squared per reflection
layer, retaining base alpha and both tint colors. These are authorized visual
approximations, not exact GX emulation. Existing fences/storage remain intact.
Particle width now uses the full-width logical640 coordinate system; textured
width255 keeps original CPU expansion because GPU sampling differed at edges.
# Six-stage layered tint and captured scalar oracle (2026-10-06)

Reviewed pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
`native-3ds/port/3ds/renderer.c` for combiner operands, alpha/write-mask state
and native render/EFB coordinate scaling. SFA's five-stage base plus constant
tint is factored into five PICA combiners using the existing three texture
samplers; this is a game-specific expression, not imported Melee gameplay.
The captured comparison's emulator-only 400-wide scalar grid distinguishes
material arithmetic from the existing 320-wide CPU fallback resampling.
No hardware allocation, synchronization or readback mechanism changed.

The same layered families now retain their variable GX alpha constant on GPU.
Five-stage bases use the existing four combiners; six-stage bases keep five
combiners with half-range intermediate products to avoid early saturation.
Optional terminal tint consumes one more combiner. The new six-stage fades
have a separately derived six-level channel bound (observed maximum4/255),
including quantization of half opacity and its product before doubling. Earlier
opaque and five-stage cases retain the three-level limit. No effect is omitted.

Reviewed the pinned renderer's EFB half-size coordinate expressions, which
also use tiny boundary epsilons, and local Azahar `src/video_core/pica_types.h`:
f24 has16 fraction/7 exponent bits, but FromFloat32 retains an ordinary float.
SFA's aligned half-size copy now samples reversed PAL row centers exactly,
departing from Melee's generic epsilon expression to avoid rounding onto the
wrong row. A host regression models nearest/truncating endpoint/interpolant
quantization and catches the former expression under nearest rounding.
This is not a complete hardware model. Device admission remains byte-exact;
whether this fixes physical rejection requires console evidence. Existing
copy fences, storage bounds and raw-depth-readback rejection are unchanged.

2026-10-06: reviewed the same pinned Melee renderer's EFB DisplayTransfer,
completion, invalidation, scalar converter and efb_rgb565.h again. Foxhollow
now preserves its512-word physical readback pitch instead of compacting399
rows and flushing the CPU-written result after every acquisition. Conversion
and depth-point helpers accept the pitch; packed APIs remain wrappers. The
264-row PAL source and512-word storage differ from Melee's packed240-row view.
GPU ownership/fences and post-transfer invalidation remain; initial allocation
invalidation precedes GPU ownership. CPU converters never write source storage.
Tile-local gathers limit competing source cache lines with the padded pitch;
this ARM11 cache consideration is not an emulator performance guarantee.
No Melee gameplay code, BE8 assumptions or extra GPU allocation introduced.

2026-10-06: reviewed pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/renderer.c reserve_commands/reserve_dynamic and
tests/layered_material_tests.c. Preserve bounded streaming and complete queue
ownership, and compare material translations against scalar evaluation.
Foxhollow-specific departure: the recognized chained projective material uses
the user-authorized native-screen visual detail reduction, bounding coordinate
error at1/8 rather than1/32 texture texel. No Melee analogue is copied for
this SFA material. Color/alpha operations and fallback admission remain.
Append multiple accepted triangles to the existing384-vertex scratch area;
overflow cannot overwrite an accepted prefix. Flush before scalar fallback
to retain transparent draw order. No additional heap/linear/VRAM buffer.

2026-10-06: re-read the pinned Melee renderer's bounded streaming/queue rules
and geometry_dirty.h. Existing SFA single-pass reflection LOD now has an actual
Palace floor workload and explicit package/configuration support. This remains
an SFA visual approximation (two squared samples replace two sample products),
not a Melee import or an equivalence claim. GPU ownership, depth/color/alpha
masks, allocation limits and live tint inputs are retained. Bounded emulator
capture discovery retains successively doubled-cost draws in separate files;
no console capture code, allocation or synchronization change is introduced.

## Geometry key storage, 2026-10-06

Reviewed pinned melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e,
native-3ds/port/3ds/renderer.c (NativeGeometry directory, reserve_commands,
reserve_dynamic), port/engine/geometry_dirty.h and tools/test_geometry_sources.py.
Retain bounded GPU storage and completion before recycling, with current GX
source validation unchanged. SFA's variable material keys now occupy the first
aligned bytes of their existing linear geometry allocation, followed by vertex
and index data. This departs from Melee's fixed CPU metadata directory: it avoids
a separate small ordinary-heap allocation disabling otherwise admissible native
geometry when ordinary RAM is near its reserve. Key bytes and padding count
against the same geometry budget and linear reserve; allocation failure still
declines caching, and GPU fences still protect the entire allocation. CPU-only
keys are included in the existing flush; no shader, draw order or source-write
assumption changes. Telemetry distinguishes zero normal key bytes from linear
key bytes already included in total linear cache occupancy.

## Experimental indirect lighting mesh, 2026-10-06

Revisited pinned melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/renderer.c native_submit, texture pinning and reserve_dynamic,
port/engine/geometry_dirty.h and tools/test_geometry_sources.py. Reuse this
port's existing ordered render_direct submission, texture ownership, stream
flushes and fences. No new GPU storage, service, core or synchronization rule.
The new indirect_lighting.c is SFA-specific original code, not imported Melee
logic. It recognizes the audited four-stage dynamic S/T lighting family and
samples its dependent coordinates on a bounded mesh. PICA retains the base
texture, original opacity coordinates, TEV color arithmetic and raster state.
Lighting interpolation is a deliberate reduced-detail approximation, disabled
by default until image and integrated checks establish acceptable behavior.
Default detail samples the original vertices without subdivision. A diagnostic
detail setting can split at projected 16-pixel edges, at most four levels/256
children; the first close-up rejected that high workload. Scratch submission
holds 63 vertices (9324 bytes); no retained heap
or linear allocation. Actual integrated stack/CPU/GPU load still needs checking.
Unsupported state/invalid projective denominators retain the scalar path.

## Generalized indirect coordinate lowering, 2026-10-06

Re-read the same pinned Melee renderer.c native_submit layered TEV setup,
stable per-slot texture bindings, depth/blend state, and the independent scalar
oracles in tests/layered_material_tests.c and material_program_tests.c. Preserve
those ownership/raster patterns through the existing render_direct path.
No Melee material recognizer is imported: SFA has a different indirect-stage
coordinate chain. The experimental indirect_lighting path now resolves constant
matrix and dynamic S/T offsets, disabled-map SU scaling and add-previous chains
per vertex before compiling the unchanged color/alpha TEV program for PICA.
Direct-sampled UVs are reserved and retained. Newly resolved coordinates occupy
unused slots; lack of slots or a program exceeding PICA limits rejects the plan.
Wrapping within indirect offsets and indirect bump-alpha channels also reject
the plan rather than interpolating across a wrap seam or losing bump alpha.

This is a reduced-detail texture-coordinate approximation, not equivalent
per-pixel GX indirect sampling. It remains disabled by default. Dependent
opacity coordinates can now be approximated too, so live transparency/coverage
inspection is required before enabling it. Direct opacity maps are unchanged.
The per-draw prepared coordinate program occupies sizeof(FhIndirectLighting)
bytes of stack and is reused across all triangles. No persistent allocation,
new GPU buffer, service, core or synchronization mechanism is introduced.

The audited chained-effect family additionally uses its existing two-stage
fh_chained_pipeline program because the general compiler cannot retain that
family's earlier raster-alpha register. Its color sampler selects the newly
resolved stage1 coordinate; its independent alpha sampler keeps coordinate2.
The experimental mode omits adaptive coordinate subdivision, retaining original
geometry and sampling the combined coordinate at vertices. This deliberately
reduces effect detail and requires image validation; it does not establish a
whole-frame gain. Original accurate chained rendering remains the default.

## Borrowed EFB queue ordering, 2026-10-06

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c
gpu_efb_copy (slot lifetime, render target switches, capture sampling and
efb_verify comparisons), the previously adapted early queue implementation,
and pinned Citro3D renderqueue.c/base.c. Melee copies EFB rows to a separate
capture; this port already has persistent sampleable color/snapshot buffers.
The optional ordered-view mode retains GPU queue producer/consumer ordering
but removes CPU completion waits at those two boundaries. This departs from
our prior hardware-isolation waits, not from required GPU barriers: FrameSplit
flushes/invalidates the framebuffer and texture binding clears texture cache.
The source cannot be freed or CPU-written while sampled; subsequent GPU
overwrites stay later in the same queue. CPU materialization and buffer reuse
retain existing joined lifetimes. Capacity drains remain bounded. No raw
TextureCopy depth-readback path is restored. No new allocation or imported
code. Exact device fixtures and delayed host queue tests pass; physical
cache/coherency and timing remain unvalidated for this opt-in mode.

## Whole-model asset clustering experiment, 2026-10-06

Consulted Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/engine/geometry_dirty.h, port/3ds/renderer.c, lighting_uniforms.h and
render_worker.c. Preserve the existing source-ownership boundary: host-generated
render payloads are applied before model relocation/publication through the
existing validated loader. No live GPU buffer is rewritten, and no queue,
cache-visibility, worker, OS, or allocation rule changes. No Melee code imported.

The new host-only model_cluster.py is an SFA-specific lossy geometry policy,
not a Melee optimization. It shares original position representatives across
material lists with compatible resolved joint bindings. Same-numbered GX slots
are insufficient because opcode4 can rebind them to different bones. Baked map
visibility boxes remain valid for newly introduced indices. Collision, animation,
bone and attribute arrays remain byte-identical; render position indices need
not. These invariants do not prove animated visual quality. Meshoptimizer retains
its existing pin/license. The evaluated pack is rejected for deployment because
whole-frame benefit was small and character geometry visibly degraded.

## Unlit indirect surface geometry residence, 2026-10-06

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/renderer.c (texgen_compute, texture_matrix_uv), lighting_uniforms.h
and port/engine/geometry_dirty.h. Normal-derived coordinates are independent
of channel lighting; live matrices need not invalidate owned raw attributes.
Extend SFA's existing zero-light normal-palette PICA path from the ordinary
two-coordinate material to recognized five-coordinate indirect surfaces.
The latter previously baked normal-derived UVs into every vertex when lighting
was disabled, defeating whole-mesh residence. No Melee code imported.

The SFA-specific surface still uses its existing combiner, indirect samples,
shells and moving post matrices. Existing palette-equality, material support,
normal bounds and texgen dependency proofs remain required. Original source
comparisons, allocation limits, GPU buffer lifetimes, queue fences and cache
flushes are unchanged. No new shader, arena, worker or OS service. ARM regression
compares resolved output against scalar evaluation during palette/source/color
changes and rejects a mismatched texture palette. Headless scene comparison
is required before any physical performance claim.

The following material-input lowering also follows that separation. Renderer
plans identify actually consumed coordinates; the GX frontend preserves their
backward dependencies and leaves special CPU/sphere/movie paths conservative.
Unused generators no longer execute or enter resident geometry dependencies.
Masks are part of both geometry identity formats; source validation and existing
fixed allocation budgets remain. The reduced indirect-surface policy is still
SFA-specific, lossy and opt-in. No shader resources or OS behavior changes.

### 6 October: experimental native model compiler and weighted skinning

Consulted Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e:
`port/3ds/renderer.c` (native indexed geometry, stream flushing, attribute setup
and shader-uniform ownership) and `port/engine/geometry_dirty.h` (source lifetime
and mutation visibility). No additional Melee code imported. The compiler and
bridge are SFA-specific original code. Existing renderer integration is pending;
the helpers do not add an arena, alter OS behavior or bypass a GPU fence.

The departure is a 24-base-bone, two-weight shader and grouped model references,
instead of translating repeated ten-blended-matrix GX sections. SFA's
`objprint_dolphin.c:modelCalcVtxGroupMtxs` defines the bind-tail adjustment and
quarter weights. Original animation remains responsible for poses. Prepared
topology keeps source identity, material/fallback order and original triangles.
Mutable vertex/normal data must remain live or generation-validated at runtime.
The shader has 136 instructions and uses all 96 float-vector registers; palette
indices are bounded to rows0..71. This is an assembled capacity check, not a
hardware execution or performance result. Runtime material admission and
buffer/state ownership are still required before this path may draw game work.

### Native model runtime integration, 6 October

The preceding compiler-only record is superseded by integrated bounded loading,
live model-loop admission and indexed PICA submission. Reconsulted the same
Melee pin's renderer.c and geometry_dirty.h: append-only GPU stream ownership,
flush/fence before reuse, complete attribute/program rebinding, and exact byte
comparison when mutation history is missing. SFA's resource generations are
coarse and escaped sources always compare topology; live source pointer/size
identity is checked too. No additional Melee code imported.

SFA's two-bone skinning shader now uses22 bones and an affine RGBA transform:
142 instructions,95/96 float vectors,8/16 booleans. The companion directory and
payloads have a256KiB normal-heap cap and preserve the existing15% reserve.
GPU vertices/indices share existing bounded flushed streams. Original callbacks,
animation and state side effects execute; unsupported draws stay ordered on the
original path. Runtime admission remains synchronous; it is not a new worker
lease or an excuse to bypass ownership. Azahar renders admitted groups, but the
matched character-only integration shows no useful whole-frame improvement.
This path is experimental and uninstalled; no physical performance claim.

### Scene atlas and persistent geometry, 6 October

Reconsulted Melee at c2ebe7c769b3287f2b3748173e8fe5ec24ef408e:
`port/3ds/renderer.c` for persistent indexed GPU storage, cache visibility and
fences, and `texture_repack.h`, `texture_storage.h`, `texture_decode.h` for
RGB5658x8 Morton ordering. The focused host test calls the pinned repacker on
generated GX RGB565 fixtures as an independent storage oracle; no new Melee
code was copied. Inspected pinned Citro3D `source/buffers.c` for physical-address
binding and the existing prepared-texture loader's CPU staging before GPU writes.

Departures: SFA-specific source/material UV charts deliberately reduce diffuse
texel density; cross-material scene groups retain per-list visibility metadata;
FHNSCN01 separates normal metadata from persistent GPU geometry. The renderer
now accepts optional borrowed persistent vertices/indices, requiring caller-owned
flushes, validated mutations and completion before reuse/free. No runtime loader
or game-side admission supplies those pointers yet. Dynamic material modulation
uses one additional shader row:147 instructions,96/96 float rows,10/16 booleans.
The existing copied-buffer path has a short Azahar regression; persistent scene
rendering and physical behavior remain unvalidated. No new OS/thread behavior,
memory entitlement or relaxation of fences is introduced by these components.

### Integrated scene material replacement, 6 October

The preceding loader/admission status is superseded. Both model and world
instruction loops now submit FHNSCN02 assets, with bounded metadata, persistent
GPU geometry, renderer-owned atlas pages and synchronous mutation/release rules.
Reconsulted the same Melee renderer/texture references and render_worker.h;
also checked pinned Citro3D renderqueue.c and buffers.c. No new reference code
was imported. GPU resources are published with explicit flushes; freeing waits
for both CPU consumers and GPU completion. C3D_FrameSync is only a vblank wait,
so it is not used as a resource-retirement fence.

Departures: original SFA material constructors are bypassed only for admitted
opaque groups. World lists retain visibility and dynamic-light fallback; scene
groups split at differing fog/ambient modes. Live morph positions/normals/colors
refresh persistent storage after completion; changed UV domains fall back.
The native scene path constructs pose/color/diffuse-light state directly rather
than running GX display-list shading preparation on prebuilt atlas vertices.
Up to two spatial lights are retained; additional lights contribute the existing
live directional-fill approximation. Specular or unsupported channel contracts
fall back. These are authorized visual reductions, not changed simulation.

The runtime Texture header includes the enlarged TARGET_PC GXTexObj; pixel
addressing uses textureGetImageData/sizeof(Texture), while file bounds continue
to use the serialized 96-byte header. A hardcoded live 96-byte offset initially
rejected every prepared texture and was corrected without relaxing identity.
Runtime integration and appearance have emulator evidence, but the first matched
comparison has no useful whole-frame FPS gain and no console validation.

### Persistent native sky groups, 6 October

Reconsulted the same pinned Melee renderer and its point geometry shader.
The new SFA vertex shader is original; it links the existing adapted
`game_point.g.pica`, retaining that file's Melee attribution and GPL notice.
The geometry shader keeps its homogeneous clipping/edge guards. No new Melee
source was copied. The SFA-specific vertex shader selects one of76 live group
colors and applies the original camera rotation/projection. Original3800 points,
point size, RNG calls and draw order are preserved; the remaining16 textured
groups use their original path. This replaces76 submissions with one.

Persistent vertices use the existing linear allocator, explicit cache publication
and CPU/GPU completion before rewriting/freeing. Changed source lists trigger a
rebuild; unsupported contracts fall back before admission. The buffers count
against scene normal/linear caps and reserves. The geometry shader is shared
with the ordinary point implementation, not a new resource-lifetime mechanism.
Native sky is currently gated by the experimental scene package. Emulator
integration executes, but physical correctness/performance is unvalidated.

### Renderer removal and PICA reconstruction, 6 October

Reconsulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/renderer.c and render_worker.h, plus pinned Citro3D renderqueue.c.
The rendering-off diagnostic removes decode/submission and recorded GPU setters;
copy extents, scissor queries, FIFO service callbacks, display-list recording,
original animation and arbitrary game callbacks remain. Model matrix commands
advance the original bit cursor without constructing view/normal palettes.
This deliberately changes visual query results and is not gameplay acceptance.

The original platform_minimal_graphics.c and minimal.v.pica start a separate
PICA reconstruction path: 400x240 target, 256KiB command buffer, solid resident
geometry, original animated joint weights. No Melee code was copied here.
Adapted ownership rules retain explicit CPU cache publication and completed GPU
queues before rewriting/freeing geometry. FrameBegin NONBLOCK is polled with a
three-second bound; vblank is never used as a storage-release fence. The original
CPU metadata/linear geometry package caps remain. No desktop allocation sizes
were transplanted. It shares the identity-checked asset loader and its original
GPL provenance, but has no generic material compiler, texture cache, EFB copies
or legacy rendering fallback. Missing geometry and effects are diagnostic gaps.

The first integration caught duplicate present calls clearing the real image;
the new path now submits only an open frame. Console compatibility and physical
performance remain unvalidated. The raw-depth-readback rejection is unchanged.

### Ordered synth/movie audio pipeline, 6 October

Reconsulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/audio.c and render_worker.h. Retain the existing NDSP-owned linear
ring, actual-range cache publication, service fallback, core-2 ownership and
synth lock. The new FIFO in audio.c is original SFA-specific code; Melee has
no equivalent movie DMA callback or MusyX ring to transplant.

Main retains movie counters, queues, volume changes and callbacks. The worker
executes ordered synth, PCM copy, movie mixing and NDSP publication commands.
Two reserved packets count against the existing eight NDSP slots. No borrowed
movie samples survive their callback: they are snapshotted into bounded CPU
storage. Original synth and movie buffer reuse is ordered by the FIFO. Synth
API entry, callback replacement, DMA stop/rate change, suspend and shutdown
drain pending work. Arbitrary callbacks remain on the previous path. The option
is explicitly gated by audio_pipeline.txt; physical scheduling is unvalidated.

The minimal geometry source checks now reuse the existing audited model-resource
write/escape directory. Unknown or escaped sources retain byte comparisons;
checked mode verifies every generation reuse. A stable pointer alone does not
authorize reuse. Additional version metadata remains under the same normal-RAM
cap, and GPU completion still precedes geometry modification or release.

### Snapshot alignment, prepared movie codec and read-ahead, 6 October

Reconsulted the same Melee pin's port/engine/libc.c and its bounded ARM compare
tests. Packed scene snapshots previously misaligned equal source/snapshot pairs,
forcing the byte path. Each private snapshot now reserves three spare bytes and
matches the live source's low address bits. Comparisons, escape handling and
mutation checks remain; no new assembly or unaligned ARM loads were introduced.

The optional movie container version2 adds an original bounded LZ4 block decoder
with decoded CRC32. Host preparation uses installed liblz41.9.4 and records its
version. All1797 ETC1 payloads and original audio packets are byte-identical to
the existing prepared asset after decoding. The CPU buffers and existing PICA
texture cache-publication contract are unchanged. Old builds reject version2.
Raw uncompressed repacking was tested and rejected: its SD cost outweighed CPU
savings. Neither the source RVZ nor any original asset was modified.

Reconsulted port/engine/dvd.c and port/3ds/log_io.c for ordered callback delivery,
sleeping same-core I/O and joined close. platform_movie_prefetch.c is original
code using those ownership rules: one bounded CPU buffer, one native I/O request,
32KiB stack, existing application core(-2), no additional core privilege. Only
the original movie parser requests its known next packet. Game reads join before
sharing the FILE or consuming the private bytes. Decode, movie progression,
callbacks and GPU cache notification stay on main in their original order.
Sleep/suspend joins the request; close joins before releasing the file/buffer.
Three-second read/shutdown bounds panic rather than freeing an in-flight buffer.
Mode2 verifies every hit against a direct read. Native checked integration and
host sanitizer/concurrency tests are required; physical lifecycle and stack
headroom remain unvalidated. No Melee source was newly copied.

### Native base materials and HUD, 6 October

Reconsulted Melee pin c2ebe7c769b3287f2b3748173e8fe5ec24ef408e:
port/3ds/renderer.c, texture_storage.h, texture_visibility.h and the previously
imported texture decode/repack tests; port/engine/libc.c for bounded aligned
comparisons. platform_minimal_graphics.c now has an independent bounded base
texture store (4MiB linear, 2MiB CPU snapshot caps, 4MiB linear reserve and 15%
normal-heap headroom). It reuses the already attributed conversion routines and
prepared CMPR-to-ETC1/ETC1A4 package reader, not the old GX material pipeline.
Every binding compares actual source/palette bytes against private aligned
snapshots, including writes without notifications and recycled addresses.
Broad GX invalidation is a hint to revalidate, not unconditional re-upload.
Initial integration exposed that distinction; the redundant SD uploads were
fixed before any console deployment. GPU data stays alive until a Citro3D
completion fence; invalidation itself does not free queued storage.

The new PICA program adds a UV output to the animated two-joint shader. After
captured-image inspection, its V coordinate follows Melee renderer.c's native
vertex finalizer (1-t*scale); final program21 instructions/73 float registers.
Native TEV handles base color/alpha, intensity-as-alpha and ordinary
cutout/opacity rules. Original shader layers provide live texture objects through
a read-only ABI accessor on the producer. Full material lighting, scrolls,
multiple layers, callbacks and effects are not yet reproduced; missing coverage
must remain explicit. Model refresh decodes an entire validated group directly
into its fenced private buffer instead of revalidating/copying 64-vertex chunks.

HUD quads use a fixed 96KiB linear arena. Consecutive compatible glyphs batch;
flushing, wraparound and retirement honor GPU completion. The game's font atlas,
text layout/reveal, color setters, original projection and scissors are retained.
Title/name-entry text callbacks also retain their game translation and scrolling.
Host sanitizer tests exercise real cache/batch code with a queued-GPU adapter,
including unannounced source/palette mutation, byte/slot pressure, allocation
failure, clipped UVs and 601 glyphs spanning three submissions. This adapter
does not establish physical GPU behavior. No new Melee source was copied.

Foxhollow-specific departure: HUD GX_VTXFMT1 positions have two fractional bits
(main/pi_videoinit.c), so native quads convert quarter-pixel positions/depth before
the original projection. Title callbacks retain their distinct depth translation.
Build verification now requires the linked native HUD entrypoint; an early
generated-unit whitelist had silently excluded the new call sites. Nonzero draw
counters alone did not catch the coordinate error; captured images were required.

Reconstruction continuation: re-read the same pinned renderer.c dynamic-buffer
flush/retirement and native conversion paths. Morph refresh retains the existing
GPU completion fence, but validates and updates only mutated position/normal/
color/UV fields; immutable topology and skin data stay resident. Full constructor
byte comparisons cover all fifteen mutation masks. The new loop uses bounded
byte reads, no wider alignment assumptions or new assembly.

Secondary HUD masks use an independent PICA texture unit and alpha TEV stage,
with the original I4 image and fade semantics from Foxhollow texture.c and
intersect_render.c. Two simultaneous cache bindings are pinned during acquisition
and queued source writes flush batches. Additive HUD uses the original blend
factors. This is new code following Melee's storage lifetime contract.

Foxhollow model-specific adaptation: zero-joint objects use an explicit object
world matrix. CPU-deformed models mix that fixed matrix slot9 with ordinary
skeletal slots; compiler metadata inserts a virtual world joint and remaps blend
bindings without changing the game's deformation work. World/view are applied
once to those vertices. The temporary combined pose array is bounded to16KiB
on the reserved512KiB main stack. Scene caps are now2MiB CPU/6MiB linear and128
records, allocated on demand; each admission still enforces15% normal headroom
and4MiB free linear reserve. Console timing/ownership checks remain outstanding.

6 October, asynchronous I/O and native GPU publication:
Re-read pinned Melee port/3ds/file_io.c and port/engine/dvd.c, plus
port/3ds/audio.c and command_cache.c at c2ebe7c769b3287f2b3748173e8fe5ec24ef408e.
Adapt the sleeping same-core file worker (32KiB stack, priority one above main),
completion publication and callbacks/cache notifications on main. Foxhollow uses
separate read-only FILE cursors for DVD async work, streamed audio and prepared
movie reads; it does not share a seek cursor across producers. Stream read-ahead
owns32KiB of normal RAM, preserving original decode/history/count/volume/callback
ordering and the existing NDSP capacity. Ordinary DVD jobs borrow the caller's
buffer while BUSY and join before cancellation/close/shutdown can release it.
Suspend joins pending work; unavailable workers retain synchronous fallback.
Each worker admission retains15% normal-heap headroom including reserved main
stack; no new linear allocation. SD accounting now tracks duplicate asset handles
so background reads remain visible. This adapts Melee's file ownership model,
not its asset layout. Physical SD contention/suspend validation remains open.

The independent PICA path now adapts Melee command_cache.c's direct process
cache flush with sticky GSP fallback. Link wrapping flushes every command segment,
including Citro3D's internal clear/transfer splits, before publishing it; failure
withholds the segment. FrameEnd retains GX_CMDLIST_FLUSH, which *avoids* Citro3D's
full-linear-heap fallback. An initial hypothesis that the ordinary frame flag
was causing full-heap flushes was incorrect. Native UI/texture/resident geometry
ranges have explicit flushes. Legacy GX command behavior is unchanged. Model
refresh first retires the previous frame, but drains the current frame only when
that asset has already been referenced there. Reuse/free still uses a full GPU
completion fence. Emulator pass cannot establish physical cache correctness.

Ordinary async integration exposed the old64-request queue's incorrect sizing:
Foxhollow has a94-entry file-info pool; native peak was78. Adopted Melee's bounded
128-request capacity (additional1280B BSS), with94-request regression and explicit
peak/rejection counts. Main callbacks still run in FIFO order; no loading-state
or game work is bypassed. Physical async throughput has not been measured.

Native PICA base lighting (6 October) adapts pinned Melee vertex.v.pica,
lighting_uniforms.h, unit_attenuation.h and lighting_verify.h GX equations.
Generated by tools/generate_minimal_shader.py (GPL-3.0); light packing is
native_lighting.c. Foxhollow retains22 bones and original light objects;
normal inverse-transposes execute on PICA. Citro3D's12-attribute limit and
boolean-location convention are enforced. No Melee BE8 bridge is used.

The lighting input layout also uses Melee renderer.c vertex_attributes'
explicit64-bit attribute permutation workaround for Citro3D's32-bit shift
in slots8 and above. Twelve identity-mapped inputs are set explicitly.

Native material ordering (6 October) retains Foxhollow's original material
callbacks at their original instruction positions. Geometry submission now
follows those callbacks, rather than preceding the instruction loop. Callback
models submit individual sections; ordinary models keep resident group batching.
The THP callback captures its initialized texture object for the native opaque,
unlit screen material. Existing private texture copies and completion fences
retain source-buffer independence; no decoded movie buffer is borrowed by GPU
commands. Pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c,
texture_storage.h and command_cache.c were reviewed for storage, format and
publication rules. This adapts ownership rules, not Melee-specific materials.

Following the same pinned Melee log_io.c ownership model, minimal-render.csv
now joins the existing bounded asynchronous diagnostic writer's whitelist.
Queue/storage budgets and joined-close behavior are unchanged; saves and assets
remain excluded. Added monotonic frame-submission intervals measure whole
submission cadence, not display scanout or emulator-to-console scaling.

Native startup repair (6 October) rechecked pinned Melee renderer.c,
vertex.v.pica and command_cache.c plus pinned Citro3D attribs.c/base.c. UI now
uses an independent11-instruction shader and40-byte three-attribute vertices;
it has no model palette, lighting branch or fixed-attribute dependency. Model
bindings initialize every enabled fixed attribute. Melee's explicit full
texture output writes are retained, including unused Z/W lanes. The new
assembled shader checker conservatively requires full output-register writes
on every IF path, rejects duplicate writes/adjacent MOVA and bounds IF depth.
This is a stronger initialization policy, not a complete PICA emulator or
proof of the physical timeout cause. Hardware rule references reviewed:
https://www.3dbrew.org/wiki/GPU/Pitfalls and devkitPro libctru shbin.c.

Primary sphere-map sampling follows Foxhollow shader_dolphin.c's
addSphereMapTexStage, using the original per-object intensity and reference.
Native texture1 adds map*intensity to raster color before base multiplication.
It uses the existing privately copied/fenced cache, pinning the base during
second-image acquisition. View-space normals currently use the native shader's
normalized inverse transpose, an approximation to the original texture matrix.
Rcp_UpdateDistortionTextures' generated maps, secondary/specular stages and
projected lights remain incomplete; this must not be called restored lighting.


6 October movie-delay repair: platform_movie_prefetch.c and dvd.c follow
pinned melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
native-3ds/port/3ds/file_io.c's sleeping same-core worker, BUSY polling,
release/acquire publication and joined shutdown (U64_MAX). Foxhollow retains
its validated prepared THP file and a bounded private prefetch buffer instead
of Melee's generic borrowed request buffer. Main alone copies completed movie
bytes and invokes original DVD FIFO callbacks; movie cancellation never leaves
worker access to caller memory. Removed the port-specific three-second fatal
read deadline; this was not a Melee requirement. Actual synchronous callers and
close still wait for ownership. Regressions inject six-second latency and
reject the old blocking watchdog behavior. No service permissions or extra
cores were added; no hardware timing equivalence is inferred from Azahar.


6 October native diffuse sphere generation and sky backdrop:
platform_minimal_graphics.c follows pinned Melee renderer.c texture-backed
render targets, VRAM texture allocation and fixed-attribute initialization.
Reviewed pinned Citro3D framebuffer.c, base.c and renderqueue.c: framebuffer
switches flush preceding writes before rebinding, texture-unit updates invalidate
the sampler cache, and FrameDrawOn restores the destination viewport. Six
bounded32x32RGBA8 surfaces (at most24576VRAM bytes) replace the original diffuse
RCP mesh/copy pass; no CPU GPU-color/depth readback. Original light selection,
channel colors, slot groups and reference counts remain. Reconstructed17x17
vertices preserve the original outer-ring normals without renormalizing them;
1536triangle indices preserve strip diagonals. Main and UI shader fixed/output
initialization repairs remain. This is not the secondary/specular RCP pass.

The time-of-day backdrop keeps original CPU texture blending, time advancement,
lighting-color updates, visibility guards and pitch/roll UVs from engine5.
One native UI quad uses PICA INTERPOLATE for texture*(1-C1.a)+C1.rgb*C1.a,
opaque blending and original texture alpha. Zero tint alpha must still draw.
This adds a named UI blend mode instead of sending the quad through genericGX.
Desktop reference still shows missing clouds/rain, and sphere coordinate mapping
remains approximate; this is integrated partial restoration, not fidelity proof.

6 October native alternate models/Palace repair: consulted pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c and scissor.h. Reused existing
licensed mp_scissor_rect for model/world400x240rotated clip rectangles; UI and
32x32offscreen targets explicitly reset scissor. Cloud occlusion uses PICA depth
write mask without color and original0x80alpha cutoff. Unlit alternate models
use original object/tint color and texture modulation, avoiding arbitrary lights.
World materials are already pointer-relocated by Foxhollow; model material
handles retain their original lookup. This ARM pointer/handle distinction is
Foxhollow-specific and not imported from Melee's BE8 machinery. Scene metadata
is demand allocated within independently retained normal/linear safety reserves.


6 October 2026: static world PICA materials and Old3DS target

Pinned Melee renderer.c (c2ebe7c769b3287f2b3748173e8fe5ec24ef408e)
provides the two-UV attribute/TEV, stable aliased sampler descriptor and GPU
ownership patterns. SFA's native20instruction world shader is new, with full
output writes as in Melee. Blend equations are derived from Foxhollow's
shader_dolphin.c KColor(mode0),mode9,vertex/KAlpha sequence; they are not Melee's
shield formulas. Independent UVs and original animation/scroll offsets remain.
No extra CPU readback or unbounded cache allocation is introduced.

The user's new Old3DS target deliberately departs from Melee's core2 assumption:
audio ownership queue runs on core0, GX/fragment core2 workers remain inactive,
and existing original-3DS CPU path does not request speedup. Separate40MiBnormal/
12MiBlinear build is experimental and currently fails ARAM allocation in Azahar.
Melee worker publication, fences, joins and APT handling remain. This is not an
Old3DS compatibility/performance claim; no Old3DS deployment has occurred.

6 October: audio asset ownership / smaller desktop working set

Reviewed pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e audio.c's
separate linear PCM allocation, publication/flush and shutdown lifetime, and
services.c/main.c platform boundaries. These changes do not alter NDSP buffers,
thread placement, cache flushing or DSP ownership. Melee has no matching MusyX
PC decoder; this is a Foxhollow-specific departure in generated ARM sources.
The pinned vendor tree is untouched. fh_audio_asset_fields.h replaces the
decoder's per-field vector/qsort with sparse nibble-tag pages, retaining bounds,
alias-conflict rejection, byte-order conversion and transactional cleanup.
fh_audio_asset_batch_impl.h owns a shared conversion ONLY during the game's
explicit, synchronous55-group SFX registration batch. Source identity is not
a global cache contract: a later batch at identical addresses creates a fresh
conversion. References keep converted project/pool/directory alive after the
batch ends until every group is released. The pinned sample registry already
recognizes identical directories, preserves live refcounts, uploads each live
sample once, and removes a directory only after its sample refs drain. Tests
exercise those actual functions, allocation failure and group-release lifetime.
All samples/voices remain; no audio or progression work is skipped. Normal-heap
savings are measured separately from host/emulator timings and from physical
performance. Old40MiB normal heap still fails; no extra memory entitlement is
inferred from Azahar's96MiB process limit.

6 October: native model second UV and TEV material adaptation

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c
attribute/sampler setup and fixed-attribute hardware correction; retain12input
attributes, complete output writes and existing cache/fence ownership. Consulted
pinned Citro3D texenv.c/texture.c and Azahar glsl_fs_shader_gen.cpp for TEV buffer
write visibility. Independent tests implement that delay explicitly. SFA's
objprint_dolphin.c addShaderLayerStages/addLitColorStage define material meaning;
Melee has no equivalent scene-packet format. Departure: SHORT4 primary/secondary
UV at existing attribute2, FHNSCN05 indexed secondUV in per-reference material
lane, three native samplers and up to four TEV stages. Snapshotted descriptors
preserve per-slot wrapping for aliased texture objects. Live texture animation,
alpha and original lighting order are retained. Warped decals, secondary
specular/C2 and nonidentity swaps are not established by this implementation.
No threading, heap entitlement, NDSP or depth-readback changes. Shader float
registers are full96/96; physical cache/TEV correctness still requires console
validation, not an emulator performance multiplier.

6 October: ordered particle billboards in native PICA reconstruction

Reviewed pinned Melee renderer.c reserve_commands/flush_dynamic and vertex
attribute initialization before this change. Reuse the existing fenced60KiB
UI stream; never overwrite it before GPU completion, and preserve partial
framebuffers on rollover. The existing20instruction world shader transforms
unskinned billboard vertices, with fully initialized fixed attribute3. This
avoids introducing generic GX vertex/TEV translation for each particle.
The native queue copies original drawGlow world coordinates, per-vertex alpha,
colors and signed7fractional-bit UVs after all original game calculations.
Lifetime, randomization, source callbacks, resource updates and sorting remain.
Alpha/additive/depth-writing cutout modes retain their original semantics.
Review of pinned Citro3D effect.c established DepthMap viewport mapping; use
scale=-(far-near),offset=1-far and restore defaults after drawing. Camera clip
planes remain unchanged; shifting projection z would incorrectly move them.
Flush batches at texture/material/view/projection/scissor changes and before
all other native draws. Texture mutation and eviction retain existing fences.
Particle and HUD batches share storage but distinct shader/depth state.
Reflection-distortion mode4 remains explicitly unsupported and counted missing;
fog and other particle engines are not claimed complete. This is SFA-specific
material adaptation, not code copied from a Melee effect implementation.

6 October: deferred transparent world geometry

Retain pinned Melee renderer.c queue-ordering, fenced persistent geometry,
scissor conversion and bounded command reservation patterns. No new GPU shader,
cache policy or worker/OS change. The SFA-specific departure is FHNSCN06 world
packets with separate main/transparent/water draw counts and original bit
positions. Native draw lookup uses stream+bit+list+shader, rather than assuming
that repeated display-list IDs imply identical layouts. Source snapshots include
the two deferred instruction arrays; live bounds/visibility checks and original
lightmap queue sorting still run. The transparent hook is immediately before
the original accepted GXCallDisplayList, after its additional visibility tests.
Use the existing20instruction world shader and TEV mapping, restoring original
-.01 transparent viewport via Citro3D DepthMap without changing clip planes.
Water topology is retained but its special material is not yet implemented;
unsupported deferred draws remain counted, not rendered as opaque fake water.
This does not establish radial lights/fog/reflection visual equivalence.
# 6 October: single-UV model material updates

The native path now resolves ordinary single-UV model animation slots and UV
scrolling at the original draw, using the same first matching material slot as
Foxhollow's `objprint_dolphin.c` texture-layer construction. The material's
integer opacity `((renderAlpha + 1) * shaderAlpha) >> 8` reaches the vertex tint;
blend/depth selection still uses the original object alpha and flags. Alternate
implicit-shader models retain their caller-provided tint and fixed texture;
prepared movie materials retain their own texture and unlit opacity.

Consulted pinned Melee `port/3ds/renderer.c` texture binding, independent RGB/
alpha combiners and descriptor lifetime handling. Reuses existing native
shader uniforms, texture cache and three stable sampler descriptors; introduces
no shader instructions, buffers, threads or larger heap. This is an SFA-specific
material adaptation, not a transplanted Melee material. Separate model alpha
masks, full material channel semantics and dark cloud/lighting output remain
unresolved; the inspected Palace cloud textures have no separate alpha mask.


7 October cloud coverage: reviewed pinned Melee renderer texture/alpha state and
linear-memory staging rules. Runtime cloud opacity selection was already correct;
private topology reduction had discarded all opaque cloud vertices. Host RGBA4444
attribute/alpha-boundary correction preserves original packets and gameplay.
Opt-in emulator capture copies CPU vertex/index data out of linear memory before
stdio, bounded to4assets; no physical path or GPU readback. New/Old .3dsx binaries
unchanged by the asset repair. Full evidence IMPLEMENTATION7October and
artifacts/research/native-cloud-alpha. Not a performance gain or Old compatibility.


7October Old memory layout: pinned Melee game.c reserve-aware cache budgeting,
renderer.c GPU retirement/pin lifetime, and audio.c staging reviewed. SFA Old
region capacities are selected from recorded game peaks, not copied from Melee.
19MiBarena,46/6MiBnormal/linear heaps (same52MiBtotal),15%normal and1MiBlinear
reserves. Existing full completion fence retires unpinned texture cache entries
for native scene admission. New layout unchanged. No GPU readback/audio omission.
Old boot now passes; Palace texture thrashing and deadlinefailure remain.
See IMPLEMENTATION7October; no physical hardware/performance acceptance.


7 October surface texture residency: consulted pinned Melee renderer.c texture
identity, eviction pins, completion barrier and texture_vram.h. The latter's
queued VRAM promotion is disabled in its hardware build after GPU hangs; this
port does not enable/copy that experimental path. Existing full GPU completion
before cache retirement and CPU SD staging remain unchanged. SFA-specific v2
private assets add128px opaque ETC1 variants while keeping exact original source
identity and full-resolution UI/alpha paths. Separate quality cache identities
and normalized UV/scroll domains prevent reduced textures leaking into HUD use.
No queue/thread/heap changes. Actual-C sanitizer tests and exact console-binary
Old/New headless Palace checks pass; physical performance remains unverified.


7 October native distance fog: Melee c2ebe7c769b3 engine/gx.c preserves GX fog
parameters; no matching PICA renderer lowering was available. This port reuses
its existing fh_fog.h equations with pinned Citro3D9f21cf7b380c FogLut_FromArray,
FogGasMode and base.c command-list LUT copying. New reconstruction controller
caches one512byte CPU table; no vertex shader/linear/VRAM work. Original enabled/
disabled state survives the generated dispatcher; HUD/movies/sphere lookup passes
disable explicitly. Host negative control rejects the former dispatcher discard.
PICA fixture covers28color/alpha samples; exact console binary has67,095fogged
Palace draws. Heavy fog overlays, particle fog and range adjustment are not yet
implemented. Physical validation/performance remains unproven.

7 October packed static world: consulted pinned Melee c2ebe7c769b3 renderer.c
vertex_attributes/vertex_pointer/layered_vertex_pointer and layered_verify.h.
Retain explicit PICA attribute formats/permutation, independent secondary UVs,
fully initialized shader outputs and owned GPU storage/fences. SFA departs from
Melee's float vertex stream: FHNSCN07 stores original signed16 positions/UVs and
RGBA4 in20bytes instead of64. Existing view/tint/UV uniforms absorb the exact
integer scales; translation and animated UV offsets remain unscaled. The world
shader is unchanged (no additional instructions). Offline conversion verifies
every visible attribute and copies all indices; models remain byte-identical.
No heap enlargement, queued VRAM promotion, thread or SD-service changes.

7 October world-movie routing: Melee's pinned renderer.c vertex binding and
layered_verify.h reviewed. World callback draws now use their20byte native
world layout instead of40byte model fetch, with the existing THP opaque state.
No shader/buffer changes. The focused GPU fixture initially borrowed the raw
color readback approach, but its files were striped while normal displayed
output was correct. Those captures are rejected. Final verification uses the
same isolated frontend capture as game tests, with distinct presented-frame
markers and spatial pixel assertions. No raw readback was added to the port;
hardware timing/correctness still require separate physical evidence.


7 October native reflection/water: reviewed pinned Melee c2ebe7c769b3 renderer.c
gpu_efb_copy (1590+) and texture_vram.h; keep owned texture storage and ordered
producer/sampler transitions. Do not restore its raw output-gap capture: this
port has physical failures for that transfer. The native backend instead uses
a256x512RGBA8 texture-backed framebuffer, active240x400viewport, a depth target,
and a separate240x400display target. Presentation uses the existing audited
VRAM-to-VRAM input-gap copy contract. The framebuffer's active tiled rows start
112rows into storage; the focused GPU fixture caught the initial crop mistake.
A128x128RGB565 target samples the borrowed framebuffer through the existing UI
shader, with explicit split/visibility boundaries and bounded queue capacity
waits. ARM never reads either image. No linear sampled capture, raw depth copy,
texture-cache VRAM promotion, CPU depth readback or queue/thread changes.

SFA's setupWaterCausticTev and mapBlockRenderWater supply the original visible
stream, caustic texture, scroll clock, tint and vertex alpha. The reduced-detail
PICA material uses one projective reflection and low-amplitude caustic brightness
instead of the two GX indirect distortion samples. This is an explicit fidelity
departure under the user's reduced-detail authorization. It preserves geometry,
depth test/no depth writes and alpha blending. A21instruction vertex shader
writes every output, including the projective Q lane. Heavy-fog averaging occurs
before clamp, avoiding a bright-surface darkening error. The separate engine19
water/foam material and two-sample distortion fidelity remain unimplemented.
Dedicated GPU coverage includes varying homogeneousW, RGBA4transparency, heavy
fog tint, caustic brightness and restoration to ordinary world materials.
Hardware correctness/performance and normal in-game water coverage remain open.


7 October Old surface64 experiment: reviewed pinned Melee texture_compact.h
and texture_storage.h, retaining their distinction between opaque and partial
alpha storage. Existing private SFA preparation additionally reduces opaque
surface dimensions to64 under the user's detail-reduction authorization, while
retaining full originals, original source identity and full HUD/effect selection.
No runtime cache, allocator, GPU synchronization, VRAM promotion or OS changes.
All4native builds are unchanged/current. This is an explicit visual departure,
not a Melee-equivalent lossless conversion or a physical performance result.


7 October engine19 ripple/wake native draws: reviewed pinned Melee renderer.c
vertex_pointer/layered_vertex_pointer and existing shared-stream flush ownership.
The port reuses its already fenced 60KiB UI/effect stream and 21-instruction
water shader; no new linear pool, VRAM capture, shader program or OS/thread work.
Camera_LoadModelViewMatrix and original lifetime/visibility loops remain; the
original six triangle vertices retain signed position and 7-bit-fractional UVs.

Foxhollow Aurora shader.cpp confirms indirect sampling ABG: GX_ITBA_U selects
green, GX_ITBA_S selects alpha. Ripples use generated gNewShadowBumpTexture
(the original setup ignores its nominal ripple texture argument); wakes use
gWaterfxWakeTexture. Preserve those channels, original A1 lifetime alpha, tint,
RGB K-color mix, heavy fog averaging, depth test and no depth writes. One native
projective reflection replaces the two GX distorted samples. BUMPN's five-bit
mask quantization is approximated by the full sampled channel, at most7/255
alpha difference before lifetime/blending. This is an authorized visual
reduction, not exact indirect rendering. Splash bands/droplets remain missing.


7 October engine19 splash rings: consulted pinned Melee renderer.c guarded
C3D_DrawArrays/reserve_commands, indexed_draw and apply_cull (front maps to
GPU_CULL_BACK_CCW, back to GPU_CULL_FRONT_CCW). Retain the original two cull
passes in that order. The shared stream uses the existing capacity fence and
cache flush; pin the alpha-mask texture while resolving the foam texture.
No VRAM promotion, CPU readback, new linear pool, OS or threading changes.

Original waterfx_drawSplashBurst still computes all eight transforms and alpha
values. Expand its fifteen16-vertex triangle strips exactly into210triangles;
transform128unique vertices once on CPU, then submit two630-vertex native draws.
This replaces the ignored legacy display lists, not the particle simulation.
The shared water shader grows21->25instructions and5->6output registers for the
independent foam UV scaled12x in U. Existing map water/ripples/wakes continue
through that shader and pass the preceding spatial regression cases.

Authorized reflection reduction: sample one projective reflection instead of
three indirectly distorted images. Retain each original RGB K weight and its
saturating accumulation with ambient/4, mask alpha times original lifetime,
then original foam-alpha interpolation toward ambient/4+192. Heavy fog already
selects the original fog ambient in setupWaterReflectionTev. BUMPN five-bit
quantization remains approximated by the full mask channel (at most7/255 alpha
difference before lifetime/blending). Preserve depth test/no depth writes,
scissor and native fog. Droplet points remain missing. Splash feature coverage
is from the GPU fixture until a normal gameplay route actually emits bursts.

Linked ARM waterfx_drawSplashBurst stack frame is31,040bytes including saved
registers; it contains the630-vertex transient array and128unique transformed
vertices. It uses the existing512KiB main-stack reservation; no heap allocation.
A normal active-splash gameplay stack high-water measurement is still required.


7 October shared-bone native skinning: consulted pinned Melee vertex.v.pica
normal-matrix transform and normalization. SFA retains its full inverse-transpose
for arbitrary nonuniform scale/shear because its 22 position matrices already
use 66 of 96 float registers; a second complete palette would not fit.
When both bone indices match, transform position and normal once. Sphere-light
generation now takes its original supplied normal before the unused inverse
calculations. Distinct-bone weighted skinning and determinant guards remain.
No new attributes, uniforms, shader outputs, persistent buffers or services.
Assembled program grows351->357instructions; executed prefix source operations
fall69->44 for matching bones and66->19 for sphere generation, while distinct
bones grow69->73. These are source-path counts, not measured PICA cycles or FPS.
Actual PICA emulator comparisons cover ten matrix/weight/sphere cases with
byte-identical interiors and an independent inverse-transpose reference.
Runtime single-bone counters count index references, not unique shader calls;
other groups may contain matching-bone vertices too, so coverage is a lower
bound. Physical performance and full-scene correctness still require validation.


7 October paired normal palettes: pinned Melee vertex.v.pica keeps separate
model[30]/normal[30] palettes and uses three DP3 instructions per normal.
Adapt that division of work to SFA: up to eleven position matrices use float
rows0..32, and their full CPU inverse-transpose matrices use33..65. Existing
projection/material/lighting uniforms and five outputs keep their ABI. Use the
last available boolean at0x77; no added float registers or attributes.
The existing22-matrix stack array holds both halves, with no additional
persistent CPU/linear/VRAM storage. Larger palettes and near-singular/unstable
matrices retain the original GPU inverse path and determinant guard. No rigid
or orthogonality approximation. Normalization and weighted blending unchanged.

Regroup private models at original draw boundaries with at most11bones; retain
triangle/index order, attributes, source refs, materials, callbacks and original
per-draw visibility. Every existing individual model draw uses at most10bones.
World packets remain byte-identical. The tradeoff is more draw groups and some
duplicated vertices/uniform uploads, requiring measured runtime CPU/memory
checks. Older22-bone packs remain readable with the GPU fallback.

New model shader367instructions, five outputs, sixteen booleans, existing66
palette rows. NOP after the nested determinant branch separates PICA IF-stack
returns. Existing command-capacity checks, completion fences, texture lifetimes,
raw-depth readback rejection, OS and worker ownership remain unchanged.

7 October normal-helper follow-up: retain the same paired-palette adaptation and
all determinant/range guards, but spell out fixed3x3cofactors/output stores.
No new cache, matrix approximation, allocation, draw grouping or hardware path.
12000host cases preserve accepted bits and rejection decisions; ARM11 output
hashes match at all counts. Eleven-bone helper4274->1787instructions under
actual platform GCC/O2 flags, excluding upload/callsite work; not cycles or FPS.
Keep v10 as default: v11 grouping/uniform-submission tradeoff remains unproven.


7 October: native sampler transition and timeout observation. Consulted pinned
melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c bind_program:
leaving the dual-map path explicitly disables the extra texture unit. Native
world now analogously disables unit2 on entry (its shader emits texcoord0/1),
and sphere generation disables all unused samplers. Citro3D NULL binding fix
remains the pinned adaptation. The GPU device regression seeds a preceding
unit2 binding and verifies it is cleared; default pixels remain exact.

Failure-only GX queue logging and a bounded owned command-buffer snapshot use
Citro3D 9f21cf7b380ce6f9e01a0420f19f0763e5443ca7 internal context and libctru's
public gxCmdQueue_s entries. This is a port-specific diagnostic, not a Melee
render algorithm or a confirmed fix for bd92's physical timeout. No new fences,
normal-frame disk writes, physical-memory probing or raw depth readback.


## Native world point lights (7 October)

The native map adapter retains Foxhollow 33f3079b's original per-display-list
AABB selection (up to two lights), view-space positions, live RGB/radii,
visibility and deferred ordering. It replaces only generic GX light-stage
construction. Original additive, modulate and vertex-alpha blend equations
remain post-texture; alpha itself passes through. Zero-light materials retain
their original dimming behavior.

Consulted pinned Melee c2ebe7c renderer.c bind_program and explicit sampler
unbinds, and its shader full-output initialization discipline. Melee does not
supply this point-light implementation. This is a departure using pinned
Citro3D 9f21cf7 light.c/lightenv.c/lightlut.c: two ordinary C3D lights with
normal-independent specular0 accumulation and distance attenuation. One shared
immutable 1KiB LUT is initialized once; per-light radius changes only its
public attenuation scale/dirty state, keeping Citro3D APT restoration. The
lighting environment is unbound after each world draw. No geometry pass,
linear allocation, readback or disk IO is added. World vertex output gains
view position and identity normal quaternion; original sampler2 reset remains.

Authorized visual reduction: isotropic max(0,1-distance/radius) replaces the
original product of view-XZ radial and view-Y radial texture falloffs. This is
not pixel-identical GX lighting. A continuous 40,401-point sweep gives maximum
weight difference 0.085778 (not a bound on original sampled/quantized output).
Selected lights, colors, radii and material equations are preserved. Actual C
combiner equations pass 72,900 host cases; the GPU fixture tests two-light
falloff, three blend modes, zero-light cases, layered materials and restoration.
Physical crash/performance acceptance remains pending on a separate unchanged
diagnostic build; these additions have not been deployed.


## Startup per-draw completion diagnostic (7 October)

Following physical9ec's outstanding rendering command list, the diagnostic
wraps original C3D DrawArrays/DrawElements calls only while minimal frame<=2.
C3D_FrameSplit(GX_CMDLIST_FLUSH) and libctru gxCmdQueueRun/Wait retire each draw;
existing Melee-derived async_renderqueue.py capacity reservation/join performs
queue recycling. It does not bypass game work or alter shaders/materials. The
pinned Melee explicit completion ownership, Citro3D renderqueue.c FrameSplit,
and queue publication adapter were inspected. This is a diagnostic departure,
not a performance configuration; it must be removed after crash localization.
The bounded failure-only queue/command snapshot remains. Actual hook180host
ASan/UBSan cases cover immediate/delayed/timeout paths and absence of work past
frame2. Exact console emulator run completes all67initial draws and drains
ordinary Select shutdown. Physical cause/compatibility remains unproven.


## Smaller normal palettes after physical model-draw localization (7 October)

Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e vertex.v.pica and generator
provide separate CPU-generated position/normal palettes. Existing native v11
pack uses11bone groups within our66rows, preserving original sections and
triangles; Melee uses10+10. Physicalbc9 stalls on model465's22bone inverse
path; smaller groups avoid that path for measured startup. Exact cause is
unproven and extra draws/uniform work can cost CPU time. No added linear/VRAM
buffers or shader changes; existing startup completion probe is retained.


## 7 October: Shared native PICA instruction store and early native submission

Pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e renderer.c shares the
stereo DVLP across entry points to avoid code uploads when switching paths.
This port applies that idea to model/UI/world/water vertex entries. A strict
host linker retains each independently assembled constant, uniform and output
table, relocates IF addresses and operand descriptors, and compares all433
operations against the original binaries. Pinned Citro3D base.c selects code
uploads by DVLP identity. Uniform reloads remain on every binding. The model
entry consumes all96 float registers, unchanged;433/512instructions and52/128
operand descriptors fit. Picasso uniform sharing rules were checked against
https://github.com/devkitPro/picasso/blob/master/Manual.md ; a direct multi-file
assembly would share/renumber our intentionally aliased CPU uniform ranges,
so it is not used. All four programs are freed before the shared DVLB.

Native draws now submit immutable command prefixes in batches of8 through the
existing adapted Melee early_queue.c/async_renderqueue.py ownership protocol.
Producers flush resource ranges before drawing; FrameSplit flushes commands.
Presentation reserves its queue tail. FrameEnd defers completion ownership,
and only a successful next FrameBegin retires it. Texture retirement, changed
native geometry and reused UI streams retain their existing full fences.
No additional geometry buffers, threads or larger heap allocations are added.
The first three startup frames retain serialized failure probes. This changes
submission overlap, not game simulation or draw ordering; physical timing and
Palace switch hang recovery are not established by host/emulator checks.

## 7 October: Failure-only CPU input snapshot

Consulted pinned Melee citro3d_fix.c (physical buffer addressing and bounded
command packets), existing completion fences, and CPU-only copy ownership.
This port adds CPU-view linear-heap evidence because its Palace stall dump
lacked live vertex/index inputs. It deliberately does not copy/read VRAM or
invalidate CPU/GPU caches while a GPU fence is outstanding. The existing
nonfatal resource retention remains; this capture does not recover a deadlock.
libctru's allocator globals were verified in the linked ELF and official
source: https://github.com/devkitPro/libctru/blob/master/libctru/source/allocator/linear.cpp
and https://github.com/devkitPro/libctru/blob/master/libctru/source/system/allocateHeaps.c .
They describe the same contiguous linear allocation; no arbitrary address
range or desktop-sized secondary heap is assumed. Per-launch capture bounded
to the configured24MiB/6MiB,64KiB temporary normal scratch, private SD file.
Offline decoder field layouts/alignment follow pinned Azahar regs_pipeline.h
and vertex_loader.cpp (GPLv2-or-later, license retained as citra-GPL-2.0.txt)
and pinned Citro3D buffers.c. Unknown inherited/default state remains explicit;
checked CPU values are not asserted to be a coherent live GPU state.

## 7 October: Cached static-world chunk bounds

Adapted pinned Melee port/engine/geometry_bounds.h four homogeneous side-plane
box tests and0.002magnitude margin. Consulted tests/geometry_bounds_tests.c;
independent double-corner checks cover100,000random configurations. Our world
packets are rigid, already admitted and CPU/GPU-owned;16triangle chunks use
their exact original positions. No mixed-bone inference or screen-size LOD.
Preserve index/material/alpha order and merge consecutive surviving spans.
Difference from Melee's dynamic cache: compute bounds once on asset admission,
using bounded normal metadata under existing headroom guards; free with its
record after GPU join. No extra linear/VRAM storage. Missing metadata retains
original rendering. This adaptation is GPL-3.0 under the existing Melee license.

## 7 October: Ordered world index gathering

Consulted pinned Melee renderer.c flush_dynamic/reserve_dynamic/reserve_commands
and its geometry_bounds.h. Four-triangle world bounds replace the earlier
sixteen-triangle granularity. Adjacent surviving spans are copied in original
order into an optional, fixed 64 KiB linear index stream, allowing one PICA
draw without changing any vertex, material, triangle winding or alpha order.
Each publication begins on a128-byte boundary, is explicitly cache-flushed
before command publication, and remains immutable until successful FrameBegin
retires the prior readers. Capacity checks precede copying; no implicit wrap,
growth, per-frame allocation or forced flush to reclaim a full index stream.
Full/unavailable streams retain the original separate-span draw path. Unlike
Melee's larger dynamic vertex/index buffers, this contains indices only; all
vertex data remains in persistent asset storage. Normal metadata remains under
the existing scene and global headroom guards; no added VRAM or texture memory.
The acquired-buffer reset is exercised in delayed and permanently busy queue
tests, not assumed from vblank. This follows the existing GPL-3.0 adaptation.


## Explicit native vertex padding (7 October)

Pinned Melee c2ebe7c renderer.c matrix_vertex_pointer uses descriptor D to
skip eight unused bytes while preserving the interleaved vertex stride.
Apply this same explicit-padding approach to Foxhollow water: packed20-byte
vertices fetch16 bytes plus C padding4; float64 fetch40 plus F16/D8 padding.
Float world vertices fetch52 plus E12 padding. No asset/shader/timing/effect
changes, new allocation or additional GPU vertex attributes are introduced.
References: reference/azahar/src/video_core/pica/vertex_loader.cpp padding
handling and https://www.3dbrew.org/wiki/GPU/Pitfalls stride/extent restriction.
Azahar does not reject the old extent mismatch; emulator rendering success
alone was insufficient. tools/test_native_vertex_layout.py compiles actual
production bindings, checks all4 layouts and rejects3 restored bad descriptors
plus both captured physical hangs. analyze_gpu_inputs.py now audits layouts.
The physical queue interpretation is corrected: libctru gx.h lastEntry counts
completed entries; curEntry names the first unsubmitted entry. In12a queue15
is submitted/unfinished and starts with water98/99; queue16 was not submitted.
Older decab queue0 begins with the same bad water descriptors. This identifies
a strong candidate, not proof that a physical GPU freeze has been eliminated.


## Attribute-aware private geometry preparation (7 October)

Consulted pinned Melee c2ebe7c vertex.v.pica (separate position/normal transforms,
vertex colour and light response), renderer.c/citro3d_fix.c (texture state and
cache invalidation), and early_queue.c (resource retirement) during this review.
No Melee runtime code or new hardware synchronization is imported by this change.
The host-only departure is Foxhollow asset simplification using already pinned
meshoptimizer73583c3. Its error now measures colour/UVs, model normals and dense
bone influences, rather than geometry alone. Animation/callbacks and material
order remain original. Layered model topology is retained until both UV sets can
be measured. Existing topology/boundary, alpha and exact surviving-byte checks
remain. See docs/NATIVE_GEOMETRY_LOD.md: the integrated lighting discrepancy is
unresolved, so these packages are not accepted for installation or an FPS claim.

7 October GPU-work observer: platform_gpu_work.c adapts pinned Melee c2ebe7c
native-3ds/port/3ds/gpu_busy.c (GPL-3.0). Retains unlocked forwarding and union
of outstanding-command intervals. Adds class pairing, errors, unknown events,
and explicitly invalid measurements after errors. Unlike the reference's simple
--wrap pair, this SDK needs a local weak-symbol adapter in gspgpu.o; see
tools/build_gpu_observer_ctru.py. No queue state machine or SDK instructions
are modified. Linked IRQ-call guard and real console-binary headless counts
verify observation. This is not a hardware-hang fix or physical timing proof.

7 October collapsed current-mesh refinement: consulted pinned Melee c2ebe7c
native-3ds/port/engine/geometry_bounds.h and vertex.v.pica. Retains conservative
box support and rounding margins, extending to Foxhollow compact position/tail
decoding and convex skin blends. Unlike Melee's cached single-matrix bounds, this
rare collapsed-transform fallback reads current vertices and encloses every
palette bone, avoiding stale bounds for animated meshes. No allocation, GPU/OS
protocol change or ordinary-scale vertex walk. Pixel-center coverage is retained.
Both captured Pepper model457 draws are culled; physical hang recovery unproven.

7 October native clear restoration: consulted pinned Melee c2ebe7c renderer.c
frame clear and citro3d_fix.c state/ownership handling. Retain ordinary Citro3D
RenderTargetClear with existing queue fences. Foxhollow's GXCopyDisp(clear)
color/depth is latched for its next native frame, replacing the early blue
diagnostic constant; use its existing reversed24-bit depth mapping. No raw
depth readback, additional target or GPU protocol change. Private v13 geometry
adds opening-flight maps through the existing original-reference compiler;
no Melee asset or game assumption is copied. Tests do not establish physical
Pepper hang recovery or performance.


7 October systemic freeze investigation: reviewed pinned Melee c2ebe7c
port/engine/geometry_bounds.h (homogeneous side-plane rejection) and
port/3ds/uniform_upload.h (bitwise actual-uploaded values). Local collapsed-mesh
bounds use a full bone union, current compact positions, inflated clip intervals,
and maxW side tests; unlike Melee no single-matrix cache is assumed. Private
Citro3D9f21cf7 uniforms.c suppresses identical whole float32 dirty batches and
invalidates shadow validity on float24 constants/context restore. SDK unchanged.
Standalone captured-workload probe follows pinned Citro3D drawElements.c and
base.c lighting-disable ordering; no new game GPU queue/reset protocol.
Hardware checks consulted https://www.3dbrew.org/wiki/GPU/Pitfalls and
https://3dbrew.org/wiki/Shader_Instruction_Set for output-write/MOVA/IF-stack
restrictions. They do not establish a tiny-triangle freeze mechanism. The latest
user explicitly requires a systemic renderer fix; bounds/capture checks alone
must not be reported as resolving the recurring physical hangs.


7 October isolated GPU probe VRAM correction: consult pinned Citro3D
9f21cf7b source/framebuffer.c C3D_FrameBufClear and source/renderqueue.c
C3D_SyncMemoryFill, plus pinned Melee c2ebe7c port/3ds/renderer.c EFB ownership.
Physical dump190 proves CPU stores to allocated VRAM fail in this launch mode.
The private probe now uses GPU fill and completion before sampling; direct
CPU zero loops removed. This adapts SDK device ownership, not game semantics.
The new host read-only mapping regression catches the observed gap in Azahar.
Production renderer unchanged; its existing GPU-clear paths are retained.

7 October retained-resource waits: reviewed pinned Melee c2ebe7c
native-3ds/tools/async_renderqueue.py and port/3ds/renderer.c plus pinned Citro3D
9f21cf7b renderqueue.c. Keep publication/callback locking and ownership; departure
is periodic diagnostic observation of indefinitely blocked queue joins. The
surface-specific panic was removed. No reset or early storage reuse. Private
probe uses SDK attribs.c paired input counts and buffers.c base-relative address
contract; every effective address must be relocated when replaying captured heaps.
These are harness corrections, not production cutscene-fix evidence.

7 October normal-transform physical correction: pinned Melee c2ebe7c
port/3ds/vertex.v.pica supplies position and inverse-normal palettes separately;
its vertex path applies three normal dot products instead of per-vertex matrix
inversion. Foxhollow's66rows hold11position plus11normal matrices. Current935model
groups fit that limit. Extend ARM inverse preparation for finite small/ill-
conditioned matrices, preserving scale/shear and shared bone weighting. Physical
replay de82 proves CPU-normal variant completes while original inverse61 stalls.
The precise PICA inverse-path hazard is not yet instruction-isolated. Singular
near-threshold arithmetic may differ from oldfloat24; no gameplay/timing changes.

7 October rejected file-texture snapshot removal experiment: pinned Melee's
visibility generation policy was reviewed, but Foxhollow republishes unchanged
file textures frequently. Removing CPU copies forced 6359 uploads by renderer
frame417 in the exact-console firstFox run (baseline482 in the entire1100-retrace
run). Bounded run expired; owned emulator exited. The change is REJECTED and
reverted, not deployed. Keep exact snapshots to prove unchanged published bytes.
Evidence artifacts/research/native-texture-residency/rejected-snapshotless/ and
artifacts/native/console-fox-token-residency. This is evidence against this
approach, not a console FPS measurement.

7 October New 3DS texture residency budget: retain exact CPU snapshots, dynamic
publication validation and completion-before-free, following pinned Melee
c2ebe7c renderer.c/texture_visibility.h. Increase only New CPU source-snapshot
cap from2MiB to4MiB. The allocator's85% normal-heap guard still overrides it;
GPU4MiB and linear4MiB reserve unchanged. Old1MiB snapshot cap unchanged. Physical
2c36 peak normal49,002,376B plus512KiB main stack and2MiB extra is51,623,816B of
71,303,168B (27.60% headroom under that observed workload). Future allocations
still require runtime admission. This reduces needless eviction, not source
precision or rendering work; no hardware gain established for this candidate.
Actual storage tests include repeated publication of20 compressed textures:
2.5MiB exact sources,160KiB GPU; New retains them, Old safely evicts under cap.

7 October prepared geometry startup RAM cache: reviewed pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e native-3ds/port/3ds/file_io.c,
particularly menu_cache immediate reads and sleeping file_worker (215-310).
Adapt the startup residency policy and caller-owned read destinations. Unlike
Melee's raw menu files, Foxhollow's immutable prepared geometry fits normal RAM
as independent lossless 64 KiB LZ4 blocks. This is new code using the existing
bounded decoder, not a copied Melee implementation. Startup reads and verifies
all blocks; normal reads decode into 64 KiB CPU scratch and retain the existing
normal-to-linear staging/GPU ownership fences. No added GPU passes, linear RAM,
VRAM or threads. New-only cap is 7 MiB including scratch, subordinate to the
existing 15% normal-heap reserve. Scene/texture admission may release the cache
before evicting live assets; restore the original stream cursor before freeing.
Old 3DS retains file reads. Original prepared pack, packet CRC and live game
asset identity admission remain authoritative. Runtime SD fallback is measured;
cache residency is not evidence that disc/texture streaming stalls are solved.

7 October prepared texture startup residency: reviewed the same pinned Melee
file_io.c startup-cache and bounded-reader policies, renderer.c linear allocation
lifetimes, and pinned Citro3D source/texture.c. Foxhollow retains exact source
bytes for texture identity; CRC is only a lookup key. Deduplicate matching
sources only after byte comparison. Read every prepared payload and verify its
CRC at startup into caller-owned CPU-readable linear storage, using a 4 KiB
normal-memory SD staging buffer. New-only maximum11MiB is discretionary and
subordinate to the existing4MiB linear reserve. GPU textures retain independent
owned copies and normal publication fences; cache bytes are never referenced
by GPU commands. Thus memory pressure may release this cache without waiting
for GPU completion or discarding live textures. Old keeps ordinary file reads.
No new GPU pass, shader, format or visual reduction. Host pressure test confirms
cache release retains queued/pinned GPU textures. Native texture cache IO logs
separate startup reads, resident reads, runtime SD bytes and pressure releases.

8 October texture write invalidation: reviewed pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
texture_visibility.h and texture_index.h. Retain exact image/palette byte-range
overlap and source identity; the new fh_texture_write_index.h is an independent
conservative page-to-slot index for write notifications, not Melee's key lookup.
Collisions only add candidates. Cache entries retain verified image/palette
lengths, removing per-notification dimension divisions. Remove indexed slots at
the existing fenced retirement point. UI flush order, exact content snapshots,
asset generations and GPU lifetimes are unchanged. Renderer-owner-only static
normal storage is 16 KiB New / 8 KiB Old; no GPU, linear or VRAM allocation.

8 October command packet batching: reviewed pinned Melee
c2ebe7c769b3287f2b3748173e8fe5ec24ef408e `port/3ds/citro3d_fix.c`
(`mp_native_draw_elements`) and `port/3ds/renderer.c` command reservation.
`tools/packed_commands.py` adapts the checked packet approach to private
Citro3D drawElements, drawArrays and EffectBind. It extends Melee's triangle
packet to all existing primitive paths and raster effects by preserving the
pinned Citro3D source statements. Command words, masks, ordering, padding,
primitive restart, draw-mode toggles and post-vertex-cache flushes are identical.
Bounds are checked after context publication, before raw command copying.
Foxhollow retains its own panic path and existing GX_CMDLIST_FLUSH/range flushing;
Melee's frame-end flags and platform-specific synchronization are not imported.
No suppression of GPU work, new queue, service, worker or persistent allocation.
Generated library sources retain Citro3D's zlib license; the Melee adaptation
remains within the existing GPLv3 component provenance above.
Host source comparisons and actual SDK ARM serializer checks are recorded in
`artifacts/research/native-command-packets/`; they do not establish hardware FPS.

8 October broadphase preparation: reviewed the same pinned Melee revision's
`port/engine/geometry_bounds.h`, `geometry_dirty.h` and `collision_matrix.c`.
World culling retains the same homogeneous side planes, scalar arithmetic
order and conservative margin. Prepare each plane's support-corner offsets
and constant magnitude once before its box loop. Access float subobjects by
validated layout/byte offset; no one-past-array aliasing. The collision rewrite
is independent Foxhollow code: reject the same integer axis bounds before
constructing float vertices and output records. Read current source vertices,
offsets and flags on every query; no persistent geometry cache or pose repair.
Preserve accepted triangle bytes/order, capacity exits and normal/edge math.
Rejected scratch output at the returned pointer is not a caller-visible record;
the broadphase caller uses only the returned count. Tests explicitly compare
that live range and guard allocation boundaries. No new heap/linear/VRAM pool,
GPU work, service, thread or memory-publication change. Evidence is under
`artifacts/research/native-broadphase/`; console FPS remains unvalidated.

8 October combined native submission experiment: reviewed pinned Melee
`port/3ds/render_worker.c` and `render_snapshot.h` at
`c2ebe7c769b3287f2b3748173e8fe5ec24ef408e`. Adapted its immutable descriptors,
release/acquire CPU retirement, separate GPU retirement and worker failure
handoff. Foxhollow resolves/converts textures on main, then emplaces copied
model/world/UI state into 32 bounded slots (113,664 normal-RAM bytes).
Persistent geometry and GPU images remain borrowed: texture eviction and
geometry refresh/release join CPU descriptors before the existing GPU fence.
Sampler descriptors used by Citro3D have persistent consumer storage.

Unlike Melee's dedicated worker/source arenas, this opt-in experiment attaches
to Foxhollow's existing core-2 audio owner and adds no thread, linear pool or
VRAM allocation. Audio is serviced between draws and during GPU waits. It
captures floating-point control, serializes special water/sphere/EFB paths,
joins before presentation/suspend/shutdown and detaches before audio shutdown.
An internal command drain never resets the producer's UI allocation cursor.
Queue waits retain resources rather than turning a long GPU stall into teardown.
Old 3DS remains synchronous. Cache-flush telemetry is separated by owner;
range publication and explicit GX_CMDLIST_FLUSH policy are retained.

Host queue/cache checks and four headless routes are recorded in
`artifacts/research/native-prepared-draws/worker-integration.json`. This does
not establish physical speed, complete material fidelity, audio-signal fidelity,
real APT behavior, or correctness across every game transition. Default remains
off (`native_worker.txt=1` opts in); it is not performance-accepted or deployed.
