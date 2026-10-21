16 October local candidate 0d6f83a3f5d5: DSP-default audio now processes common
reverb directly at the DSP rate, preserving software sends and existing output
alignment. Delay lengths and decay are retuned; audio is an authorized close
approximation. Old palace check passed with zero source/callback audio faults.
Whole sampled ARM work is12.7% below the last physical build's matching profile,
not a measured FPS gain. Shared Old80/New124 CIA and3dsx; assets, conversion and
saves unchanged. NOT DEPLOYED. See NATIVE_DSP_RATE_EFFECTS.md for scope/limits.

16 October local candidate c1640051d32a keeps DSP audio DEFAULT on both models,
with CPU comparison in the existing startup menu. Includes retained collision
queries, fused raster snapshots and prepared shader configurations. Old palace
runtime passed; no physical FPS gain established. Shared Old80/New124 CIA and
3dsx, unchanged conversion recipe/assets/saves. Not deployed. See
NATIVE_PROGRAM_PLANS.md for measurements and limitations.

16 October local candidate 9dc28d861d24: DSP audio is the default on Old/New,
with CPU comparison selectable in the existing startup menu. Includes retained
collision queries and fused raster-cache publication. Shared Old80/New124 CIA
and 3dsx; conversion recipe unchanged. Old palace runtime passes, memory bounds
unchanged; no new physical FPS gain established. Not deployed: both FTP checks
timed out before login. Particle batching remains a separate local experiment.

Build 5f50dd9e7bc7 retains DSP-default audio and the startup CPU option.
It also reuses filtered collision triangle lists instead of rescanning all
triangle masks on repeated queries. Geometry, live collision checks and output
order remain unchanged. Added static normal RAM: 16,592 bytes; no added linear
RAM, VRAM, worker or GPU work. Old palace replay passed; collision ARM work
fell 63.9%, total sampled ARM work 4.23% versus the DSP baseline. These are
instruction encounters, not hardware frame times or an FPS prediction.
Deployment is recorded separately. No asset conversion or save changes needed.

Build ee35300a85c5: DSP audio defaults ON for both Play and Benchmarks.
Select the startup Audio row to choose CPU for this launch; the benchmark
picker also uses Start to toggle audio. The choice locks when audio starts
and resets to DSP after relaunch. CPU remains available for comparisons.
Use the existing single Foxhollow installation on both models. No new assets,
saves or conversion are needed. Copying the CIA to SD still requires FBI
installation to update the HOME icon. Physical FPS gain is not established.

# ISO-only startup build (development)

Build e61d43cdbad0 enables **Lighting, Environment and Specular in all four
named presets**, including o3ds-fast. Reselect a preset to apply its updated
choices; existing Custom settings are preserved. Old Fast still omits weather
and ground shadows. This build retains the palace memory repair below.

Build e3052ddc285f addresses the Old 3DS palace texture-allocation panic by
using a 48 MiB normal / 12 MiB linear profile when the actual kernel grant
permits it. Both 15% memory reserves remain enforced; New 3DS is unchanged.
Assets and conversion recipes are unchanged. Physical hallway validation is
still required; this is not a performance-improvement claim.

Earlier build 30ea42e7a564 disabled lighting, environment lighting and weather
in o3ds-fast. The lighting choices are superseded by e61d43cdbad0; weather's
hidden CPU quad/matrix preparation remains disabled when its toggle is off.
Other presets and asset recipes are unchanged, so no reconversion is needed.
See [Old Fast effects](NATIVE_OLD_FAST.md) for validation and limits.
Follow-up fa18b1a85a42 also stops particle appearance preparation when the user
turns PARTICLES off, while preserving simulation and shared RNG calls.

Read the running build ID on the startup menu. The matching SD application
record is `SD:/3ds/foxhollow/build-manifest.json`; copying a newer CIA to SD
does not update the HOME Menu icon until it is installed through FBI.
The setup application contains the startup menu, on-device converters and
native rendering updates. Historical measurements below identify their scope.
Deployment is recorded separately in AGENTS.md and the FTP transfer record.
This is not a release or a claim of complete rendering or target frame rates.

The preceding build's fresh ISO-only conversion finished in **62m58s in New-mode Azahar**, versus
68m49s for the preceding search-hint build. The user accepts under70 minutes;
this run meets that emulator threshold. Physical-console and Old-mode full
conversion times are not established. App-recorded conversion time is62m50.594s;
the observed wall time includes startup/menu overhead. There were no copied
checkpoints or prepared-asset inputs.

All original/reduced geometry, topology, caches and3,754 texture variants match
the checked references byte-for-byte. All1,797 movie frames and audio decode
identically; host/ARM LZ4 choices can differ losslessly. The complete output
verification report is `artifacts/native/setup-readback-fresh-complete-new-v1/`
`conversion-complete-verification.json`. The exact build subsequently passed
an uninterrupted New-mode movie/cockpit check and a short Old-mode CIA-content
startup/cockpit check. Captures were inspected, both exited cleanly and isolated
save sentinels were preserved. These checks are not full gameplay acceptance.

The repaired shadow capture state is preserved. Later additions include optional
reflected splash materials/fog and native temporal motion blur; both are OFF in
all four presets. Motion blur uses the original request/capture ordering, but
visible gameplay blur and physical GPU cost remain unvalidated. See
NATIVE_MOTION_BLUR.md and NATIVE_WATER_REFLECTION.md. Menu actions now have
separate identifiers from effect bits, preserving the original-assets controls.
Converter sources and installation recipe are unchanged; the earlier full
conversion evidence is reused and was not repeated for this renderer update.
The earlier `191072361db3f64b` package remains withdrawn because its shaped
shadows disappear. Do not deploy that package.

The seed contains hashes and two-bit encoder search decisions, not converted
assets. Every texture colour, selector and geometry value comes from the ISO.
Geometry search hints remain an unenabled prototype and are not in this build.
Movie conversion batches durable checkpoints and combines checksum/semantic
readback into one sequential pass while retaining both validations. This
reduces its measured phase from17m37s to11m57s without changing content.

The startup display shows **Overall (estimate)**, elapsed time and rough
**ETA remaining**, separately from the current task's progress. It includes
source checking, both geometry sets, textures, movie conversion and final
validation. Estimates learn each phase separately; they are not calibrated
physical-console or Old-mode durations. The movie prior still predates the
latest speedup, so early estimates can be conservative. Only a validated
published bundle establishes completion.

The native RGB8 movie retains the existing half-size resize, all frames and
audio. Its ARM pack is201,493,568bytes, with the existing asynchronous reader.
The complete conversion retains819,500,082bytes of prepared outputs/checkpoints
in this run, in addition to the original ISO. Older generations take additional
space and are not automatically deleted. Existing conversion recipes remain
compatible with the preceding checked search-hint build.

1. Keep the PAL revision 1 game dump at `SD:/3ds/foxhollow/disc.iso`.
   RVZ is not accepted by this build. The original ISO is opened read-only.
2. Launch `SD:/3ds/foxhollow/foxhollow.3dsx` in Homebrew Launcher, or install
   `SD:/3ds/foxhollow/foxhollow.cia` with FBI and launch its HOME icon.
   Copying a CIA to SD does not update the installed HOME application.
3. If the menu already shows **Assets: Ready** (including a privately deployed
   prepared bundle), skip conversion and choose **Start game**. Otherwise,
   choose **Prepare / resume assets**. The build first verifies the complete
   disc, then prepares original geometry, performance geometry, textures
   (including font atlases), and the opening movie. Overall percentage/ETA are
   estimates; the separate current-task bar reports actual task progress.
   Geometry scans report records as well as banks; see ISO_SETUP_PROGRESS.md.
4. B requests cancellation at the next safe boundary. Completed checkpoints
   remain under `SD:/3ds/foxhollow/prepared`. Resume rechecks the source disc;
   source verification itself currently restarts from the beginning.
5. When **Assets: Ready** appears, select a graphics preset and **Start game**.
   Presets are n3ds-fancy, n3ds-fast, o3ds-fancy, o3ds-fast, and Custom. Custom
   retains your per-effect preferences. The fast geometry converter currently
   reduces very little geometry; its name is not a performance guarantee.

The same 3DSX and CIA select capabilities at runtime. The CIA requests the
existing Old 80 MiB / New 124 MiB memory profiles. Old 3DS should use the CIA
for its established memory grant; a Homebrew Launcher grant can differ.

The standard homebrew audio prerequisite remains `SD:/3ds/dspfirm.cdc`, from
your console. It is separate from game assets and is not included in this
package. The startup menu can run before audio initialization.

The package updates executable/build records and these instructions only.
It contains no game assets, firmware, saves, graphics settings, or preconverted
packs. Existing saves and graphics preferences stay in their current paths.
Existing manually prepared packs do not automatically become committed setup
generations. The Diagnostics page is present but currently has no tests wired in.

The bottom-screen build ID identifies the application actually launched. After
leaving the menu, `SD:/3ds/foxhollow/startup-result.txt` records that ID, the exit
result and LCD retirement. It does not prove complete conversion or gameplay.

Maintainer packaging (an archived, isolated setup build is required):

```
py -3.13 tools/package_console_setup.py --build-dir artifacts/research/normal-setup-shadow-capture-v2-builds/console-setup
```

The resulting development ZIP can be consumed by the existing verified console
transfer tool. Its setup branch preserves graphics settings and does not install
legacy renderer control files or prepared assets. Packaging does not deploy it.
The manifest does not label a reused conversion measurement as a fresh run of
this repaired executable. The prior complete proof and exact source-difference
check remain in their archived evidence directories.


13 October private New223 setup: at the user's request, the checked converter's
runtime assets were copied to the console so gameplay can start immediately.
Six existing geometry files were reused by converter CRC records and current
sizes; textures and movie were uploaded, then the genuine bundle metadata was
published. The FTP record is `artifacts/console-prepared/20261013-new223/transfer.json`.
Large remote payloads received completion/size checks, not a slow full readback.
Launch the current 3DSX and choose **Start game** (expected **Assets: Ready**).
This deployment is not a physical conversion benchmark. Preserve or isolate this
prepared tree before a later fresh ISO conversion measurement. Saves, settings
and ISO were untouched, and no converted data was added to the distributable ZIP.

14 October private Old229 setup: the same checked runtime bundle is now
installed at `SD:/3ds/foxhollow/prepared`. All ten files transferred, bundle
activation published last, saves and graphics settings unchanged. Evidence:
`artifacts/console-prepared/20261014-072204-old229/completion.json`. Choose
**Start game** when **Assets: Ready** appears; no conversion rerun is needed.
This is deployment evidence, not a hardware launch or performance result.

14 October Old229 startup correction: the assets above were present, but the
old setup app rejected the console's complete trimmed ISO (789,640,928 bytes)
because its size differed from the original 1,459,978,240-byte disc. The collected
header matches the local original, its FST CRC matches, and the last file ends
exactly at the trimmed image's end. Startup build `22e57bb425dd` accepts a
complete trimmed ISO with an already-prepared bundle. It still checks the PAL
revision, every file extent, expected FST CRC, bundle identity and child sizes.
Creating a new bundle still requires the original full-disc SHA256; this change
does not rewrite that identity or manufacture conversion records.

The exact shared Old80/New124 build passed a silent Old-mode startup-to-title
check using the genuine converted bundle and a real trimmed copy of the local
disc. Captures show **Assets: Ready**, the opening movie and title scene; Select
exits cleanly. Evidence: `artifacts/native/install-trimmed-ready-old-v1/` and
`artifacts/research/install-trimmed-ready-test/`. This does not establish physical
performance. The prepared-deployment tool now rejects incompatible installed
startup/ISO-size combinations before uploading. For the HOME icon, install the
updated `SD:/3ds/foxhollow/foxhollow.cia` with FBI; copying a CIA to SD does not
update the installed title. The accompanying `.3dsx` can be launched directly.
15 October: regular Old3DS builds add a main-thread millisecond breakdown below
READ MAX with GRAPHICS / PERF ON and the menu closed. SIM/PREP/DRAW/TEX/AUD/LOAD/
FX/UI/GPUW/WRKW/VBL/OTHER/SET/PRES/IN/MET are exclusive one-second averages per rendered frame.
SET/PRES/IN/MET separate frame setup, presentation, input and telemetry from
OTHER. Clean exit saves the 35 phase totals and last displayed window to the
existing native log, without extra gameplay-time SD writes.
Waits and SD IPC are elapsed time, not CPU utilization; do not add overlapping
GPU or filesystem measurements. PERF OFF stops the new timing samples.
See CPU_PERF_PANEL.md for scope and limitations. New3DS keeps its recording UI.
