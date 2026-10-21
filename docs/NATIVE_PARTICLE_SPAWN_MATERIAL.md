# Particle descriptions prepared at creation

The producer now has a candidate that reads a native description from each
particle instead of classifying its blend, shape and camera-facing policy on
every draw. It uses the existing four bytes at offset `0x9c` in the 160-byte
slot. No pool, GPU instance or asset format grows, and no new allocation,
shader, worker or GPU submission path is introduced.

Successful `expgfx_addremove` prepares the description immediately before the
existing final slot flush. Failed creation paths remain unchanged. The stored
word contains raster mode, untextured/unsupported selection, static/dynamic
shape, depth override and orientation policy. Position, camera, source object,
alpha, UVs, colour and template selection remain live. The template bit aliases
impact-position state, so it must not be frozen at creation.

The initial material-write summary was incomplete: its raw inventory included
XOR writes which were omitted from the prose conclusion. Updates can toggle
attraction to the player, attraction to Tricky, and billboard locking. These
three mutation sites now refresh the description in the same slot view. A
fail-closed audit classifies all 27 named flag writes in the actual parent TU;
any other write intersecting the compiled masks requires explicit handling.
Removal clears behavior and invalidates the sequence/active membership; no
description is consumed from that removed slot.

Embedding the word in the reserved tail makes it travel with both direct and
staged pool data. A separate table indexed by pool/slot would risk retaining a
new particle's description when a callback's older snapshot is committed.
The first 156 bytes, including the aliased quad/simulation region, are not
repurposed. Slot size and tail offset are asserted in the actual ARM source.
This uses `memcpy` for the four-byte access rather than type-punning padding.

Evidence so far:

- 30,000 actual producer comparisons under ASan/UBSan: ordered records,
  metadata, RNG, slots and cleanup match. The actual successful-spawn store
  preserves all 156 preceding bytes. Structural mutations and their exact
  refresh statements are exercised. Missing creation/refresh, wrong tail and
  wrong shape mutants fail.
- The actual archived pool-view helpers pass 16,384 modeled updates with
  59,747 callbacks, including cross-pool reuse and snapshot restoration. A
  mutant retaining newer live descriptions beside older staged flags fails.
  This models callback work; it does not execute every game callback.
- Identical-input Clang18 ARM11 fixture: draw self-instructions fall from
  165,155 to 153,127 (-7.28%). Separately measured creation wrappers execute
  108,662 instructions for 3,200 creations. The wrapper is deliberately not
  inlined for measurement. Mutation refresh work is additional. These are
  synthetic counts, not a net frame-time or hardware FPS claim.
- Apart from description publication, the entire generated spawn and update
  function bodies remain byte-identical. The source inventory and pool tests
  are not an exhaustive proof against arbitrary raw-pointer aliases.

The first isolated build was rejected because generator order placed the new
helper on the other side of the existing pool-view helpers. The failed source
is retained, and root/archive restoration passed. Placement was made stable
before `expgfx_initSlotQuad`; the full-TU equality gate was kept intact.

Pinned Melee command-cache publication was consulted. Existing native GPU
owners still control cache flushing, fences and resource lifetimes. The
description is CPU bookkeeping: it is prepared at creation even while particle
drawing is disabled, so an OFF-to-ON toggle needs no per-draw validation/cache.
That small creation cost is not represented as free or eliminated by the toggle.

Tools: `particle_spawn_material_patch.py`, `test_particle_spawn_material.py`,
`test_particle_spawn_material_pool.py`, `test_particle_spawn_material_arm.py`,
`audit_particle_spawn_material.py`, `build_particle_spawn_material.py`.

No deployment or full renderer/performance acceptance is implied by these
checks. Physical save-load validation remains a separate unresolved issue.

## Application record

17 October SPAWN-TIME PARTICLE DESCRIPTORS; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(rejectedcache/sourceinventory); this turn PROGRESS(native
slot descriptor, ownership/mutation fixes, hostARM/sourcegates, sharedbuild/run).
Local83074988c111 unified-particle-spawn-material-v2-console-setup-builds
NOTDEPLOYED; installedOld5052 unchanged, physical88915 26.40484FPS/savecrashOPEN.

Fourreservedbytes atslot0x9c holdnative material/orientation description; slot160
andallpool/instance/buffer sizesunchanged. Createdat successfulspawn beforeflush;
sameword followsstaged/directsnapshotcopies. Livecamera/position/alpha/UV/colour
andtemplatebitremainlive. THREEstructural XORsites refreshinlineinmutatedview.
Importantcorrection: previousinventoryPROSE omittedXORwrites alreadyinrawdata.
Attractplayer/Tricky andbillboardLOCK_B overlapcompiledflags; creation-onlyunsafe.
New failclosedactualTUaudit27writes/3refreshsites. Templatealiasesimpactbit,
neverfreezeit. Originalspawn/updatebodiesbyte-identicalminusnewpublications.

Hostactualstore/draw30000ASanUBSancasesPASS, missingstore/refresh/wrongtail/shape
mutantsreject. Actualpoolhelpers16384updates59747callbacksPASS; stale-live-
descriptor/oldstageddata mutantreject. Scope modeledcallbacks, notallgameDLLs.
Clang18ARM384pairedproducer calls165155->153127(-7.2829%) selfinstructions;
3200creationwrappers108662instructions, plusstructuralrefreshwork. Synthetic
distribution, notnetframecost/FPS; nohiddenzero-costcreationclaim. Descriptors
arepreparedatspawnevenPARTICLESOFFtosupportlaterONwithoutperdrawvalidation.

Onlyexpgfx.o+archiveindexchanges; all18GPUprograms/Old80New124/recipeunchanged.
BSS3266800/data178996 unchanged. PinnedMeleecommand_cache.c reviewed; GPUowners
untouched. Initialv1buildfailedexactTUguard duehelperordering, archivedfailure
andrestorationPASS; stableplacementbeforeinitSlotQuad fixedv2, guardretained.

OneOldpalaceprofile 126.79hostsec executionPASS,state/saves/drain/
ownedexit/missingactors0. Rawfailedcoreplacement retained (profilerSVC11stub);
actualELFexecutionprovescore1audio. Mainwindow16757793->15882630
(-5.2224%); scenealignmentinacceptance.json.
Parent lightning work absent in candidate; UI/RNG/camera phases differ.
DoNOTconvertinstructionratio toFPS/subtractvariablework. Memoryinacceptance;
fullraster/hardware/sustainedperformance/playthrough goalstillINCOMPLETE.
Sourceattribution matches exactDWARF MD5 7f29cc2ac5d6b04a3a9666ce3dcdfa46.
Capture1601 inspected Krystal/platform/sky/HUD; particlesoffcamera, notparity.
Ownedbuild/runtime/audit/sourceanalysis processes terminal.
NoFTP/deployment/hardwareprobe/privateassets/saves/preferenceschanges.
Tools particle_spawn_material_patch,test_particle_spawn_material{,_pool,_arm},
audit/build/profile/verify/compare/record_particle_spawn_material; docs native
spawnmaterial. Source-mutationgates remainneeded; no blanketrenderworkbypass.

