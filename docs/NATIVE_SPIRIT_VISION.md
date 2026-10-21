# Native Spirit Vision candidate

Spirit Vision retains the source's red/green clamps, adds the blue contribution
at half scale, then subtracts that value from the remapped G/B/R texture channels
and multiplies by four. This differs from the sepia filter.

The candidate uses six PICA combiner stages. Stage 0 retains blue×coefficient in
the combiner buffer while stages 1–3 compute bias and clamped red/green. Stage 4
averages the terms; stage 5 subtracts and scales. Three draws with separate RGB
write masks implement the final channel remapping. Alpha comes from the source
bias. The first draw publishes the six stages; later draws change only stage 5
and the write mask. Buffer updates are disabled before returning to other owners.

The source adapter keeps the original trigger and consumes its live coefficients.
`SPIRIT VISION` is off in all presets. When disabled, it returns before packing
coefficients or entering the capture owner. It shares the sepia snapshot and
immutable quads, capturing the current image at every invocation. The effects
have independent unavailable flags. No simulation, unlock, progression or save
is modified.

Cost and limits:

- Four draws and 24 vertices per invocation: one capture, three output channels.
- No new shader, CPU pixel processing, geometry rebuilding or worker.
- Reuses the shared 512 KiB VRAM snapshot and 512-byte linear allocation.
- Adds 180 bytes of material descriptions and two unsigned state counters.
- Enabled gameplay timing, coexistence with other effects and physical behaviour
  remain unvalidated. This is not an accepted performance improvement.

The half weight is approximated by 127/255. Both 127/255 and 128/255 are equally
near 1/2. In 96 GPU colour cases, 127 has RMS error 0.9014 byte levels and maximum
3.7177; 128 has RMS 0.9408 and maximum 3.6628. Thus 127 is not uniformly better.
The repeated-filter owner fixture is the deciding gate: maximum error falls
from 9.9991 to 7.2393 byte levels against ideal compounded math. This is a bounded
test result, not a universal error bound or a comparison with GameCube hardware
rounding. The failed 128 case remains archived.

Evidence lives under `artifacts/research/pica-scene-rewrite/spirit-material-test`
and `spirit-filter-integration`, and `artifacts/diagnostic/spirit-*`:

- Executed original GX setup and native SDK descriptors: 3,145,728 comparisons,
  maximum ideal half-weight error 2 byte levels; three incorrect mutants rejected.
- Actual GPU material fixtures and inspected images: 96 distinct colours,
  12 bias levels and 288 channel draws.
- Actual shared-owner fixture: both filter orders, repeated Spirit Vision,
  disabled paths, colour-owner allocation failures, unchanged restored pattern.
- Strict ARM compilation and the complete archived source-adapter pipeline pass.

The isolated owner fixture stubs game producer joins and invalidation. It does
not prove whole-engine state restoration. The updated `spirit-owner-admission-v1`
fixture covers Spirit-specific VRAM/linear budget rejection and texture, linear
buffer and render-target allocation failure. Each rolls back partial allocations,
rejects only Spirit, and prevents repeated allocation attempts. Four disabled
conditions do no auxiliary work. Mixed-filter pixels remain unchanged against
the earlier fixture's acceptance bound (maximum 7.2393 byte levels).

The diagnostic `post-filter-bench-v1-on-old` replay completed 978 Spirit calls
with clean shutdown and preserved saves. Its inspected capture shows the filter
over the cave, with bright foreground mist; this is deliberately inserted
rendering, not source-triggered gameplay or original-image parity. Concurrent
host work excludes timing attribution. The filter remains off in all presets.
The diagnostic build is not eligible for deployment. Source-triggered game
coverage, performance and physical validation remain open.
