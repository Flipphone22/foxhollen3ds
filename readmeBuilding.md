# Building foxhollen3ds

A fork of [Foxhollow](https://github.com/JackPriceBurns/foxhollow) that ports
Star Fox Adventures to the Nintendo 3DS, with a native PICA200 renderer. It
runs on New 3DS / New 3DS XL and, experimentally, on Old 3DS. It is a work in
progress.

This repository contains no game data. You need your own copy of
**Star Fox Adventures, Europe/PAL, `GSAP01` revision 1**. The game's assets are
converted from that disc on the console itself the first time the app runs.

## Requirements

- Windows 10/11 with WSL 2 and an Ubuntu distribution named `Ubuntu`
  (`wsl --install -d Ubuntu`), or a Linux machine where you can run commands as
  root (the build uses `mount --bind` and `chroot`).
- Python 3.11 or newer and Git on the host.
- [Dolphin](https://dolphin-emu.org/) (`DolphinTool`) to turn your disc dump
  into a plain ISO.
- Roughly 8 GB of disk space for the toolchain, the build chroot and build
  outputs. The first setup downloads the pinned devkitARM toolchain, an Ubuntu
  24.04 base image, the Azahar emulator, makerom and bannertool, and installs
  Ubuntu packages inside the build chroot (network access required).

## Build

All commands run from the repository root. `<repo>` is the repository path as
seen from WSL/Linux (for example `/mnt/c/src/foxhollen3ds`). On plain Linux, run
the `sh` commands with `sudo` instead of through `wsl`.

1. Clone without line-ending conversion:

   ```sh
   git clone -c core.autocrlf=false <url> foxhollen3ds
   cd foxhollen3ds
   ```

2. Download and verify the toolchain, then set up the build chroot:

   ```sh
   python tools/bootstrap_foundation.py
   wsl -d Ubuntu -u root --exec sh <repo>/tools/setup_linux.sh
   ```

   The chroot lives in `/home/user/.cache/foxholen3ds` inside WSL/Linux; set
   `FOXHOLEN3DS_CACHE` to use another location (for both scripts).

3. Build:

   ```sh
   wsl -d Ubuntu -u root --exec sh <repo>/tools/linux_exec.sh make -C /work -j8
   ```

   The app is written to `build/app/foxhollow.3dsx` and `build/app/foxhollow.cia`.

## Prepare the game assets

The game's assets are converted from your disc. You need an unscrubbed ISO
whose SHA-256 is
`f5e5a9a29147f80648dbef00ed68cd84d0f08c7120645e6f653333296944facf`:

```sh
DolphinTool convert -f iso -i "<your dump>.rvz" -o disc.iso
```

You can convert either on the console or on your PC.

- **On the console:** copy the ISO to `SD:/3ds/foxhollow/disc.iso`, start the
  app and choose **Prepare / resume assets**. Conversion can be resumed if
  interrupted.
- **On your PC** (much faster): build the converter and run it on the ISO,
  then copy the output folder to `SD:/3ds/foxhollow/prepared` and the ISO to
  `SD:/3ds/foxhollow/disc.iso` (the app still needs the ISO).

  ```sh
  wsl -d Ubuntu -u root --exec sh <repo>/tools/linux_exec.sh make -C /work convert
  wsl -d Ubuntu -u root --exec sh <repo>/tools/linux_exec.sh /work/build/fh-convert /work/disc.iso /work/prepared
  ```

## Install on a 3DS

The console needs custom firmware with the Homebrew Launcher or FBI.

1. Copy `build/app/foxhollow.3dsx` and `build/app/foxhollow.cia` to
   `SD:/3ds/foxhollow/`, together with the ISO (and `prepared/` if you
   converted on your PC), as described above.
2. Install `foxhollow.cia` with FBI (or start `foxhollow.3dsx` from the
   Homebrew Launcher).
3. On first launch without PC-converted assets, choose **Prepare / resume
   assets**. Then choose **Start game**.

## Repository layout

- `native-3ds/` – the 3DS application: platform layer, PICA renderer, audio,
  on-device asset preparation. `native-3ds/wip/` holds unfinished code that is
  not built.
- `tools/` – build, packaging, deployment and test tooling, including the
  game-side source overrides applied to the decompilation at build time.
- `tests/` – host and ARM unit tests.
- `docs/` – design notes and development records
  ([PORT_KNOWLEDGE.md](docs/PORT_KNOWLEDGE.md) is the current summary).

## Licence

GPL-3.0; see [LICENSE](LICENSE). The project includes code adapted from
melee-3ds (GPL-3.0), Citra/Azahar (GPL-2.0-or-later), libctru and citro3d
(zlib), the MusyX decompilation (MIT), meshoptimizer (MIT) and rg_etc1 (zlib);
the individual notices are in `native-3ds/licenses/`. Star Fox Adventures is
owned by Nintendo; this project is not affiliated with or endorsed by Nintendo.
