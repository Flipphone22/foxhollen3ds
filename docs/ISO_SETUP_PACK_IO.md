# Original-geometry assembly I/O

The original stage validates every spool packet when it finishes the spool,
then validates every final packed packet before accepting the completed pack.
It also used to decode and validate every spool payload again between those
two checks, while gathering the directory for assembly.

The candidate gathers only bounded, checksummed spool headers at that middle
step. Final output still passes every packet CRC and native-layout check before
`fh_install_artifact_finish` can mark it complete. The initial completed-spool
validation, source fingerprint, transaction CRCs and complete-bundle validation
remain. A changed/corrupt spool can write an incomplete temporary output but
cannot publish a completed pack. Cancellation still retains a recoverable cursor.
There is no gameplay change, new allocation, worker or GPU work.

Pinned Melee `port/3ds/file_io.c` was consulted for bounded reads and avoiding
redundant filesystem work. Its game asset format is not imported here; this is
our startup transaction's immutable intermediate/output validation boundary.

`tools/test_install_pack_io.py` compiles the actual prior-mode and candidate
assembly under ASan/UBSan. Real actor/world packets produce byte-identical output
with and without interruption/resume. Invalid payload CRC, a recomputed CRC over
an invalid native layout, and bad spool-header CRC are rejected. Removing the
final validator makes the corrupt-payload test fail. The changed owner compiles
for ARM. No previous baseline evidence or private source assets are overwritten.

`tools/check_install_pack_io_full.py` reuses those binaries on the entire checked
original spool. Both produce pack SHA256
`3b93a639bf8983af22c5993c101e9168d1c436fe012e827ab2534274f9d3c5ec`,
50,971,512bytes. Actual assembly payload/metadata reads fall from152,920,450to
102,017,220bytes; native packet layout checks fall from4,204to2,102. Thus one
50,903,230-byte payload scan disappears. These are instrumented host counts,
not physical SD-time, total-conversion-time or gameplay-FPS improvements.

Evidence: `artifacts/research/install-pack-io-test/{result,whole}.json`.
Shared candidate3dsx`efe0e2881f4a` / CIA`83b6bb8d6f6c` now builds in
`normal-setup-pack-io-v1-builds`; menu source`a1eb9f22397e`. Its558-source comparison
finds exactly one changed source, `install_geometry_pack.c`. The game archive,
all linked shader programs and Old80/New124CIA profiles are retained; workspace
restoration passes. `comparison.json` records the checks. It is not deployed
or runtime-validated; unchanged menu/renderer evidence is reused rather than
repeating an unrelated boot. The currently deployed87ca5200d925 remains installed.
Its full original-stage ARM recovery check has since passed independently.
