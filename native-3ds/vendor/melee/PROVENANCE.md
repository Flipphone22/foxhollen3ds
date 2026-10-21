# Melee rendering helpers

Unmodified files from `2gifts/melee-3ds`, revision
`c2ebe7c769b3287f2b3748173e8fe5ec24ef408e`, directory
`native-3ds/port/3ds/`: `texture_decode.h`, `texture_storage.h`,
`texture_repack.h`, `texture_visibility.h`, `alpha_test.h`, `scissor.h`,
`blend_state.h`.

Original 3DS integration contributions are provided under GPL version 3.
See `../../licenses/melee-GPL-3.0.txt` and
`../../licenses/melee-LICENSE-SCOPE.md` for the preserved terms and scope.

The Foxhollow wrapper adds extent/input checks and support for its linear RGBA
movie textures. It retains GameCube CMPR interpolation and PICA Morton layout.

`../../include/fh_texture_cmpr16.h` adapts the same pinned `texture_decode.h`
CMPR palette and tile layout for the native renderer's unprepared texture
fallback. RGB is rounded to five bits (maximum channel error 4/255); binary
alpha, transparent RGB interpolation and edge replication are retained.
This deliberate detail reduction halves RGBA8 fallback storage. It remains
GPL-3.0 code; prepared ETC textures and the generic exact decoder are unchanged.

`../../source/efb.c` adapts `renderer.c::efb_copy_cpu_reference` from the same
revision, with explicit extent/capacity checks, a fixed 320-pixel content width,
and a separate caller-controlled source-alpha flag. It remains GPL-3.0 code.

`../../include/fh_efb_rgb565.h` adapts `efb_rgb565.h` from the same revision.
It retains exact CPU-visible RGB565 tiles and repeated-row/pixel reuse, with
Foxhollow's 640x528 logical / 320x264 native content dimensions. Extent checks
remain in the caller; odd-origin half filtering and unaligned destinations use
the original converter. It remains GPL-3.0 code.

`../../include/fh_primitive.h` adapts `../engine/primitive_expand.h` from the
same revision to Foxhollow's two raster colors, eight texture coordinates and
variable viewport size. It remains GPL-3.0 code.

`../../source/fragment.c::fh_texture_pack_fur` adapts the 4x4 CMPR palette
reuse in `texture_decode.h::mp_cmpr_to_native` from the same revision. It
retains per-texel mask sampling, GameCube interpolation, transparent RGB and
PICA Morton layout. Edge padding clamps to the original texture dimensions.
The adaptation remains GPL-3.0 code.

`native-3ds/source/memory_compare.S` adapts the bounded ARM `mp_be_memcmp` routine from `port/engine/libc.c` at the same pinned revision, renamed for native little-endian CPU data. Byte ordering and non-overreading tails are unchanged. `memory_copy.S` is a new bounded copy using the same alignment discipline, without a BE8 bridge.

`../../include/fh_texture_16.h` adapts the 4x4 source-to-Morton mapping and
padding semantics of `texture_repack.h` at the same revision. Aligned RGB565
and IA8 tiles use bounded 32-bit loads/stores and ARM REV16; unaligned sources
retain the imported generic implementation. Partial edges retain exact clamping.
It remains GPL-3.0 code; the imported vendor header is unchanged.
