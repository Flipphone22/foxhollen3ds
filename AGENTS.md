# Project instructions

You are not allowed to write any prose that will be read by another human. No commit messages, PR descriptions, code
comments. These must be written by a real human.

The user wants routine testing to be unattended and predominantly headless. Use isolated profiles, captured logs/images/audio, scripted input, bounded runs, and cleanup limited to owned processes. Avoid visible emulator windows, desktop input automation, and audible playback. Custom emulator builds are authorized when needed. Preserve the original supplied RVZ and keep all game assets and saves out of source control.

- Read `docs/PORT_KNOWLEDGE.md` before starting work. It covers current goals and targets, deployed builds, open bugs, the do-not-repeat list, PICA/engine facts, and tooling.
- Native PICA renderer only; never reconnect the old GX/CPU-vertex renderer. Consult `reference/melee-3ds/native-3ds/` (pinned in `sources.lock.json`) for graphics, OS, and hardware behavior.
- Build with `make` (see README), test with `make -C tests check` and `tools/emulator_check.py`.
- Prefer structural performance changes that remove a measured dominant workload. Never claim physical FPS from emulator or instruction-count data.
- Do not poll or overwrite a pending hardware/FTP test, and do not loop FTP retries. Ask for a console run only for substantive changes.
- Before trying another performance candidate, check the do-not-repeat list in PORT_KNOWLEDGE.md.

## Records

- `docs/PORT_KNOWLEDGE.md`: the condensed, current knowledge base. Update it in place when facts change (new deploys, fixed or new bugs, rejected experiments).
- `docs/knowledge/01..08_*.md`: per-period detail with citations into the raw log.
- `private/CODEX_PROGRESS_LOG.md` (local only, gitignored): the raw session history (2.3 MB, newest first). Grep it; do not read it whole. Append new progress entries there, never to this file.

This file is loaded into every agent session and must stay small.
