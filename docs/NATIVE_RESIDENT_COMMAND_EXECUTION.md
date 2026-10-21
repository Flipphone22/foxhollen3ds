# Resident GPU command execution

16 October 2026. Experimental implementation; **not enabled in Foxhollow and
not deployed**. Full renderer parity and performance goals remain incomplete.

The existing native renderer prepares geometry and material commands at load,
but copies retained command fragments into the current SDK command segment.
This experiment lets PICA execute an immutable draw packet directly from linear
memory. It is a potential building block for larger prepared draw groups, not
an accepted replacement for every small packet.

## Mechanism and ownership

`pica_command_chain.c` validates the native 30-word indexed triangle packet once
at preparation and appends a channel-1 return, producing a 128-byte leaf. It
preserves the two source primitive-cache writes, draw-mode transitions, vertex
cache clear, index count and index address. No triangles are omitted.

The caller sets channel 0 to the leaf and channel 1 to the remaining caller
segment. A 12-word call (14 if alignment padding is needed) replaces the copy.
Return sizes and addresses are relocated only when the SDK segment is complete,
before its cache flush and submission. All relocations are checked before any
are changed. Leaves remain immutable and resident through the segment's GPU
completion. No nested calls are supported. The API does not allocate, submit,
wait, infer physical addresses or take ownership of SDK rendering state.

PICA's documented jumps do not save a return stack. See [command buffer
registers](https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_CMDBUF_SIZE0).
The pinned Azahar `regs_pipeline.h` describes this subroutine use and its
`pica_core.cpp` implements jumps by replacing the current reader. The pinned
Melee `port/3ds/command_cache.c` flushes every SDK split independently; the
experiment follows that publication boundary, with return relocation before
the flush. It does not disable data-cache publication or change retirement.

Production integration would need a bounded linear command arena, generation
and fence ownership during asset relocation/eviction, SDK handoffs, and larger
groups that preserve every live material, pose and effect boundary. These have
**not** been added to the game. Ordinary process/RAM budgets are unchanged.

## Evidence and cost gate

`command-chain-test/result.json`: 162,208 ASan/UBSan checks; independent bounded
command interpreter executes 800 multi-call sequences. Wrong return units,
skipped continuation, wrong return channel and missing alignment mutants fail.
Actual ARM11 compilation passes. This does not model physical cache coherence.

`artifacts/diagnostic/command-chain-v2/summary.json`: six local Azahar GPU cases,
384 total draws and 224 resident calls, including SDK-only, inline copy, repeated
leaf calls, split segments, alternating SDK/calls, and single-call segments.
All six RGBA captures equal the SDK output byte for byte and equal the expected
64-colour fixture; the resident image was visually inspected. The 2.576-second
host runtime is test duration, not a GPU timing or gameplay benchmark.

The first run completed five cases but timed out in the sixth. That harness
queued 64 separate segments in one frame against the finite SDK queue. The
corrected sixth case retires each batch of 16 and checks submission results;
all 64 single-call segments still execute. The failure is preserved in
`command-chain-v1`. No console test was requested or installed.

`command-chain-arm/result.json`: synthetic ARM11 execution compares the actual
call/seal implementation with a 120-byte `memcpy` per draw, with finalization on
both routes. At 280 calls, aligned start:

| Measurement | Inline packet | Resident call |
|---|---:|---:|
| ARM instruction encounters | 39,230 | 44,873 |
| CPU command-buffer writes | 33,616 bytes | 15,696 bytes |
| Total CPU data reads | 39,236 bytes | 33,688 bytes |
| Total CPU data writes, including stack/relocations | 38,120 bytes | 22,476 bytes |

Thus the small-packet version reduces command-buffer writes by 53.3% but adds
14.4% instructions. GPU consumption also grows from 30 words per inline draw to
44–46 words per draw including call/return. These are synthetic counts, not
physical cycles, bandwidth or FPS. Cache behavior could change the tradeoff,
but **this is not sufficient evidence to enable per-draw chaining**. All ten
tested call-count/alignment cases fail the instruction-reduction gate. Larger
prepared groups, with substantially more work amortized per call, are the next
candidate; they need actual scene grouping and full-frame evidence.

No full-frame or physical speedup was established by this experiment. The
previous local DSP-default build `0d6f83a3f5d5` remains the packaged candidate;
its prior 12.68% whole-window instruction reduction against the installed Old
build is unchanged, and it remains undeployed.

## Reproduce locally

Run through the existing WSL `tools/linux_exec.sh` environment:

```text
python3 /work/tools/test_pica_command_chain.py
python3 /work/tools/build_command_chain_gpu.py
python3 /work/tools/run_command_chain_gpu.py --label command-chain-UNIQUE
python3 /work/tools/measure_command_chain_arm.py
```

The GPU runner is isolated, silent, asset-free and bounded to 30 seconds. It
cleans up only its owned process. The builder does not build the root game,
modify archives, transfer files, or touch saves/preferences.

The later ordered-world-packet experiment adds `fh_pica_chain_group_call` for
16-byte-aligned leaves up to 64 KiB. Sealing now validates the actual leaf size
and overlap rather than assuming 128 bytes. The fixed draw API delegates to
this checked mechanism. Updated host evidence covers 810 interpreted sequences
and 162,238 checks; the earlier small-leaf instruction figures above describe
the original fixed-size implementation, not the extended function. Actual
multi-material GPU acceptance is recorded in `NATIVE_WORLD_PACKETS.md` and
`artifacts/diagnostic/world-packet-gpu-v1/`. Neither path is enabled in gameplay.
