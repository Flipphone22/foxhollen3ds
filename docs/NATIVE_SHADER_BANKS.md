# Shared shader bank experiments

Local experiments, **not accepted for deployment**. The accepted normal/replay
boundary remains `prepared-buffers-normal-v1-builds` / `prepared-buffers-v1-builds`.
The requested toggle-and-sky update still needs the sky repair.

The pinned Citro3D `C3D_BindProgram` compares DVLP owners to decide whether to
upload vertex/geometry code. Sharing the owner preserves code across entry
changes while the SDK still publishes constants, booleans, output maps and
geometry configuration. Existing target splits, worker joins, lifetime fences
and invalidation remain intact. No geometry or original shader operations change.

| Candidate | Grouping | Result |
| --- | --- | --- |
| V1 | Actor/HUD/height/shadow/receiver, 454 instructions; particles separate | Cave command bytes rose 3.57%; submission unchanged. Rejected. |
| V2 | Actor/HUD/height/particles/receiver | Linker rejected 556 instructions. The initial count omitted particle helper instructions. No ARM build. |
| V3 | Actor/particles/receiver 500; HUD/height/shadow passes 129 | Builds and renders; cave command bytes rose 1.67%; submission unchanged. Rejected as a performance improvement. |

V3 replay is `d8874521fc0c`; CIA `4c46bf21963b`. Its 456-file snapshot and game
archive live under `artifacts/research/pica-scene-rewrite/common-shader-v3-builds`.
The game archive equals V7. The isolated builder restores all prior source and
archive bytes in `finally`; do not build unrelated VRAM/finite-projector WIP.
There is no normal V3 build and nothing was uploaded.

Validation:

- Actual native bank ownership and pinned SDK binder: 100 lifecycles, 200,000
  intra-bank switches without code-upload flags; cross-bank uploads retained.
  Premature-free mutation is rejected. This does not validate GPU execution.
- Nine relocated vertex entries: 4,096 float24 cases each with bit-identical
  outputs. Receiver vertex/geometry source oracle: 32,768 query/face cases,
  325,584 exact output lanes.
- Static and dynamic particle pipelines: 27,648 emitted vertices each, poisoned
  scratch registers, original position/colour/UV lanes unchanged and fog correct.
- Exact shared CIA content completed the New Azahar cave test in 50.01 host
  seconds. State, saves, shutdown, owned-process cleanup and receiver coverage
  pass: 11 plans, zero declines/missing plans, 715 casters, 5,005 projection draws,
  9,493,770 receiver points. Capture 1410 inspected: world, Fox, HUD, mist and
  shadows present. This is neither complete visual parity nor hardware acceptance.

| Same cave recipe | V7 baseline | V3 |
| --- | ---: | ---: |
| Scene preparation (ms/frame) | 1.951167 | 1.936137 |
| Native submission (ms/frame) | 5.867804 | 5.863929 |
| Whole-run command bytes | 114,245,888 | 116,158,400 |
| Heap peak (bytes) | 48,505,120 | 48,510,136 |
| Process commitment (bytes) | 103,317,504 | 103,321,600 |

Both paced at approximately 59.83 emulated FPS. Animation/particle phases differ;
the small CPU differences do not establish a speedup. Command bytes are submitted
storage, not GPU time. Linear free 18,439,168/25,165,824, VRAM free 4,723,712,
measured main stack 73,008/524,288 bytes. No Old, OFF, normal or physical V3 run
was requested after the unhelpful New result.

Evidence: `common-shader-v3-test/{contract,vm,glow,runtime}.json`,
`common-shader-v3-owner-test`, and `artifacts/native/common-shader-v3-cave-on-new`.
The first V3 runtime attempt stopped before launch at the storage guard; completed
V1/V3 ELF/map/archive files were losslessly NTFS-compressed with SHA verification.
No asset, save or evidence was deleted and the guard was retained.

The isolated binder result is insufficient to predict the full frame: bank
crossings and the amount of code reloaded also matter. Measure actual command
traffic before accepting another grouping. Prefer reducing repeated per-caster
submission without changing source ordering or deleting GPU safety fences.
