# Sparse click-suppression ring consumption

The eight-frame palace ARM profile identifies 476,091 instruction encounters
at the add/clear/index/loop lines of the existing stop-tail consumer, out of
975,486 in `salCtrlDsp`. This is source-line attribution, not cycles or an FPS
prediction. The DWARF source MD5 is `1e10563c1dd0f0b9c78a08e0917f8390`.
Many channels can have no pending stop tail, yet the old consumer reads, adds
and clears every sample of every selected channel.

The replacement keeps the original ring and tail arithmetic. One occupancy bit
covers 32 samples, with 352 bytes of metadata for all eight studios and eleven
channels. Each nonzero tail writer marks every touched block. The consumer
skips blocks that have never received a pending write; occupied blocks use the
same integer add/clear operations. A partial read conservatively leaves its bit
set. Only consuming a complete block clears the bit. Studio and global reset
clear data and occupancy together. Wraparound and overlapping tails retain
their original behavior. There is no change to voice events, sample clocks,
interpolation, quality, DSP commands or publication.

`audio_depop_sparse_patch.py` applies to the accepted `4e9ce0e40ad2` source
archive, independently of the particle experiment. It changes only the support
source adapter. All six direct ring references are audited: declaration, tail
writer, consumer read/clear, studio reset and global reset. The owning audio
thread and existing synchronization are retained. Pinned Melee audio cache and
publication rules were consulted; this CPU-only metadata introduces no new
DSP-visible data or cache operation.

The test executes the actual new helper, the original consumer loop and the
original `fh_depop_tail`. Thirty thousand randomized overlap/wrap/reset cases
produce identical output samples and remaining rings. A targeted partial read
retains the unconsumed suffix, and complete final drains clear every occupancy
bit. Mutants omitting publication, retiring partial blocks or omitting wrap
are rejected. ASan/UBSan and strict ARM11 compilation pass. This does not execute
the whole audio engine or establish hardware speed.

The isolated shared build verifies the complete generated `hw_pc.c` against
the tested source, preserves the game archive and all 18 GPU programs, retains
Old80/New124 profiles and the installation recipe, and restores the workspace.
Runtime and performance acceptance must additionally include the new helper
cost, actual callback counts and unchanged rendering. No physical gain or
complete audio/gameplay acceptance follows from a helper test alone.

## Integrated result

Shared source `20c5e0b542b5` passes the isolated Old80 palace route, including
state, saves/settings, DSP LLE, drained logs and owned exit. Capture1601 was
inspected. Runtime104.36 host seconds is diagnostic duration, not console timing.
The complete generated source and restored workspace passed verification.

The sampled `salCtrlDsp` body falls from 1,034,476 to 525,726 encounters with
28 entries in both windows. The inlined empty-ring check accounts for 2,464
encounters; no inner ring drain was needed in this window. Active tails are
covered by the host equivalence cases, not a new physical audio recording.
Whole-window encounters fall 22,527,356 to21,640,568 (-3.94%); changing voice
and particle phases mean that whole reduction is not entirely attributable to
this change. Relative to the last physical build's matched emulator sample,
27,651,093 to21,640,568 is -21.74% instruction encounters, not an FPS multiplier.
No new full-frame timing or physical performance result exists.

Heap41859104/50331648, minimum linearfree1622784/10485760, initialized
VRAMfree4723712 and main stack72984/524288 match the parent on this route.
Process commitment69599232/83886080 is4KiB higher. The new352B CPU occupancy
metadata is included; no GPU/linear/texture or audio PCM buffer was added.

The shared ISO-only package is `foxhollow-iso-setup-c7f8028a91a9df55.zip`, with
3dsx `29553a080308` and CIA `f801e1e33611`. Old/New payload identity and stale-CIA
pairing rejection pass. It is ready locally, not deployed. It does not include
the separate early-particle experiment. Full performance, visual parity,
progression/save/endurance and physical validation remain incomplete.
