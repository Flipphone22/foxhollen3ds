# Direct updates to prepared actor materials

Local candidate, not deployed or physically accepted. It builds on the current
collision/sky renderer, excluding the unpromoted rigid-culling and shadow
preflight experiments.

The game adapter previously resolved the same native material separately for
layer information, basic animation/tint, channel selection, alpha raster state,
post callbacks, clouds and environment lighting. The native plan already owns
that material. The new adapter resolves it once, reads the object's live values,
and publishes those values together to the prepared material.

The handle is a synchronous borrow used only inside that material's update.
Nothing caches it across frames, model retirement or worker boundaries. Unused
source material declarations have no ready native target and are skipped. Their
previous setters also made no changes. Texture animation/scroll selection,
constant colours, alpha, channel swaps, environment parameters, cloud state,
post callbacks and removal of stale UI/movie/moon state are retained.

The pinned Melee material-program separation between prepared state and live
inputs was consulted. Its CPU colour evaluator is not introduced here. Native
PICA material binding, descriptors, shaders, command submission, GPU resources
and fences are unchanged. UI and other dedicated adapters retain their existing
setters. No simulation, animation or progression work is skipped.

The change adds no material allocations, GPU storage or worker. A four-byte
publication counter and temporary per-object/per-material values are new. This
is a reduction in CPU publication overhead, not a claim that all actor rendering
or the full frame has become six times faster.

`tools/test_actor_live.py` executes the actual old and new game adapters plus
the native publication functions under ASan/UBSan. It compares complete material
bytes across20,000object updates and100allocation lifecycles, including512,000
ready-material publications. Lookup calls fall from4,058,944to640,000. Missing
callback resets and incorrect channel-selection handling are rejected.

The shared build changes only `objprint_dolphin.o` in the game archive and three
tracked sources. All13GPU programs remain byte-identical. Old80/New124CIA
profiles are retained. Source comparisons and binary identity are not console
performance or complete visual acceptance; runtime results are recorded below.

Shared replay`b4caad32747a`/CIA`ff4672b4ac22` and normal`659f3ca49785`/
CIA`34b92e950a9c` are built locally. Their457sources, game archive and thirteen
linked GPU programs match. No deployment or new normal-app boot is claimed;
unchanged normal UI evidence is reused.

New moving original-geometry Thorntail and Old cave ON replays complete in
74.922and53.569hostseconds respectively. State/save/log-drain/owned-exit checks
pass. New1500/1650and Old1410captures were inspected. World/player/HUD/shadows
remain visible, along with the Arwing and Old touch stick. Source poses differ,
so these are regression captures, not pixel-equivalence proof.

Compared with the corresponding collision-current-v2 runs, actor-render CPU
falls0.084300ms New and0.105329ms Old. The disjoint scene+native-submission+
actor-render sum falls0.104173/0.093627ms. This is a modest local improvement.
Azahar53.589/57.432FPS does not establish console performance.

New heap56566256/71303168bytes, linearfree16033280/25165824, VRAMfree4723712,
stack73068/524288, process103321600/130023424. Old heap40973392/50331648,
linearfree3759104/10485760, same VRAM, stack73008/524288,
process67670016/83886080. Varying heap peaks are not allocation-saving proof.
Keep this update locally; do not spend a console run on a0.1ms improvement alone.
