# Guarded common particle updates

The native particle producer still evaluates many general behavior branches per
slot. Live read-only palace samples identified two recurring combinations:
behavior/render `0x20280101/0x822` and `0x80201/0x4040800`. The new specialization
does not assume that a pool's flags remain homogeneous. It checks each slot's
live flags, absence of an impact callback (`impactEffectId == -1`) and a non-null
resource after the original lifetime and phase processing.

The compiler folds a copy of the original update expressions for those admitted
combinations. Potential writes remain writes to the original slot; conditions
outside the admitted combinations use the unchanged general path. No particle,
simulation step, RNG event, callback, bounds update or GPU effect is skipped.
No buffer, cache, thread, GPU command or shader is added. Pinned Melee's producer
snapshot ownership guidance remains applicable; existing staged views and
callback isolation are retained rather than replaced with immutable-pool claims.

The complete actual compiled update function is exercised on ARM11 with 33
controlled combinations of flags and occupancy, colored slots and nonzero
velocity. Its own instructions are counted; external dependency models are not
counted as actual game implementations. Slots, pool state, phase globals, RNG
state and external event order match the direct-position parent. Two additional
impact cases mutate a staged slot through an explicit callback. Removing the
impact guard fails that comparison, confirming that constant flags cannot be
carried through arbitrary callbacks. These are controlled fixtures, not captured
complete game states or proof of every callback interaction.

With 25 slots, the first type falls from 8,640 to 6,421 update instructions
(-25.68%); the second from 7,665 to 5,571 (-27.32%). Noneligible full-pool cases
regress by up to 456 instructions in the fixture. The function grows from 9,592
to 10,176 bytes. Neither these counts nor the observed residence histogram
establish a whole-frame FPS gain or account for physical instruction-cache cost.
The single-type predecessor is retained under `particle-common-update-test/`.

Local shared normal **df5668002cdc** builds from direct-position **0dc0f5fac88a**.
Only the archived generator changes; the complete generated translation unit
matches the tested candidate SHA256
`a8a23def40507adec527c97a7e903bcaff722fb496f6f53a1dc1fd0c73f07330`.
All GPU programs, Old80/New124 CIA profiles and installation recipe are
unchanged. Selected workspace files and game archive were restored. It is not
deployed; Old4f58/Newd478 remain installed/uploaded as previously recorded.

Tools: `particle_common_update.py`, `compile_particle_common_update.py`,
`test_particle_common_update_arm.py`, `test_particle_common_callback_guard.py`,
`build_particle_common_update.py console-setup --revision v2`,
`run_particle_common_update.py`, and `compare_particle_common_update.py`.
Evidence directories: `particle-common-update-v2-test/`,
`particle-common-callback-mutant-test/`, and
`particle-common-update-v2-console-setup-builds/` under the native research root.

## Integrated palace evidence

The Old80 profile run completed all benchmark steps, preserved saves/settings,
drained logs and exited. The palace capture was inspected: Krystal, world
textures, purple creatures, particles and HUD are present. This is limited
integration coverage, not full rendering parity or endurance acceptance.

The launched CXI SHA256 is
`6b3786f1727a76e4975dc6a67531b702bce559ce19339db3f7c3f6c8285e6354`;
its run ELF is byte-identical to the archived ELF. The summary's generic
`binary_sha256` identifies this CXI, not the 3dsx. The 3dsx SHA256 is
`af9425047ff723174e6a67fce43fe7ba006005b4873956fa38c1a299a7fa0d2f`.
`verify_particle_common_update.py` confirms only `expgfx.o` changed and its
10,176-byte update instruction section exactly matches the ARM fixture object.

In the existing matched eight-update window, main-core instruction encounters
fell 15,243,960 to 14,846,949 (-2.604%). Particle update fell 1,510,204 to
1,147,856 (-23.99%). Major update/render entry counts match. Other particle
activity differs: drawing falls 12,460 encounters, while spawn/add-remove work
increases. Do not attribute every whole-window difference to the patch or
convert the count ratio into physical FPS. Instrumented host elapsed time of
155.42 seconds is excluded from timing comparisons, as is the captured overlay.

Observed heap peak remains 41,366,136/49,283,072 bytes; linear minimum free
3,667,840/12,582,912; initialized VRAM free 4,723,712; main stack peak
73,008/524,288; process commitment 70,713,344/83,886,080. This is sampled palace
headroom, not a full-game maximum. Missing actor attempts, logger errors and
backpressure waits are zero. No new physical measurement or deployment exists.

Runtime evidence: `artifacts/native/particle-common-update-v2-palace-old-profile/`;
inclusive comparison: `particle-common-update-v2-test/runtime-comparison.json`.
The 30/60 FPS targets, complete visual parity and final progression/endurance
checks remain unmet. This is a bounded improvement to retain while pursuing
larger remaining costs, not a completion condition.
