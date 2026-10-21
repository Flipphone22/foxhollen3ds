# Save-load / Krystal transition GPU hang

The New 3DS capture in `artifacts/console-visual/20261013-114932-223`
matches installed normal setup build `11b2c703d94d` (3dsx `4ccf98aafa10`).
The main thread reports a GPU wait exceeding three seconds at retrace 1164;
the GPU observer reports work frame 1154, 5457 submitted commands, 5454
completed and three pending. Audio keeps mixing. This proves a queue that
did not complete during the observation, not a particular failing instruction.
CPU input snapshots do not establish GPU cache coherence or contain VRAM.

`acquire()` cannot reuse old frame resources until `C3D_FrameBegin` succeeds.
Its timeout currently records diagnostics and keeps waiting. It does not
advance gameplay/input or implement GPU recovery. Freeing pending resources
or treating the timeout as completion would be unsafe. HOME's failure to
respond is reported by the user; the capture does not locate its blocker.

The captured shadow receiver draws 223–228 and 233 describe five short3
attributes: 30 bytes, with a 32-byte stride. The existing hardware-contract
checker rejects all seven. The point structure contains fifteen signed shorts
and a final reserved short. The repair describes the last attribute as short4,
fetching the existing padding; all live xyz offsets, vertex storage, primitive
counts, shader code and resource ownership stay unchanged. The shader only
uses the fifth input's xyz. This is a definite layout defect and a strong hang
candidate, not yet a physically confirmed explanation of every reported hang.

Pinned Melee `early_queue.c` keeps submitted command/resource memory alive
through completion, including capacity waits. That ownership rule is retained.
Pinned Citro3D `attribs.c` supplies the format publication; no raw register
writes or CPU rendering substitute are added. The factory in
`pica_shadow_points_integration.py` also receives the correction.

Local evidence:

- Captured fetch regression: seven rejected originals, seven repaired layouts,
  13,278 point records / 398,340 live bytes unchanged; no extra vertex storage.
- Actual ARM SDK binding test: 361,318 checks, all five existing invalidation
  mutants rejected. The test now checks fetch extent against stride as well
  as comparing prepared and ordinary SDK bindings.
- Actual receiver vertex/geometry shader interpreter: 32,768 cases, arbitrary
  signed padding in v4.w leaves outputs identical; 325,584 admitted output
  lanes still match the reference projector. Existing shader mutants rejected.
- Candidate normal build `912b6c79f861`, 3dsx `6027e2e19fc2`, CIA
  `1e62c669893b`, shared Old80/New124 profiles. Game archive and every embedded
  shader match the deployed build. ISO conversion recipes are unchanged.

Build evidence is under `shadow-fetch-v1-console-setup-builds`. Its initial
strict archive check rejected an extra **unimported** blur adapter included by
the build system's source-inventory glob. The completed binaries were then
independently archived with exact source hashes, unchanged game archive and
shader comparisons, and the incidental source explicitly recorded in
`scope.json`. The unrelated blur implementation is not linked or deployed.
Future isolated builds shelve untracked adapter inventory entries too.

The first save-load diagnostic attempt did not launch a game: its emulator
identity check caught a diagnostic-emulator replacement in progress. Failure
evidence is retained. The updated diagnostic emulator supports both the
startup-menu and game input counters; source and baseline emulator restored.
The second attempt reached Play Game but its last A press had arrived too early;
its 120-second bound expired before loading. With the input corrected, the
exact deployed app fails the diagnostic at draw 79,996 / combined input counter
1495: loader 0, five components, extent30/stride32, 2118 vertices. That vertex
count and layout match physical captured draw223. See
`artifacts/native/shadow-fetch-before-save-new-v3/pica-layout.json`. The owned
emulator exited and assets/saves stayed unchanged. This reproduces the hardware
contract violation on PC; it does not emulate the physical lockup mechanism.
The corrected normal app passes the matching sequence in
`artifacts/native/shadow-fetch-after-save-new-v3`: loaded images at counters
1600 and 1750 inspected, 167,936 audited draws reported before shutdown (a
periodic lower bound), clean Select exit, zero queued log records/errors,
owned emulator stopped, and all copied saves/asset bytes unchanged. Runtime
was 182.32 host seconds under the 240-second diagnostic limit. No throughput
comparison: the strict diagnostic and unrelated host workload affect speed.
The ship-fight transition was not separately replayed this turn. Hardware
recovery remains unconfirmed.

Correction (13 October): the panel reports USED linear6795/24576KiB and
VRAM1531/6144KiB, not free space. The earlier claim that its values proved
missed allocation pressure was incorrect. Logs report free space and agree
with these figures. Samples are still not continuous high-water marks. This
change adds no allocations.

New223 app-only deployment is **complete**:
`artifacts/console-test/20261013-122455-new223/transfer.json`.
All six payloads were staged, fully byte-readback verified, renamed and checked
for final size. Saves, graphics settings, ISO and prepared assets were untouched.
Prior GPU dumps were renamed to dated backups; local captures remain intact.

Launch `SD:/3ds/foxhollow/foxhollow.3dsx` in Homebrew Launcher, build
`912b6c79f861`. The matching CIA is `SD:/3ds/foxhollow/foxhollow.cia` and needs
FBI installation to update a HOME icon. No Old console or replay app update.
This is a tested correction for the captured defect, not yet confirmation of
physical-console recovery or all cutscene transitions.
