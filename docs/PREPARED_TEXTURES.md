# Optional native compressed textures

This private companion pack replaces eligible GPU texture uploads with ETC1
or ETC1A4 at the original texture dimensions. The native CMPR fallback now uses
RGB5A1: half the former RGBA8 storage, exact binary alpha and at most 4/255
error per RGB channel. ETC1 uses one quarter of that fallback storage; ETC1A4
uses one half. This is an intentional RGB detail reduction authorized for
the 3DS port. Alpha is checked exactly, including transparent pixels.

The original ISO/RVZ and extracted assets remain unchanged and private.
CPU effect sampling still uses original textures. Texture modes that pack
multiple material inputs, dynamic framebuffer copies, unsupported formats and
dimensions retain their existing paths. This is not a 30/60 FPS claim.

Prepare under the locked Linux SDK environment:

```
python3 /work/tools/prepare_texture_pack.py --maps gamefront warlock . --output /work/assets/GSAP01-rev1/prepared/texture-pack-opening.bin
```

The preparer accepts bounded, validated ZLB texture records from the named
extracted map directories. It reuses the pinned Melee CMPR decoder, including
GameCube interpolation rules, and the locked SDK's high-quality tex3ds encoder.
Only power-of-two dimensions from 8 through 1024 qualify. Decoded visible RGB
RMSE must be at most 14 byte values; rejected records remain original. The JSON
manifest records source and encoder hashes, dimensions, alpha equality, error,
pack hash, and rejections. A preparation subset limit is explicitly recorded.

Install the resulting private file as `/3ds/foxhollow/texture-pack.bin` with
its manifest, using the readback-verified transfer tool's `--texture-pack`
option. Remove or rename that companion file to restore original uploads.
Do not distribute the pack or its source images with the executable.

The renderer allocates a bounded ordinary-heap directory only within its 15%
reserve. Lookup uses CRC and dimensions, then compares every original source
byte; hash collisions cannot admit a different texture. SD reads go through a
4 KiB CPU buffer into an unpublished owned GPU allocation. Payload CRC must
pass before the common GPU cache flush and binding. A corrupt pack is closed,
its candidate discarded, and the original conversion retried. Existing GPU
buffer retirement fences and linear reserve remain in force.

Validation: `tools/test_texture_pack.py` exercises identity collisions, short
reads, malformed directories, payload corruption and output bounds under
ASan/UBSan. `tools/test_graphics_device.py --texture-only --texture-pack ...`
compares a small pack containing both ETC1 and ETC1A4 against decoded RGBA
reference images with nearest and linear filtering in headless Azahar.
This does not establish physical GPU correctness or SD cost.

The first menu run (`artifacts/native/texture-pack-menu`) admitted 39 uploads,
avoided 3,409,920 RGBA upload bytes, and left roughly 3.4 MB more linear RAM
free. Matching menu frame times did not improve meaningfully. Extra SD reads
were measured; physical SD overhead remains unmeasured. Default runtime use
requires the explicitly installed optional pack.

## Opaque surface variants (7 October)

The native renderer also accepts FHTXPK2. Each record stores separate source
and GPU dimensions. The surface lookup chooses a prepared reduced image;
HUD, effects, movie, unlit alternate models and legacy rendering request the
original resolution. Cache identities distinguish these uses. UV coordinates
and scrolling retain the original normalized domain. ETC1A4 images cannot be
resized by this format; source bytes and payload CRC checks remain mandatory.

```
python3 /work/tools/prepare_surface_textures.py --input /work/assets/GSAP01-rev1/prepared/texture-pack-opening.bin --output /work/assets/GSAP01-rev1/prepared/texture-pack-surface-128.bin --max-edge 128
```

This keeps all226 original compressed images byte-for-byte and adds51 opaque
variants with edges capped at128. BOX filtering precedes the pinned high-quality
ETC1 encoder. Encoding RMSE is bounded at14 against the filtered target;
full-resolution detail loss is recorded separately. The private comparison PNG
shows original/filtered examples. Alpha, game assets and simulation are unchanged.
The output is6,230,168bytes, SHA256
`57ddd5296e3c614276e22c1f33750bbba5e1801ef2d1bd7d9309b275a50de253`.
The eligible surface set needs1,106,336GPU bytes instead of2,212,256; this is not
the entire game's texture footprint. CPU identity snapshots retain full sources.

Use this pack only with a build supporting FHTXPK2 (current Newc30a34eb7800 /
Old78ac16ddc582); the previously installed New1d build does not. FHTXPK1 remains
supported. The pack is optional and separate from the required paired scenev9 /
alpha-coverage topology assets. Do not distribute any private pack or montage.
Old exact-console headless Palace now finishes its bound, but still reloads too
many textures and has not been tested on physical Old3DS. See IMPLEMENTATION.
