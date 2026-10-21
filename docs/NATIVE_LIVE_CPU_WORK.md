# Retiring repeated CPU validation and stack scanning

The shared application `3b403841b1e1` is built locally, not deployed or packaged.
It extends `a14385518a6f` without changing the game archive, asset format,
conversion recipe, any of the 18 GPU programs, or the Old 80/New 124 MiB grants.
The isolated builder restores unrelated root work and the game archive.

The native packet compiler now validates immutable resident descriptions at
preparation. Each update still checks every live texture configuration and
sampler before mutating commands or cached values. Prepared descriptions remain
borrowed and must stay immutable/alive until packet retirement. No source/GX
interpretation, per-frame allocation, resource copy or new GPU pass is added.

The audio worker retains its exact stack watermark result, 128-job check
cadence and exhaustion threshold. It scans aligned 16-byte and 4-byte blocks,
then locates the exact changed byte. Reads stay inside the painted interval;
`memcpy` avoids an aliasing violation. It still reads the same memory, so this
does not eliminate memory bandwidth. Audio production, synchronization,
priority, core placement, publication and shutdown are unchanged. The pinned
Melee `port/3ds/audio.c` ownership/cache-before-publication behavior was reviewed;
this scanner is a local adaptation, not a copied Melee algorithm.

## Evidence

`native-live-work-test/result.json` records 301,870 ASan/UBSan prefix checks,
including inaccessible end guards and all short alignment/tail combinations.
An ARM11 test covers actual 76 KiB and 236 KiB painted spans: synthetic scan
instruction encounters fall from 1,916,958 to 339,500. These are component
instruction counts, not hardware cycles. The native packet oracle passes
247,500 checks over 1,024 effective draw states; five incorrect-state mutations
are rejected. Invalid immutable input is still rejected during preparation and
late invalid live input cannot partially mutate the packet.

One bounded Old palace replay completed in 107.03 host seconds, with native
DSP/LLE verified, saves/settings preserved, logs drained and the owned emulator
exited. Its frame-1601 capture was inspected: palace floor/walls, Krystal,
purple creatures, particles and HUD remain visible. This is limited scene
coverage, not full fidelity or physical acceptance. No unchanged New startup or
conversion was repeated; prior setup evidence remains applicable by source and
recipe identity. The CPU patch adds no GPU instructions or resources.

The matched recipe's eight-frame ARM window retains 768 world draws, including
528 packet draws across 144 calls. Exact linked DWARF attribution separates
static functions with identical names:

| CPU function bodies | Parent encounters | Candidate encounters |
| --- | ---: | ---: |
| Packet update plus immutable validator | 154,056 | 137,232 |
| Audio worker loop including watermark | 546,018 | 84,093 |
| Entire observed ARM window | 24,282,087 | 24,565,041 |

The packet bodies decrease 10.92%; called helpers are outside that subtotal.
The worker body decrease is not a measurement of all audio work. The complete
window rises 1.17%: the candidate has 64 `lightningDrawBolt` calls and 1,552
native lightning points versus zero in the parent window, alongside particle
and audio scheduling differences. Do not subtract selected functions and call
the remainder an estimated frame time. No attributable whole-frame speedup or
regression is established. LLE/capture host timings remain excluded.

Observed Old budgets match the parent exactly: heap 41,734,800/50,331,648;
linear minimum free 1,688,704/10,485,760; VRAM free 4,723,712;
main stack 72,984/524,288; process 69,611,520/83,886,080 bytes. These are samples,
not proof of worst-case memory headroom through the entire game.

## Reproduction and remaining work

Use `test_native_live_work.py`, `build_native_live_work.py console-setup
--revision v1`, and `run_native_live_work.py benchmark --family
unified-live-work --revision v1 --old --profile` in the existing WSL toolchain.
Build/run output directories are intentionally exclusive; archive a new revision
instead of overwriting evidence. Then run `analyze_arm_blocks.py` on the full run
path, `compare_native_live_work.py`, `attribute_live_work.py`, inspect the capture
and record its review, and run `checkpoint_native_live_work.py`.

`native-live-work-test/integration.json` binds source hashes, exact ELF owners,
runtime, memory and the inspected image. The last hardware result remains
13.5157 FPS / 73.988 ms. The user's 61.19 ms expectation remains an unverified
prediction. Full rendering parity, sustained target performance and final
progression/save/endurance validation remain incomplete. The accepted shadow
flicker repair is unchanged. Next work must address the remaining substantial
producer/submission costs rather than treating these housekeeping savings as
the missing multi-fold speedup.
