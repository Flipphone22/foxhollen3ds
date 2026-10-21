# Old 3DS palace transition allocation failure

Read-only collection: `artifacts/console-test/20261014-162101-old229/`.
The user approached the last palace hallway with Krystal. At retrace 27264,
`panic.txt` reports `PICA HUD image allocation`. This is an application panic,
not evidence of another GPU hang. The 16x16 prepared RGBA8 image required 1024
bytes. The image table still had space (slot 413/512), and image residency was
1,759,520 bytes against its 4 MiB ceiling.

The actual failure was the unchanged 15% linear reserve test: 1,572,992 bytes
free, less the request, would leave 1,571,968 bytes, 896 below the 1,572,864
reserve. The texture allocator therefore did not attempt an SDK allocation.
The final memory row records 40,251,456/50,331,648 normal bytes and
68,886,528/83,886,080 committed process bytes. The SD application matches
fa18b1a85a42; the gameplay log lacks its own embedded build tag, so this does
not independently prove which CIA was installed on HOME.

The candidate adds a 48 MiB normal / 12 MiB linear startup profile between the
existing 52/16 and 48/10 choices. It is selected only when the kernel's current
commit and actual grant leave at least 15% process headroom. It takes no memory
from the normal heap and changes neither the texture reserve nor image ceiling.
At the captured commit this adds 2 MiB linear RAM with 12,902,400 bytes process
headroom remaining. New 3DS's 68/24 profile stays unchanged. A larger executable
that cannot meet the reserve still selects the previous smaller profile.

Pinned Melee's SDK allocation/error checks and `early_queue.c` resource lifetime
rules were consulted. No resource migration, premature free, raw transfer,
additional GPU command, per-frame CPU work, or visual omission is introduced.
The pure patch and isolated builder use the deployed Fast source archive,
excluding unrelated actor-fog installer work. Game archive, shaders, conversion
recipes and user preferences remain unchanged.

`tools/test_old_linear_headroom.py` executes the actual candidate profile and
the exact archived texture admission expression under ASan/UBSan. It reproduces
the failed request, admits it with the expanded profile, still rejects excess
requests, checks the one-byte profile boundary, invalid inputs and a grid of
grant/current-commit combinations. This is allocation-policy proof, not an
end-to-end replay of the user's hallway. The normal application's exact Old80
startup check is `tools/run_old_linear_headroom.py`; physical hallway acceptance
remains outstanding.

The collected run averaged 21.64 presentation FPS across all recorded intervals,
including intro, menus, changed graphics options and loading. This is not a
matched scene benchmark or an established speedup. There were 434 intervals over
100 ms and a 4.25-second maximum. Performance and stalls still need work.

Local validation of normal e3052ddc285f completed in 84.18 host seconds, with
Old80 startup, preset selection, intro, cockpit menu and Select shutdown. The
actual process commit was 70,983,680/83,886,080 bytes, normal peak 39,051,680 /
50,331,648 and linear minimum free 5,930,368 / 12,582,912. The cockpit capture was
inspected. This route does not cover the reported hallway. No FPS comparison:
foreign host compilation was active. Game archive and every linked GPU program
are unchanged; only fh_heap_profile.h differs from the installed Fast source.
The base conversion recipe is identical, so existing prepared assets are reused.
