# Initial local validation

For the subsequent implemented 3DS diagnostic and desktop title-screen milestone, see [Foundation validation](FOUNDATION.md). The table below preserves the earlier source-review baseline.

Date: 2 October 2026. These checks assess reference code and available tooling; they do not validate a Star Fox Adventures port.

| Check | Result |
| --- | --- |
| Melee synthetic asset tests | 6 passed, Python 3.13 |
| Melee GDB transport tests | 3 passed, Python 3.13 |
| Melee native texture decoder | Passed under WSL Ubuntu with `cc -O2 -Wall -Wextra`; 131,072 RGB565/RGB5A3 inputs and 1,834,176 pixels across 256 CMPR textures matched independent references |
| Texture test compiler warnings | Two existing misleading-indentation warnings; no source edits made |
| Foxhollow static inventory | 794 listed game translation units, no missing or duplicate entries; 127 GX identifiers across 3,787 lexical occurrences |
| Reference integrity | Both checkouts match their pinned commits and have no tracked or untracked changes reported by Git after testing |
| Disc identity | Verified from the image header: Star Fox Adventures, Europe/PAL, GSAP01, revision 1 |
| Disc conversion | Original RVZ unchanged; separate 1,459,978,240-byte ISO created without scrubbing |
| Disc integrity | Dolphin verifier reported no problems; decoded RVZ and ISO SHA1 both `fa09928f2dc196259521893b0e89eae297dc3df5` |
| Disc extraction | 3,879 files, 789,617,752 bytes; SHA256 manifest saved locally |
| Desktop gameplay | Not run; verified game assets are now available |
| 3DS build and emulator gameplay | Not run; no port build exists |
| Physical console | Not tested |

Commands for the Python checks, from `reference/melee-3ds/native-3ds`:

```powershell
py -3.13 -m unittest discover -s tests -p test_assets.py -v
py -3.13 -m unittest discover -s tests -p test_gdb_transport.py -v
```

The C test was compiled directly, because its upstream Python wrapper expects the author's pinned Windows compiler directory:

```sh
cc -O2 -Wall -Wextra \
  <repo>/reference/melee-3ds/native-3ds/tests/texture_decode_tests.c \
  -o <repo>/artifacts/research/texture-decode-tests
<repo>/artifacts/research/texture-decode-tests
```

Windows has Git, CMake, Ninja, and Python 3.13 available. The default `python` command resolves to Python 3.7, so commands use `py -3.13` explicitly. Ripgrep is absent; source searches used `git grep` and PowerShell. WSL's `Ubuntu` distribution has `cc` and CMake. The queried PATHs did not expose Azahar, Dolphin NoGUI, or a 3DS cross-compiler; the WSL PATH also did not expose Xvfb or Ninja. This is a PATH-level check, not an exhaustive search of the user's machine. No emulator GUI was launched.

After the disc path was provided, an existing `DolphinTool.exe` was located in `<downloads>/dolphin-2606a-x64/Dolphin-x64/`. Its command-line header, conversion, verification, and extraction operations were used with workspace-local processing directories. Its hash and the input/output hashes are in `assets/GSAP01-rev1/manifest.json`. The tool does not accept SHA256 for its decoded-image verification option; a rejected SHA256 attempt was corrected to SHA1, while Python computed SHA256 file hashes separately. No external canonical dump database was checked, and no gameplay was launched.

Run `py -3.13 tools/source_inventory.py` from the workspace root to regenerate the static inventory and verify the reference commit IDs. It checks listed game source paths and records lexical GX occurrences; it does not compile those sources or discover all runtime display-list commands.
