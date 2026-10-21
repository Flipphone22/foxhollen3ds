# Native actor and particle shared program bank

Local candidate, not deployed or performance-accepted.

The Old palace trace found 184 program-configuration copies totalling 369,216
bytes across eight frames. This is copy volume, not attributed CPU time. Actors
used the ambient/general bank while static particles used the lifetime bank.

The proposed shared bank contains the original general actor, ambient actor,
HUD, lifetime static-particle VSH and particle GS. It occupies 501/512 PICA
instructions and 100/128 descriptors. The linker verifies relocated operation
equivalence and unchanged per-entry tables. The lifetime static VSH passes the
existing 1,478,350 original-C cases (1,628 one-byte differences, no greater
error and no opaque/discard mismatch). These checks do not prove GPU handoff
correctness or full visual parity.

`particle_hot_bank_patch.py` changes four archived inputs. Ambient and ordinary
actor instances, HUD and static particles reference one DVLP allocation.
Dynamic particles retain their previous VSH and GS bank through explicit entry
arguments. Cloud, UI models, uncommon effects, simulation and source geometry
are unchanged. Existing code banks remain byte-identical; the hot bank is added.
This uses additional bounded shader/program metadata, to be measured in the
integrated runtime; it does not add vertex buffers or new frame allocations.

Pinned Melee command-cache publication and uniform ownership were consulted.
The existing GS-to-VSH boundary, uniform invalidation, SDK command flushing and
retirement are retained. Do not remove these to manufacture faster transitions.
The previous resident command-leaf experiment is not being reintroduced.

Build with `tools/build_particle_hot_bank.py console-setup --revision v1` in the
existing Linux toolchain wrapper. It builds from the archived lifetime candidate
and restores unrelated root work and the game archive. Profile with
`tools/profile_particle_hot_bank.py --system-core --memory-calls`.
The profile excludes timing claims; same scene activity and actual code-upload
counts must be reviewed before claiming any full-frame gain.

## Integrated result

17 October NATIVE ACTOR/PARTICLE HOT BANK; goal ACTIVE/INCOMPLETE.
Previous goal turn PROGRESS(memory trace). This turn PROGRESS(native bank owner
replacement, isolated shared build, exact linked checks, matched Old profile).
Full objective read. No FTP/deployment/save/assets/preferences changes.
Latest local normal586b9c1971dc,3dsx13d6e1069f4c,CIAe6a5bc101a3d NOTDEPLOYED.
Old SD c8acfac0d348 unchanged; physical6d78487 baseline23.0011FPS/43.4762ms.

New particle_hot_bank_patch.py replaces four archived inputs: ordinary/ambient
actor instances, HUD/static particles share one DVLP; explicit particle init
entries retain dynamic VSH/GS in previous bank. No simulation/geometry/material
work omitted. Common bank501/512instructions100/128descriptors. Existing19
banks byte-identical plusone; gamearchivefdf936af8aa6 unchanged; Old80/New124,
setuprecipe,root+gamearchive restoration PASS. GS-to-VSH split/uniform cache
invalidations/publication/retirement unchanged. PinnedMeleecommand_cache and
uniform_upload consulted. No GPU command chaining restored.

Static overlap audit exactELF rejected generic partial-upload approach: major
banks share few same-address words. Different descriptor relocation matters.
Instead constructed common native bank preserving original operations/tables.
ExistingparticleVM1478350casesPASS1628onebyte differences,0larger/opaque errors.
Exactlinked hotbankSHAf3b11fb4eb66; all4VSH+GS paths complete/bounded,4emissions.
Broader physical handoff/full visual acceptance NOTestablished.

Old matched memoryprofile166.30hostsec executionPASS/benchmark1/state/save/
drain/ownedexit,0attemptedmissingactors. Capture1600 inspectedKrystal/world/
purpleparticle/HUD; phase differs,notpixelparity. RawSVC11coretelemetry failure
retained; independentmix_onceexecutioncore1verified4848encounters.
Programcopy369216->279360bytes(-24.34%)over8frames,184configcallsunchanged.
Worldpacketcopy262656bytes/144calls unchanged. Mainencounters18062261->17930136
(-.73150%) vsimmediateparent; oldphysicalbuildprofile18206685 gives-1.519%raw.
NOTattributableFPS: label114276->0 andotherUI/audio/particlephase differences.
No fullframe speedup established; shadercopy saving alone insufficientfor30FPS.
Memoryheap41362744/49283072,linearminfree3553152/12582912,VRAMfree4723712,
stack72984/524288,process70660096/83886080. Sampledheadroom,notworstcaseproof.

Toolsbuild_particle_hot_bank/profile_particle_hot_bank/check_particle_hot_linked/
compare_particle_hot_bank_work reproduce. docs/NATIVE_PARTICLE_HOT_BANK.md.
Evidenceparticle-hot-bank-test/{result,patch-scope,linked-check,arm-comparison}.
Rununified-particle-hot-bank-v1-benchmark-old-dsp-core1-memory-profile-v1.
Owned63168audit,8637build,68110linked,9289run,68548audioauditTERMINAL.
Next largerCPU preparation/submission costs; don't claim24%FPS or deploymerely
becausecodeupload shrank. Dynamicruntime coverage/physical performance/fullgoal
remainincomplete. No externalblocker or newhardware request.

