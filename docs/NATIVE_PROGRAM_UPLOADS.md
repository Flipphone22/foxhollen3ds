# Load-prepared native shader uploads

The new program-plan owner prepares all four vertex/geometry upload flag
combinations at SDK program setters. The prior owner cached only false/false
configuration and regenerated code-upload sequences every time.

Preparation uses the exact installed SDK writer, including upload order,
chunking, padding, output maps and GS modes. Draw-time publication copies the
chosen immutable sequence into the normal command buffer. It does not expose
cached CPU allocations to the GPU. Existing command-buffer flushes, retirement,
SDK state invalidation and GS/VSH split boundaries remain responsible for GPU
execution. The pinned Melee command_cache.c publication/flush implementation was
reviewed; no new raw readback, command-list jump, service or worker is added.

A 64-entry table retains owners across hash collisions; tombstones preserve
lookup after removal. Program init/free/setters invalidate and release their
prepared allocation. The DVLE metadata/code must remain immutable between
setters, as it already did for the prior configuration cache. All four variants
are prepared at load time; missing/unadmitted programs use the SDK without any
runtime allocation or preparation. This is SDK compatibility, not the retired
source renderer. Runtime counters expose admission failures and direct calls.

Persistent normal RAM is capped at 256 KiB plus the 3,072-byte owner table and
counters. Load-only scratch is 32 KiB on the existing main/graphics stack.
There is no added linear RAM, VRAM, texture/vertex/command-buffer capacity, GPU
instruction or pass. Each temporary variant is bounded to 2,048 command words;
admission requires at most 512 code words and 128 descriptor words per stage.
These are application bounds, not purported global SDK or PICA limits.
Allocation/capacity failures must preserve the original SDK behavior.

The linked ARM SDK oracle passes 7,874 checks / 1,612 command comparisons,
including all 18 embedded shader binaries and their entries, all four upload
combinations, GS modes, owner/hash churn, same-address replacement, permutation
changes, exhausted cache and injected allocation failure. Prepared draws do not
compile or allocate. Short destination rejection occurs before any write/cursor
change. Largest tested sequence is 1,358 words. Stale permutation, wrong upload
flags and a corrupted uploaded word are rejected. A synthetic full-upload case
uses 6,577 -> 4,537 ARM instruction encounters; this is not cycles or FPS.

The initial test adaptation retained an old 512-word destination assertion
while its buffer had been enlarged to 2,048 words; it failed before the first
comparison. Correcting that fixture assertion allowed the actual SDK comparison
to execute. Failure reporting now captures the failing line at entry, rather
than reading a global before the failure function has stored it.

Use tools/test_program_upload_plan.py, tools/build_program_upload_plan.py
console-setup --revision v1, and tools/run_program_upload_plan.py benchmark
--family unified-program-upload --revision v1 --old --profile in the established
WSL environment. Builders isolate archived sources and restore unrelated root
work. Do not build the unrelated root worktree directly.

Evidence lives under program-upload-plan-test/ and the corresponding archived
build/runtime directories. Physical cache correctness, sustained performance
and the full port goal remain unproven. Gameplay results must be recorded before
promoting this candidate. The user-accepted shadow flicker fix is unchanged.

## Integrated result

Shared source ee121967dec3 passes the Old palace profile (106.07 host seconds)
and New GPU audit (90.64 seconds). All saves/settings/fixture hashes remain
unchanged, logs drain, and owned processes exit. Both palace images are inspected.
The final New audit covers 110,440 draws with zero layout faults; the summary's
intermediate checking snapshot is not the final audit.

Both runs prepare 33 plans and use 121,600 bytes of cached commands, below the
262,144-byte cap. Old/New report 15,872/15,622 cache hits, zero direct SDK calls
and zero rejections. No program setter or SDK shader configuration executes in
the Old eight-frame profile. The world draw count remains 768 (528 grouped).
No shader program, geometry, texture or effect was removed by this patch.

Old GPUCMD_Add entries fall 8,541 -> 5,057 and shaderProgramConfigure entries
136 -> 0. Whole ARM encounters are 22,630,009 -> 22,601,366 (-0.127% raw), with
25 -> 27 audio jobs, different stack-probe and particle phases. This is not an
attributable full-frame speedup. Prepared commands still get copied, and the GPU
still receives the original shader uploads; no bandwidth saving is claimed.

Old observed heap is 41,856,680/50,331,648 (+121,880 bytes), linear free minimum
1,688,704/10,485,760, process 69,595,136/83,886,080. New heap is
56,733,944/71,303,168, linear free minimum 16,368,768/25,165,824, process
104,738,816/130,023,424. Both have 4,723,712 free VRAM and observed main stack
72,984/524,288. The owner table replaces a larger old static table, so process
commit does not rise despite the bounded heap allocations. These are scene
samples, not worst-case full-game proof.

The game archive and all 18 GPU programs are unchanged. Shared Old80/New124,
recipe and root/archive restoration pass. No fresh conversion or redundant
normal boot was performed; unchanged recipe evidence is explicitly reused.
The package foxhollow-iso-setup-9034b37a21356c01.zip is prepared, NOT deployed.
Old/New payload identity and stale-CIA rejection pass. It contains no private
assets/settings. Physical FPS remains the earlier 13.5157 / 73.988 ms result.
Full parity, sustained target performance and gameplay/endurance remain open.

program-upload-plan-test/integration.json binds component proof, source/ELF,
runtime/captures, memory, comparison and package. The next design question is
remaining command copying/upload bandwidth, not another state-generator wrapper.
Persistent GPU-visible packets would require explicit linear-memory admission
and retirement; the current CPU-only cache does not provide those guarantees.
