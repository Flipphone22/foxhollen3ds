# Star Fox Adventures disc baseline

The supplied image is verified as Europe/PAL `GSAP01`, revision 1, with internal title `Star Fox Adventures`. The pinned Foxhollow entry point explicitly accepts this identity. It is the selected asset baseline for this project; no US replacement is needed.

Original input:

```text
<downloads>/Star Fox Adventures (Europe) (En,Fr,De,Es,It) (Rev 1).rvz
```

The original 526,193,440-byte RVZ was read without modification. Its SHA256 was checked before and after processing:

```text
95bb8691196288edc1035bc4bb06b58e1e13b8ad8c3d7251be81765e586929be
```

A separate, unscrubbed ISO was created at `assets/GSAP01-rev1/disc.iso` with the existing Dolphin 2606a distribution's command-line tool. It is 1,459,978,240 bytes, with SHA256:

```text
f5e5a9a29147f80648dbef00ed68cd84d0f08c7120645e6f653333296944facf
```

Dolphin's verifier reported `Problems Found: No`. Independently reading the RVZ's decoded disc and the ISO produced the same SHA1:

```text
fa09928f2dc196259521893b0e89eae297dc3df5
```

This establishes conversion consistency and a successful Dolphin integrity check. It does not establish a match against an external canonical dump database or prove gameplay compatibility.

Extraction produced 3,879 files totaling 789,617,752 bytes under `assets/GSAP01-rev1/extracted/`, with `files/` and `sys/` subdirectories. `assets/GSAP01-rev1/manifest.json` records the original path, disc identity, tool hash, container/disc hashes, and every extracted file's size and SHA256. These assets and the manifest are local ignored inputs.

Foxhollow accepts PAL assets through its GSAE01 mapping. Its default code revision remains USA 1.0 (`FOXHOLLOW_REV=0`); disc revision 1 does not automatically select USA code revision 1. Keep the default for the first desktop baseline and record any intentional change. The original Dolphin reference must record language and PAL video mode so timing and regional differences can be distinguished from port regressions.

All processing used command-line tools. No emulator window, gameplay session, or audio output was started.
