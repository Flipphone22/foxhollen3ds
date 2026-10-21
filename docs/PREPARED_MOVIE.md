# Private prepared opening movie

The native port still reads game files from the private `disc.iso`. The optional
`starfox-pica.thp` beside it replaces only `/starfox.thp` through the DVD layer.
It is a derived asset, not a ROM bundled with the executable. Keep it private.
The original RVZ, ISO and extracted THP remain unchanged.

From the configured Linux build environment, run:

```
python3 /work/tools/prepare_movie_batch.py --jobs 2
python3 /work/tools/check_prepared_movie.py /work/assets/GSAP01-rev1/prepared/starfox-pica-half.thp --out /work/artifacts/movie/full-half-validation --frames 59,66,67
```

The batch preparation uses the pinned SDK tex3ds encoder, verifies atlas output
against independent frame encodes, and reuses its private frame cache. Its
256x192 video is downsampled from 512x384 with Lanczos, padded to 256x256 and
encoded as ETC1. ETC1 and downsampling introduce visual differences. A lossless
zlib wrapper reduces SD traffic. All 1,797 frames, original timestamps/frame
rate and original THP audio payloads are retained. Decoding needs 32 KiB per
owned video buffer, plus the renderer's native texture storage.

For the optional full-resolution version use `prepare_movie.py --jobs 2`;
it requires 128 KiB per decoded native frame and more SD bandwidth. Neither
option changes gameplay, simulation timing or audio decoding.

After checking the validation report and images, install from Windows:

```
py -3.13 tools/install_movie.py --movie assets/GSAP01-rev1/prepared/starfox-pica-half.thp
```

The installer requires a complete manifest and all 1,797 native frames. It
uploads to a temporary name, verifies SHA256 by FTP readback, renames into
`/3ds/foxhollow/starfox-pica.thp`, and verifies again. An existing prepared file
is backed up. It does not modify the ISO or saves. Fixture excerpts and mixed
JPEG/native development streams cannot be installed with this tool.

Install the matching console executable with `tools/console_transfer.py --install`.
Missing or incompatible prepared headers use the original movie. Runtime
failures after opening are reported as errors rather than silently skipping
frames. Source identity is checked using pinned metadata and the original DVD
entry; the console does not hash the entire original disc at startup.

Console validation must measure the actual SD read cost, movie decode timings,
frame intervals and memory. Emulator timings do not establish console FPS.
Compare opening playback, then Start/menu transition; inspect image quality
and audio continuity. Select exits and flushes buffered logs normally.
