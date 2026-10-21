# Direct particle parameter publication

Local candidate 8942dc4929da, based on deployed88915fc75528. Not deployed.
The actual palette-build profile attributes1024558 ARM encounters to drawGlow,
including81073 at the particle aggregate assignment and247278 without a source
line. The generated source MD5 e220bcc2b995d0ce5e6cc89e0253b919 matches exactly.
These are encounters, not cycles; the aggregate assignment is a hypothesis for
avoidable temporary packing, not a measured memcpy cost by itself.

The new adapter replaces the aggregate assignment with direct member writes
into the existing unpublished particle lease or local dynamic/discard value.
It keeps conversions, arithmetic, field ordering and all36bytes unchanged.
No sorting, caching, geometry omission, RNG, animation, physics, source-alpha,
shader or stream publication changes. Pinned Melee command_cache.c was consulted;
existing explicit flushes/fences are untouched. No new allocation or worker.

Actual generated old/new drawGlow functions pass30000 comparisons under
ASan/UBSan with stub native/camera callbacks: ordered packets, RNG, slots,
metadata and cleanup match. The initial wrong-UV mutant escaped because the
fixture used equal UV corners. The fixture now uses distinct UVs; the mutant
fails. This repairs test coverage, not a game defect. Original generated source
is separately retained; only the packing block changes in candidate source.

The isolated whole-app builder verifies the generated translation unit hash
against that test. Only the particle generator changes in the source snapshot.
Of794 object members, only expgfx.o changes; the archive symbol index changes
accordingly. All GPU programs, Old80/New124 and setup recipe remain unchanged;
root/archive restoration passes. Unrelated root WIP is not a build input.

Tools: particle_direct_pack_patch.py, test_particle_direct_pack.py,
build_particle_direct_pack.py, verify_particle_direct_pack.py,
profile_particle_direct_pack.py. Evidence in particle-direct-pack-test and
unified-particle-direct-pack-v1-console-setup-builds. Actual workload comparison
and physical performance are separate gates. No speedup or full-parity claim.

## Rejected workload result

The exact candidate completed the Old palace route in121.69hostseconds with
state/save/drain/owned-exit checks and no missing actors. Inspected capture
retains world, purple creature/particles and HUD; camera differs from parent.
The raw unsupported core-ID telemetry failure is retained, with independent
exact-ELF execution proving audio core1. Host timing is excluded.

drawGlow encounters1024558->1191142; main16017300->16187815,
core16099321->5963737. Activity/phase differs; no attributable whole-frame
timing conclusion. The experiment does not demonstrate a saving and is rejected
for promotion. No deployment/hardware request. Memory is recorded in comparison.
Keep deployed88915fc75528 as the baseline, not this candidate.
