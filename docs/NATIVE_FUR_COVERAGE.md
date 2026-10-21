# Native fur coverage mask

The original `objFuzzRenderCb` samples `Shader.indTexture` at base UV coordinates
and selects its S/alpha component as `GX_ALPHA_BUMPN`. Final shell alpha is
`clamp(noiseAlpha * coverage + objectAlpha - 1)`. Previously the native path used
coverage=1 everywhere, leaving fur visible on regions the source mask excludes.

The new load adapter acquires that original image once through the bounded native
image pool. It uses the same immutable image publication, refcount and retirement
rules as other native materials. No simulation, texture source data, mesh, extra
geometry pass, worker or CPU vertex generation is introduced. The image is
resident even while FUR is OFF; toggling does not perform an SD read.

The existing draw binds noise to texture0/UV0, base to texture1/UV1, and coverage
to texture2 sharing UV1. This keeps UV2 for distance fog. Six precompiled material
variants cover lit/unlit and no mask/alpha mask/intensity mask. I4/I8 native
textures supply coverage from red because their PICA storage has implicit alpha.
The stage1 product replaces the old unmasked alpha input, followed by the same
object-alpha subtraction, noise color perturbation, fade and fog. Source masks
with different padded extents from the base are explicitly unsupported rather
than stretched; runtime counts those rejected fur draws.

The cached SDK third-sampler coordinate owner preserves sampler/procedural texture
bits and publishes its dirty flag. Fur restores its routing on every bind; other
owners already restore theirs. This follows the pinned Melee binding-reset and
write-site publication discipline, with Foxhollow's existing SDK state owner.
There are no direct register writes bypassing the SDK cache.

The source coverage normalizes five significant bits. The candidate samples the
full eight-bit channel, with a bounded alpha difference of 7/255; this is an
explicit approximation. Source indirect wind/UV distortion, projected shell
shadows, layered/masked fur materials and full visual parity remain incomplete.
This does not establish that the user's remaining fur flicker is fixed.

## Validation

- Actual native material code: 425,984 coverage/channel/quantization comparisons,
  262,144 unmasked-alpha cases, 65,536 source noise-RGB cases, 1,024 RGB/UV cases,
  and 1,000 state handoffs. Missing mask, wrong intensity channel, delayed-buffer,
  complement and stale-buffer mutations are rejected. ASan/UBSan pass.
- Actual load/release code: 1,000 lifecycles, image/allocation rollback, duplicate
  rejection and refcount/byte accounting. Missing release mutation is rejected.
- Exact linked Azahar float24 shader: 32,768 ordinary outputs unchanged, 32,768
  fur outputs retain all other lanes while swapping UV0/UV1. Normalized UV error
  at most 3.8147e-6. Linked program remains 511 instructions.
- Serial original-geometry/original-texture cockpit replays on New/Old Azahar
  pass state checks, save preservation, log drain and owned exit. Captures900
  inspected; Fox, cockpit, HUD and touch controls remain present. These do not
  prove hardware depth stability or original-image parity.
- Candidate New 4,344 / Old 4,350 masked fur draws, zero unsupported fur draws.

| Azahar cockpit | Packed-morph baseline | With coverage |
|---|---:|---:|
| New scene CPU, ms | 0.773008 | 0.777170 |
| New native submission, ms | 1.935936 | 1.942704 |
| Old scene CPU, ms | 0.771758 | 0.777587 |
| Old native submission, ms | 1.939375 | 1.943296 |

Combined rendering CPU increases are 0.010931 ms New / 0.009750 ms Old, within
the 0.20/0.50 ms development gates. Minor animation differences remain, so these
are short relative checks, not console predictions. All runs are near59.83
emulator FPS. One added texture sample per masked fragment still needs physical
GPU-cost validation.

Added linear allocation in this scene is16,384 bytes, heap peak +832 bytes.
New minimum linear free18,484,480/25,165,824, Old3,804,416/10,485,760 bytes.
New heap47,855,016/71,303,168, Old40,514,984/50,331,648; VRAMfree4,846,592;
stack72,844/524,288. Process commit remains103,186,432/130,023,424 New and
67,534,848/83,886,080 Old. No budget increased.

## Builds and scope

Replay `50e46003c21e` / CIA `c640fadb0d2f`, normal `c70bf119b342` /
CIA `ad9946fc1c56` are built **locally, not deployed**. Both share the tested
game archive,66 renderer sources and exact shader `f670fde7b7fd…`; Old80/New124
CIA profiles are verified. Normal UI behavior is unchanged; no separate normal
runtime test was repeated for this material-only change.

Evidence: `native-fur-mask-material-test`, `fur-mask-owner-test`,
`fur-mask-v1-builds-shader-test`, `fur-mask-runtime-test` under
`artifacts/research/pica-scene-rewrite`, and `artifacts/native/fur-mask-v1-after-*`.
The stable source/archive snapshot is `fur-mask-v1-builds`. Six unrelated VRAM
work files were restored after each build. The mask patch is already applied to
both this stable snapshot and active VRAM work; do not apply it twice.

New console still has `913f01dfaa2a` for the isolated morph-layout confirmation.
No remote files, saves, assets or settings were modified during coverage work.
