# Conversion progress granularity

The deployed startup build `17e602b1c3a8` displays geometry scan progress as
completed banks out of 109. Each bank contains many index-table slots; the count
can stay unchanged while records are decompressed, converted and checkpointed.
It is not a duration or percentage of the whole installation.

The local candidate instead displays `Original geometry: scan records` and
`Performance geometry: scan records`, counting examined index-table slots.
For the supplied PAL revision 1 source there are 120,800 slots, including unused
and duplicate entries, containing 4,613 eligible records in 109 banks. These
produce 2,110 unique sources. Slots are unequal work, so this is not an ETA.
Reduction, packing, cache, textures and movie retain their own stage counters.

The count is derived from table extents already inspected when opening the
scanner. A resumed cursor includes slots in preceding banks and its current
index. The change adds two unsigned counters (eight bytes), no filesystem reads,
allocation or worker. Display publication retains the existing 100 ms throttle
and only publishes changed text. Conversion/cancellation boundaries are unchanged.

Validation:

- `test_install_geometry_iso.py --label install-geometry-record-progress-test`
  checks every source record against the independent ISO parser, all slot
  increments, final total and two real cursor resumes, under ASan/UBSan. ARM
  compilation passes.
- `test_install_task.py --padded --label install-task-record-progress-test`
  runs the actual original/performance/texture stages on a small private fixture,
  including eleven cancellation/recovery cases, source rejection, ownership
  cleanup and refusal to publish without the movie. It passes; tracked explicit
  allocation peak is 9,069,520 bytes, excluding library internals and stack.
- Shared build `normal-setup-record-progress-v1-builds` succeeds with 3DSX
  `9b0c2787d80a` / CIA `175750df293b`, menu source `36b8c12ae661`. Seven progress
  source/header changes versus the pack-I/O candidate are recorded in
  `comparison.json`. Game archive and linked shaders are unchanged; Old 80 MiB /
  New 124 MiB CIA profiles are verified. The isolated builder restored the
  worktree. This build includes the earlier redundant-pack-read removal.

This candidate is **not deployed or emulator-screen validated**. The user's
console is converting with the previous deployed build; it has not been replaced.
Recipe source keys change, so existing journals must not be relabelled to avoid
reconversion. The currently running complete conversion check deliberately keeps
the deployed binary and its genuine checkpoints. Full conversion, hardware timing
and post-conversion gameplay remain separate acceptance requirements.
