# Melee and Foxhollow implementation review

Reviewed on 2 October 2026 against the commits in [sources.lock.json](../sources.lock.json). Findings below come from source inspection unless explicitly described as an upstream report or a local test result.

The useful precedent is Melee's separation of original game logic, platform replacements, graphics translation, and automated probes. Foxhollow is a better starting point for Star Fox Adventures than applying Melee's complete runtime unchanged. Its existing portability work should be preserved, while desktop dependencies are replaced at explicit interfaces.

## How Melee was implemented

| Area | Source evidence | Implication for this port |
| --- | --- | --- |
| Game compilation | `native-3ds/tools/engine_build.py` compiles decompiled C for `armeb-none-eabi` and converts objects for ARM BE8. `be_entry.S` and `game_bridge.S` switch data endianness around platform calls. | Melee avoids converting all original data to little-endian. This is a deliberate architecture, not an ordinary compiler flag change. |
| Graphics | `port/engine/gx_fifo.c`, `gx.c`, and `port/3ds/renderer.c` implement GX recording/replay and Citro3D rendering. Separate worker and cache code supports asynchronous translation. | Reuse the separation between game commands and rendering. Verify SFA materials independently; Melee's renderer contains game-specific behavior. |
| Platform | `port/3ds` implements file access, card storage, controls, audio, timing, and bottom-screen presentation. | Useful examples of libctru integration and failure reporting. These implementations depend on Melee's bridge and asset layout. |
| Audio | `port/3ds/audio.c` submits PCM through NDSP, checks cache flush results, and contains a development-only HLE audio fixture. | Test generated PCM and device queueing separately. Emulator HLE behavior does not establish hardware DSP/cache correctness. |
| Reproducibility | Toolchain and upstream lock files pin dependencies; extraction checks the supported disc revision. | Pin the compiler, SDK, assets, emulator, settings, and source for every result. |

Source links: [engine build](https://github.com/2gifts/melee-3ds/blob/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/tools/engine_build.py), [graphics implementation](https://github.com/2gifts/melee-3ds/tree/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/port), [audio](https://github.com/2gifts/melee-3ds/blob/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/port/3ds/audio.c), [toolchain lock](https://github.com/2gifts/melee-3ds/blob/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/toolchain.lock.json).

## How Melee was tested

The repository contains several complementary layers:

1. Host tests for decoding, math, memory, rendering state, queues, file access, and other isolated behavior. Python tests also validate asset extraction and the GDB transport. The texture decoder has an independent per-pixel reference and exhaustive packed-color checks.
2. Instrumented 3DS programs and development builds for GPU, stereo, audio, ABI, and runtime probes. Python drivers read symbols and communicate with Azahar's GDB stub.
3. Scenario sweeps that navigate menus, inject pad input, observe scene/frame counters, collect image captures, detect panics and stalled progress, and emit JSON reports. Some scenarios jump directly into content; these are useful coverage probes but do not replace ordinary progression tests.
4. Physical New 3DS testing, reported by the maintainer. The status document gives Versus and Training the strongest coverage and explicitly limits its broader compatibility claims.

The sweep launcher uses Windows `STARTF_USESHOWWINDOW` with a hidden window. It creates isolated portable emulator profiles with separate ports, saves, and logs, while sharing game assets. It distinguishes game failures from some emulator/GPU failures and supports rerunning failures. That is unattended GUI automation, not proof of a true offscreen emulator frontend. The scripts also hard-code parts of the author's toolchain and Azahar 2126.1 directory layout. They need adaptation for a clean machine.

The inherited GitHub build workflow is conditional on `github.repository == 'doldecomp/melee'`. Its presence does not establish continuous testing of the 3DS fork. Likewise, checked-in scenario code establishes available tests, not a passing result for every test at the reviewed commit.

Sources: [tests](https://github.com/2gifts/melee-3ds/tree/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/tests), [sweep launcher and reports](https://github.com/2gifts/melee-3ds/tree/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/tools/qa_sweep), [upstream status report](https://github.com/2gifts/melee-3ds/blob/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/docs/PORT_STATUS.md), [workflow](https://github.com/2gifts/melee-3ds/blob/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/.github/workflows/build.yml).

## What Foxhollow already provides

Foxhollow builds a C11 game archive from `port/compiling.txt`, links it into a C++20 desktop application, and uses Aurora for GameCube-style services and rendering. Its tree includes the game sources, Aurora, Borealis, and a portable MusyX implementation. Aurora's GX renderer depends on Dawn/WebGPU and SDL3. The source tree also contains Aurora tests for GX FIFO/display lists, texture caches, render workers, DVD behavior, and allocation. The publish workflow builds desktop releases; this review did not establish a game progression regression suite.

`port/include/foxhollow_compat.h` and `port/src/game_endian.c` implement explicit big-endian reads and data conversion. Retaining these conversions on little-endian ARM is the proposed approach. Audit their alignment assumptions and all 32-bit structure branches before trusting them on ARM11.

`src/main.c` accepts USA revisions 0 and 1, Japan revision 0, and Europe revisions 0 and 1. It maps these assets to GSAE01, with a configurable USA code revision. It requests a 128 MiB MEM1 arena plus `ARAM_DEFAULT_SIZE`. That is a configured allocation, not a measured live working set and not evidence that SFA inherently requires 128 MiB. The initial memory investigation must examine `game/src/main/mm.c`, arena usage, ARAM, asset caches, graphics buffers, code, and stacks together.

Sources: [build definitions](https://github.com/JackPriceBurns/foxhollow/blob/33f3079b74649aacdebf7dfd311c6ee751b3fc97/CMakeLists.txt), [entry point and disc mappings](https://github.com/JackPriceBurns/foxhollow/blob/33f3079b74649aacdebf7dfd311c6ee751b3fc97/src/main.c), [port layer](https://github.com/JackPriceBurns/foxhollow/tree/33f3079b74649aacdebf7dfd311c6ee751b3fc97/port), [Aurora tests](https://github.com/JackPriceBurns/foxhollow/blob/33f3079b74649aacdebf7dfd311c6ee751b3fc97/extern/aurora/tests/CMakeLists.txt).

## Specific portability risks

- **Materials and framebuffer effects:** `game/src/main/objprint_dolphin.c` uses indirect texture stages and multi-stage materials. `newshadows.c` copies framebuffer content into textures for shadows/reflections. These need explicit PICA200 implementations or documented visual substitutions. A generic textured triangle is insufficient evidence.
- **Timing and callbacks:** `port/src/gx_shim.c` calls `VIWaitForRetrace()` from `GXSetDrawSync()`. `vi_shim.c` pumps events, audio, and callbacks; the OS thread shim does not start original GameCube threads. Rendering, retrace, and game update counts must be measured separately before reducing render frequency or introducing workers.
- **Audio:** `musyx_output.c` uses a 32 kHz stereo software mixer with SDL mutexes. Keep the mixer if ARM measurements support it, replace output/synchronization, and account for streams handled by `ai_shim.c`. Melee's game audio layer is not a replacement for MusyX.
- **Desktop runtime features:** The hook implementation has ARM64 and x86-64 paths and uses desktop executable-memory facilities. Disable dynamic mods/hooks, desktop gallery/UI, and Borealis integration in the initial 3DS target. Resolve every remaining call explicitly.
- **Asset I/O:** Desktop ISO/RVZ support is useful for extraction and reference tests. Prefer prepared SD files or an indexed asset container on 3DS; maintain original DVD offsets and callback semantics where the game needs them.

Sources: [game rendering](https://github.com/JackPriceBurns/foxhollow/tree/33f3079b74649aacdebf7dfd311c6ee751b3fc97/game/src/main), [platform shims](https://github.com/JackPriceBurns/foxhollow/tree/33f3079b74649aacdebf7dfd311c6ee751b3fc97/port/src).

## Headless emulator findings

Dolphin's NoGUI frontend explicitly supports `--platform headless`, and its build exposes `ENABLE_NOGUI` and `ENABLE_HEADLESS`. This is suitable for investigating an original-game reference runner. Choose and verify a rendering backend separately: a Null backend cannot provide visual comparisons. [NoGUI source](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DolphinNoGUI/MainNoGUI.cpp), [build options](https://github.com/dolphin-emu/dolphin/blob/master/CMakeLists.txt).

At the inspected Azahar commit, the normal launcher selects Qt; the CLI module handles compression rather than headless game execution. There is a Libretro build option, but that option disables the normal GDB stub and scripting, among other frontend features. Do not assume an `azahar --headless` switch exists. The proposed initial route is a private virtual display on Linux, with a small offscreen frontend or Libretro harness considered if needed. None of these routes has been executed here yet. [Launcher](https://github.com/azahar-emu/azahar/blob/662d412123305a9f4be94dd3dc73ddf91a18c55e/src/citra_meta/main.cpp), [CLI source](https://github.com/azahar-emu/azahar/blob/662d412123305a9f4be94dd3dc73ddf91a18c55e/src/citra_cli/citra_cli.cpp), [build options](https://github.com/azahar-emu/azahar/blob/662d412123305a9f4be94dd3dc73ddf91a18c55e/CMakeLists.txt).

## Reuse boundaries

Melee's original 3DS integration and build tools declare GPLv3 terms; upstream and third-party material retain their own notices. Foxhollow's root license is CC0, while its bundled Aurora has an MIT notice. Record provenance per imported component instead of assuming either root license covers everything. Code adapted from melee-3ds is listed in `native-3ds/licenses/ADAPTATIONS.md`. [Melee source scope](https://github.com/2gifts/melee-3ds/blob/c2ebe7c769b3287f2b3748173e8fe5ec24ef408e/native-3ds/LICENSE-SCOPE.md).

Foxhollow's `AGENTS.md` prohibits AI-authored human-facing prose in its project. Its checkout was used for read-only research; these user-requested planning documents are outside that checkout. No upstream commit, PR, or documentation edit was made.

