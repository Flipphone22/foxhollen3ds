# New 3DS renderer update, 10 October

Uploaded and readback-verified to New 3DS new223: normal
`0b2c0f722073`, replay `42d91e75aa0f`. Completion record:
`artifacts/console-settings-update/20261010-125338-223/manifest.json`.
CIA installation through FBI is required to update HOME icons; matching .3dsx
files are ready. Saves and existing graphics preferences were preserved.
Both applications retain shared Old 80 MiB / New 124 MiB CIA profiles.

Changes since the New installation `e783636ad1e8`:

- **Animated silhouette shadows:** SHADOW SHAPE projects the animated mesh
  onto a collision plane rather than drawing a circle. GROUND SHADOW is the
  master switch. Load-time compact shadow geometry and compatible batches reduce
  submission work; collision indexes and exact stationary-query reuse reduce
  receiver cost. This is still a hard, single-plane approximation. Soft edges,
  finite/multiple/moving receivers and self-shadowing remain incomplete.
- **Facial/model morph animation:** sparse live position updates now reach the
  native actor, shadow and hologram lanes. MORPHS can disable them. This does
  not certify every expression, eye texture or facial lighting state.
- **Splash rings and droplets:** native GPU paths behind SPLASHES and WATER
  DROPS, including scene distance fog. Reflection, caustics, wakes and complete
  water parity remain unfinished.
- **Fur:** an optional two-shell native approximation, using resident geometry
  and GPU skinning. Indirect wind/masks and full original shell fidelity remain
  incomplete. FUR is on page 9.
- **Specular highlights:** GPU-generated source light maps and native material
  combiners, behind SPECULAR on page 9. Single-layer base-colour modulation now
  admits 42 of 51 declarations loaded by the palace replay, including two of
  Krystal's three; the old local candidate admitted 12. This count includes
  loaded menu assets, not 51 simultaneously visible materials. Layered/bump
  families remain partial. Opacity-mask support passes compiler/shader/owner
  tests, but the palace run contains zero such draws; rendered coverage remains
  open for that family.
- **Material corrections:** original object damage/tint fades, source-selected
  alpha lighting, and separation of distance-fog and colour-fade ordering.
  These are not a claim that every reported RGB/eye problem is fixed.
- **Less diagnostic SD traffic:** cumulative SD statistics publish at most once
  per wall second and always on shutdown, instead of every frame. Every service
  call still contributes to totals and maximum latency. A 36,000-frame test
  produces 601 rows and retains the injected nine-second maximum. This is a
  traffic reduction, not physical proof that all stalls are fixed.

New costly effects remain OFF by default on both console models. Existing saved
preferences are preserved. These additions use PICA pipelines, persistent assets
and bounded native ownership; they do not reconnect the retired GX renderer.
No new graphics worker was added. New core 2 is already used by audio.

The same-binary New Azahar palace specular comparison is 59.8308 FPS ON and
59.8307 OFF; scene plus submission adds about 0.056 ms. The Old profile ON run
is 58.35 Azahar FPS, not an Old console prediction or a matched OFF comparison.
The final telemetry-reduced replay passes the New palace check; normal startup,
SPECULAR toggling/persistence and Select exit also pass. Captures were inspected.
Shared shaders remain at 511 instructions and normal/replay renderer sources
and game archive match. Memory reserves remain within the established limits.
These short checks do not establish whole-game visual parity, hardware safety
of every optional effect, or sustained console 30/60 FPS.

Physical pre-update I/O evidence is in
`artifacts/console-visual/20261010-124716-223`. Three late observation gaps of
5.90–7.54 seconds coincide with completed 8.84–9.03-second write calls, with no
reads in those intervals. Asynchronous calls can span multiple observation
windows; their durations are not direct main-thread CPU times. Other gaps
coincide with asset reads/save metadata. A separate 74-second gap has no
completed IPC and cannot be attributed to the SD card from these counters.

READ IPC is summed read-call wall time since the previous performance-panel
refresh. FS IPC includes reads, writes, flushes and metadata. Do not add the two.
READ MAX is the session maximum single read; the refresh window can exceed a
second during a stall. Concurrent worker spans may overlap.

Evidence: `specular-runtime-test/runtime-comparison.json`,
`specular-deploy-test/build-verification.json`, `sd-observation-test/result.json`,
and the feature-specific native-rendering documents in this directory.
