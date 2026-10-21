11 October New223 deployment complete: normal `626d217324e9` / CIA
`762ef1732702`, replay `9bcad101a4fe` / CIA `9acc42b6e738`, with matching
original-geometry22-slot pack and lossless cache. All18 changed payloads staged,
byte-readback verified, renamed and final-size checked. Saves/preferences
preserved; Old229 unchanged. CIA requires FBI installation; matching3dsx ready.
Performance-geometry22-slot pack remains local; original-asset installer does
not replace the generic performance pack. Earlier status entries are historical.
See [changes, tests and unresolved issues](RENDERER_UPDATE_20261011.md).

# Native actor palette regrouping, 11 October

Local assets only; nothing in this document updates the installed console packs.
The source reconstruction has22 palette slots in its native vertex shader, yet
both deployed actor asset manifests still request11-slot groups inherited from
the old separate position/normal palette architecture. Native normals already
use the position rows. Preparation can therefore combine consecutive compatible
source sections up to22 bones, without per-frame merging or another renderer.

`rebuild_native_scene_topology.py --model-bones 22` recompiles actor packets.
An optional --model-ids list bounded the first fur experiment. Source identity,
source triangle order, material/format boundaries, hardware vertex/index limits,
bone weights and position/morph references remain mandatory. World packets are
unchanged. Ordinary model plans, sparse morph writes, shadows and fur all consume
the same prepared representation. No atlas, texture reduction or simulation edit.
The original packs and baseline builds are retained.

`compare_native_actor_packets.py` independently walks both indexed packet sets.
For every ordered corner it hashes position, normal, colour, bothUV and morph-
reference bytes plus resolved global bone IDs/weights; differing local palette
IDs and unused/deduplicated stored vertices are permitted. All corners remain
exact. Packet directories/payload CRCs and source identities are checked first.
This is stronger than comparing aggregate counts, but does not establish raster
or hardware cache correctness.

Performance asset candidate `actor-palette22-v1/native-scene.bin`:

| Quantity, across81changed actor packets | Before | After |
|---|---:|---:|
| Draw groups |716|431|
| Fur draw groups |66|19|
| Stored vertices |151849|146665|
| Ordered indices |497712|497712|
| GPU payload bytes |7073872|6864322|

All835actor packets were recompiled;754are byte-identical. Including world
packets,2021of2102packets are unchanged. Full pack42497636B,
SHA63222ea8734c4140a266575f0b9c78186e4f2e9cb4d1915f16da2ad24f511e84.
The original-geometry counterpart is now separately repacked and checked below.
Never pair a cache with a different scene SHA. Neither new asset pack is deployed.

The first10-model fur pack already passes New and Old CIA-content cockpit runs,
including live morph work and source-density fur. Fox/Krystal three fur groups
become one, retaining all triangles. New scene+submission CPU drops about0.85ms
at full density, but whole fur still adds0.615ms versusOFF and fails its0.5ms New
budget. This is not an established physical speedup. See
[NATIVE_FUR_SOURCE_LAYERS.md](NATIVE_FUR_SOURCE_LAYERS.md) for exact results.

General gameplay before/after validation and physical-console acceptance are
separate requirements; record them before changing deployment defaults. Neither
palette regrouping nor restored shell count claims to fix the reported white
Arwing, disappearing trees, day/night parity or missing indirect fur wind.

## General gameplay validation

Same3ddbd8395a16binary and cave-idle recipe,11-pack before vs all-actor22-pack
after: both pass state/save/drain/owned-exit,48.22/47.69hostseconds. After1410
was inspected: Fox, ladder, mushrooms, tunnel, ground and HUD retained.
Scene CPU1.974386->1.996747ms, submission5.435932->5.239196ms, combined gain
0.174374ms. Both~59.83122paced Azahar FPS. Pose/counts differ slightly (whole
world indices30887925->30797889), so this is a short relative observation, not
an exact frame A/B or a physical speedup. Fur is OFF in this general-scene test.

Heap48,122,320->48,110,928 /71,303,168B; minimum linear free18,804,352->18,724,480
/25,165,824. Again, aggregate pack savings do not guarantee scene residency
savings. VRAM4,846,592, measured stack72,788/524,288 and process103,186,432/
130,023,424 unchanged. Actual native plan audit accepts all835actors with1609
draws,984036indices,7515negative cases,7323palette restart checks,359fur slots
and4000world-space cases (maximum error3.142e-5). No rejected model.

Evidence: actor-palette22-test/runtime-comparison.json and plan/summary.json;
fur-source-layers-test/all-palette22-equivalence.json. No Old all-actor scene
run or physical validation yet; the earlier Old runtime covers the10-model pack.

## Original geometry, independent validation

`original-palette22-v1/scene/native-scene.bin` is51,010,084B,
SHAc5ab2aaa1cfdbda2e375eac856b67afdf4d088da07bd957f1a1338415350b19a.
The81changed original actors retain497880ordered indices, with groups716->431,
fur groups66->19, vertices151881->146698 and GPU bytes7075528->6865938.
All positions, normals, colours, both UVs, global bone weights and morph
references remain exact. Other2021packets are byte-identical. The original
topology remains empty; no triangle decimation was introduced.

Matching cache22,875,803B,
SHAb0965c21d702dea65aafa991fff3d69808070739628dd0e098fcd3123b024f6c;
its source identity matches the new pack. The native compiler audit accepts
835models,1609draws,988530indices with no rejected model;7515negative cases,
7323palette restart checks,359fur slots and4000pose comparisons pass.

Serial New/Old original-geometry Arwing routes pass state/save/drain/owned-exit
in67.39/57.71hostseconds. Both1650images inspected: Fox, tree trunks/leaves,
Arwing panel textures/blue wings, ground and HUD are visible. The Old touch
item pad is present. These captures still do not reproduce the user's physical
white Arwing or missing-tree report and are not evidence those bugs are fixed.

New scene2.001037ms, submit3.710853ms versus preceding cache build
2.009743/4.221169ms: combined CPU reduction0.519022ms. FPS54.12116 versus
54.22095, effectively unchanged. This compares the same recipe but different
animation observations and the newer shell-batching binary, not an exact
single-change A/B. Old47.57969AzaharFPS, scene1.973723/submit3.698150ms;
no matched Old baseline or physical FPS claim. Reduced fur remains selected.
New1688shells=1688draws; Old1526=1526, both zero unsupported fur.

New heap56,155,232/71,303,168B, minimum linear free16,660,224/25,165,824.
Old heap41,474,608/50,331,648B, minimum linear free1,971,968/10,485,760;
the tighter Old margin remains an explicit constraint on future features.
VRAM free4,846,592; measured stack72,860/524,288. ProcessNew103,186,432/
130,023,424 andOld67,534,848/83,886,080. New cache uses the existing7MiBwindow:
432logical reads/3,292,556B/127decodes, zero scene SD reads and no pressure
release. Old has no scene RAM cache. This does not prove all FS stalls fixed.

Evidence: original-palette22-v1/validation.json,
original-palette22-test/runtime-comparison.json and
actor-palette22-test/{original-equivalence.json,original-plan/summary.json}.
