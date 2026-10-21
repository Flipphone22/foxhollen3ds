# Output underrun and input timeline repair

The Old 3DS run of `0f7aa0a77ede` stopped with native audio callback fault 13.
Its retained raw input cursor was 490 samples behind the requested source span.
The initiating event was not captured; this report does not claim a confirmed
physical-console repair.

The archived output clock invalidated its input prediction whenever the SDK
reported dropped output frames. Its guard allows playback when the clock is
invalid. Following queue exhaustion, a paired restart could therefore reset
input position and fractional phase within the same epoch, while retained AUX
effect input still expected the old position.

An isolated test using the private DSP firmware in Azahar LLE delayed one
callback by 80 ms. The SDK reported 15 dropped output frames, but source input
continued advancing by 156 or 157 samples per callback. Output deadline misses
did not imply missing input submissions. Azahar HLE hardcodes this drop counter
to zero and cannot exercise that condition without injection.

`audio_drop_clock_patch.py` removes only the dropped-output-counter condition
from input-clock continuity. Missing callbacks, epoch changes, rate changes,
and AUX bank identity checks remain. There are no added allocations, threads,
effect resets, or skipped source samples. The pinned Melee audio queue's SDK
buffer ownership and prebuffer behavior were consulted; its simpler PCM path
does not provide this native AUX clock, so this repair is Foxhollow-specific.

Evidence:

- `old229-audio-fault13/reproduction.json`: exact recorded raw-cursor rejection;
  subsequent queue stamps in the positive control are synthetic.
- `old229-audio-fault13/clock-loss.json`: archived and candidate clock checks,
  1,000 pause lengths, dropped-output handling, missing-callback rejection.
- `audio-drop-semantics/20261018-172814`: firmware output-drop semantics.
- `old229-audio-fault13/queue-pair.json`: source-verified queue, native AUX mixer,
  SDK service, and firmware LLE integration. The same delayed publication and
  producer pause/refill produces 41 errors with the baseline, zero with the
  candidate. Candidate processes 260 blocks and has zero queue underruns.
  This fixture uses a synthetic serial producer, identity effect, and core 0;
  it does not validate all game voices or reverb behavior.
- `audio-drop-clock-v1-console-setup-builds`: source `cfa8c24c05c3`, only clock
  source/header changed from the rejected build. Game archive, GPU programs,
  conversion recipe, and Old 80/New 124 MiB CIA profiles unchanged. Root inputs
  and game archive restored after the isolated build.
- `artifacts/native/audio-drop-clock-v1-palace-old/summary.json`: exact normal
  app reaches and completes the palace benchmark, drains and exits, preserves
  isolated saves/settings. `palace.png` inspected: world, Krystal, enemies,
  textures, HUD present. This is HLE correctness evidence, not the delayed DSP
  regression or a hardware performance measurement.

Observed Old-mode process peak was 70,713,344 / 83,886,080 bytes; heap peak
41,366,136 / 49,283,072; minimum free linear memory 3,667,840 / 12,582,912;
initialized free VRAM 4,723,712; sampled stack upper bound 73,008 / 524,288.
These are route-specific observations, not worst-case budgets for all gameplay.

The normal app package is `foxhollow-iso-setup-f6ba4437832c0fc8.zip`.
See its FTP transfer record for deployment status. The parent rejected build
remains marked rejected. Full rendering, sustained FPS, broader crash safety,
and the original goal remain incomplete.
