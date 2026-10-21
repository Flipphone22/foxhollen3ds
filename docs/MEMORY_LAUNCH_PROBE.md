# Old 3DS 96 MiB launch diagnosis

This is a separate diagnostic, not an FPS improvement or replacement game build.
Its CIA title ID is `000400000F484E00` (FH Memory Probe). It requests Old96/New124
using the same access profile and verified launch artwork as Foxhollow. Its
initial heaps are only 2 MiB normal + 1 MiB linear; stack reservation is 512 KiB.
It does not open game assets or saves, or change firmware/configuration.

On Old 3DS, first launch `/3ds/foxhollow-memory-probe/foxhollow-memory-probe.3dsx`
from Homebrew Launcher. This checks that the small diagnostic and SD logging
work through your existing launch environment. Press A when the result appears.
Then use FBI to install the adjacent `foxhollow-memory-probe.cia`, and launch
**FH Memory Probe** from HOME Menu. Press A when results appear. Reopen ftpd.
If it crashes, keep the crash's process/error information and save its dump when
offered; reopen ftpd after recovery. No gameplay replay is needed for this test.

Files are under `/3ds/foxhollow-memory-probe/`: `hbl-boot.log`, `hbl-result.json`,
`cia-boot.log`, `cia-result.json`. Boot records append; result files contain the
last completed run. A previous result does not prove the next launch succeeded.
The boot log marks entry with filesystem ready, before/after APT, main, mapping,
free and exit. No new marker means it failed before the first successful SD
write; this alone cannot distinguish OS failure, early allocation or SD failure.

The diagnostic uses standard libctru kernel allocation and service APIs. Only
the probe initializes filesystem before APT so early progress can be persisted.
It checks a free virtual range before allocating and page-tests ordinary mapped
RAM, capped at 81 MiB total process commit and 85% of actual kernel entitlement.
81 MiB demonstrates use beyond 80 MiB when the grant permits it; this is not a
test of every byte or of GPU DMA into that memory. Allocated memory is released
before displaying the result. No physical addresses are guessed or remapped.

Headless tests boot the exact CIA's CXI content. Old96 and New124 both reached
81 MiB commit, checked page samples, freed the mapping and wrote results.
They do not test physical HOME Menu reboot/launch. Azahar's 3DSX launch reported
96 MiB and `envIsHomebrew()==false`, so it is not evidence of the physical HBL
grant or environment. Captures and results: `artifacts/diagnostic/memory96-*`.

Build: `tools/build_memory_launch_probe.py` through `tools/linux_exec.sh`.
Test: `tools/run_memory_launch_probe.py --hardware old|new --format cia|3dsx
--label UNIQUE-LABEL` through the same Linux environment.

The documented 96 MiB mode is valid. The earlier full replay CIA failed before
its menu, with no attributable fresh crash dump. That does not establish why it
failed. The SD's `boot.firm` identifies Luma 9.1; the running firmware must still
be established. No firmware update is part of this diagnostic.
