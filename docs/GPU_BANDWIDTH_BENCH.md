# New 3DS GPU bandwidth benchmark

Launch `foxhollow-gpu-bench` in Homebrew Launcher. It runs automatically, shows
a yellow/blue checkerboard, and exits when finished. Reopen ftpd afterward.
No game assets, audio, saves or game settings are used. The separate
`foxhollow-replay` app measures real game workloads; this synthetic benchmark
does not predict gameplay FPS.

Eight cases run twice, in forward then reverse order: a 512x512 RGBA8 texture
in linear memory or VRAM, each with an idle CPU, integer computation, or CPU
memory copies; plus whole-linear-heap automatic flushing and a repeated
explicit-flush control. Each case has four warmup frames and 24 measured
frames. Every frame submits 32 fullscreen textured quads at 400x240 with depth
rejection disabled. These are diagnostic overdraw, not removed or altered game
work. Display transfer is included in queue timing. No VBlank wait or emulator
overclock is used in the measured loop.

The CPU memory workload copies 64 KiB chunks around two separate 4 MiB ordinary
heap buffers. One chunk is performed after submission, then more while waiting
for completion. Integer work likewise runs after submission. Record actual
bytes/iterations: equal frame counts do not imply equal CPU work or DRAM bytes.
Cache hits, cache capacity and callback scheduling affect interpretation.
`gpu_ms` is Citro3D's queue-completion callback timing, not a hardware cycle
counter or exclusively vertex/fragment time. `submit_ms` includes FrameEnd and
any automatic cache maintenance; `completion_wait_ms` also includes CPU work
and time until the main thread observes completion.

At case boundaries, outside measured batches, read-only GSP service calls
record GPU busy status at offset 0x400034 and the three hotspot registers at
0x400064. Return codes and raw values are recorded in `bench.log`. No profiling
selector, reset or unknown register is written. These unconfigured snapshots
must not be reported as GPU utilization, cycles or cache-miss counts. Addresses
come from the [3dbrew external register map](https://www.3dbrew.org/wiki/GPU/External_Registers).
Azahar returned successful reads containing zeros; physical access/meaning
remains unvalidated.

Results are `/3ds/foxhollow-gpu-bench/{result.json,timings.csv,bench.log}` plus
16 raw color captures. SD output occurs between batches or at completion,
outside the timed frames. Capture files and results are replaced on rerun;
collect them first when retaining a comparison. A GPU completion timeout is
three seconds; resources are retained on that failure to avoid freeing memory
still in use. If TIMEOUT/FAIL is shown, recover the console and collect logs.

The test reserves 12 MiB normal RAM, 24 MiB linear RAM, and a 128 KiB main stack.
Measured headless use: 8,407,304 bytes normal, 21,626,624 bytes linear free,
4,474,880 bytes VRAM free. All three pools exceed 15% headroom. Stack allocation
is bounded; high-water usage and actual CPU clock remain hardware checks.
Immutable vertices/textures are flushed explicitly before use. VRAM uploads
use Citro3D's GPU copy; there are no CPU VRAM stores or depth readbacks.

Rebuild and validate in the locked Linux environment:

```text
python3 /work/tools/build_gpu_bandwidth_bench.py
python3 /work/tools/run_gpu_bandwidth_bench.py --label UNIQUE
```

Headless evidence: `artifacts/diagnostic/gpu-bandwidth-final/`, binary
`3f74595f24a2`. All 384 measured rows completed; all 16 captures contain exactly
48,000 pixels of each expected color and are byte-identical. The checker image
was inspected. The first test correctly rejected zero CPU workload admission;
the final version performs the first chunk before testing completion. The
emulator completes GPU work before subsequent polls, so it cannot establish
physical contention or hotspot-counter behavior.

Pinned Melee `port/3ds/renderer.c::renderer_begin` was consulted for completion
ownership and avoiding added VBlank pacing. Its renderer's immutable resource
publication principles are retained. This independent diagnostic uses the
locked SDK Citro3D rather than Foxhollow's private command optimizations so the
automatic versus explicit flush comparison retains SDK behavior. Game code
continues to use explicit flushing; this test does not imply it currently
flushes the whole linear heap.
