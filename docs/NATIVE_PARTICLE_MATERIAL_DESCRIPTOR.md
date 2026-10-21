# Particle material compilation experiments

These prototypes are **not integrated or deployed**. The production parent is
the active-slot producer snapshot `5f03ba1ba7b2`; root renderer WIP remains
untouched. Pinned Melee command-cache publication was reviewed. No experiment
changes GPU resource ownership, shaders, instances, simulation or RNG.

A tiny retained descriptor was tested to remove repeated blend, shape and
static/dynamic classification in `drawGlow`. Three variants preserve actual
producer output over 30,000 ASan/UBSan comparisons each:

| Variant | Retained state | ARM self-work change |
| --- | --- | --- |
| Render and behavior key | 12 bytes | +3.1099% |
| Same key, reject unsupported materials early | 12 bytes | +1.3448% |
| Render key only; consume behavior live | 8 bytes | +1.1403% |

These are identical-input, synthetic producer comparisons, not full-frame
timings or captured gameplay distributions. Clang 18 uses the game's ARM11,
VFP and optimization flags. The fixture stubs external dependencies, includes
assertions, and is not the exact linked game function. Counts include any new
descriptor helper, but exclude unchanged external callees. All variants fail
the local cost gate; **do not promote any as an optimization**. No additional
application build or emulator boot was justified.

The final variant is worse in all three tested input classes: random keys
35,268 to 35,360; homogeneous keys/behavior 65,085 to 65,946; homogeneous render
keys with variable behavior 64,956 to 65,888. Lookup and unpacking cost too much.

Negative tests reject stale render keys, stale live template selection and
wrong shape packing. An initial template mutant accidentally modified both
oracle and candidate; it was restricted to the candidate and the suite rerun.
Original v1 and v2 artifacts and patch source are preserved under
`particle-material-descriptor-v1-test` and `particle-material-descriptor-v2-test`.
The current patch/tool reconstruct v3 in `particle-material-descriptor-test`.

The source inventory scans 1,805 C/header files. Only `expgfx_internal.h` and
the particle engine name `ExpgfxSlot` or `EXPGFX_RUNTIME_DATA`. The owner sets
render flags during `expgfx_addremove`. Correction to the initial summary:
the inventory also contains XOR mutations of attraction and billboard-lock
flags, which DO affect descriptors. The previous summary considered only a
subset of writes. Creation-time compilation requires refresh at these mutations.

This makes creation-time compilation worth investigating. It does **not** prove
absence of raw-pointer, callback, byte-copy or external mutation. A descriptor
must follow slot reuse and the existing staged/direct pool snapshot semantics.
A separate array cannot simply be indexed by a slot that may have been reused
while an older staged snapshot remains live. If reserved slot bytes are used
instead, their ownership and every copy/reset path must first be verified;
the first 64 bytes already alias live simulation fields and cannot be reused.

Reproduction:

```
python3 /work/tools/test_particle_material_descriptor.py
python3 /work/tools/test_particle_material_descriptor_arm.py
python3 /work/tools/audit_particle_material_writes.py
```

The preceding active-slot test's 23.95% synthetic result used GCC 16, whereas
the game TU uses Clang 18. That result remains an isolated instruction result,
not an exact-linked-function or FPS claim. Its separate real-game profile
records scene differences and establishes no causal frame-time gain.
