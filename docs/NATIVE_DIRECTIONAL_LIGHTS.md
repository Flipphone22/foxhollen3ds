# Draw-constant directional lighting

Local candidate, not deployed or physically accepted. Directional vectors are
constant over an actor draw. The previous native shader subtracted zero times
the vertex position and normalized each of those vectors again at every vertex.
The renderer now normalizes active directional vectors once when copying the
object's light snapshot. The original game state remains untouched. Point-light
directions still depend on each vertex and are calculated on the PICA.

The ordinary, layered and fur programs use one arithmetic-only conditional leaf
for point normalization. There is no new shader bank, draw, resource, vertex
stream, material route or worker. The existing light uniforms and zero/one light
tags remain unchanged. Disabled lighting skips the new CPU normalization work.
The CPU work is bounded by the existing eight-light limit; no allocation or I/O.

The directional calculation uses six executed instructions rather than nine,
including its comparison and conditional call. A point uses eleven rather than
nine. These are instruction counts, not cycles or a frame-speed estimate. The
shared actor/HUD program shrinks from511to492instructions. Whether this wins
overall depends on the lights and vertices actually used in a scene and on
hardware branch behavior. Physical performance is unverified.

Pinned Melee's shader verification/output-initialization discipline and the
native shared-program linker were consulted. The linker now relocates CALLC;
the contract admits only temporary-writing arithmetic leaves, without nested
calls, loops, output writes, MOVA or a return at the caller's IF boundary. It
does not relax the output-initialization, IF-depth or program-size limits.

Exact linked Azahar float24 tests cover65,536vertices each in the ordinary,
layered and fur programs, including mixed light types, inactive lights, zero
and very small directions, alpha lighting, skinning and environment outputs.
All-point cases are bit-identical. Largest directional colour/alpha changes are
0.0000610351562,0.000000171363354and0.0000305175781respectively, below one8-bit
step. All other output lanes are exact. Omitting CPU preparation fails all three
tests. This does not establish rasterization parity or hardware correctness.

Evidence: `directional-test/shader.json`, `assembly.json` and
`build-verification.json`. The first build failed assembly because CMP's second
operand cannot be a uniform; a focused assembly check also corrected CALLC
operand order before the second full build. Failures remain recorded. Runtime
results are recorded below.

The uncached V2 moving replay completed but its disjoint scene+submission+
actor-render CPU sum was0.161064ms above actor-live-v2. Workloads differ, so this
does not isolate normalization's cost. V3 adds an eight-slot,224-byte cache,
keyed by all three direction components, plus12bytes of counters. The cache
belongs to the existing renderer owner; no pointers or object lifetimes are
cached. Actual ASan/UBSan tests cover256,000publications,7,000normalizations and
individual-component changes, zero/tiny vectors and point handoffs.

V3 replay`925f77cadf4d`/CIA`85201c1f31ce` is BUILT, NOT PROMOTED or deployed.
No matching normal build was made. Its458sources, unchanged game archive,
Old80/New124profiles and all13programs are checked. The three changed GPU
programs are byte-identical to the V2 float24-tested programs; the other ten
remain identical to actor-live-v2. Wrong CALLC relocation, recursive helpers
and helpers writing outputs are rejected by the verifier.

The serial New moving route and Old cave complete in75.315and58.812hostseconds.
State/save/log-drain/owned-exit and collision receiver coverage checks pass.
New1650and Old1410captures were inspected: world, player, HUD and shadow remain,
with Arwing and Old touch stick present. New1500/1650were also inspected for V2.
These are regression captures, not pixel-equivalence or full visual acceptance.

Against actor-live-v2, V3's disjoint rendering CPU sum changes+0.004142ms New
and+0.026384ms Old. Azahar53.589->53.693FPS New and57.432->57.275Old are small
single-run differences with differing poses/work counts, not a demonstrated
speedup. No console inference is made.

The important negative result is workload composition. New published1,642
directional and3,768point lights;784directional cache misses occurred. Old cave
published3,081point lights and **zero directional lights**. These counts are
per publication, not vertex-weighted. Nevertheless the Old cave has no possible
directional saving and pays the extra two point-light instructions. A CPU-only
gate passing is insufficient to accept that GPU tradeoff. Keep actor-live-v2
as the local continuation baseline; do not deploy this experiment or spend a
console run on it. The unused normal-builder draft was removed.

New heap56566232/71303168, linearfree16041472/25165824, process103325696/130023424;
Old heap40973392/50331648, linearfree3759104/10485760, process67674112/83886080.
Both VRAMfree4723712; stack73068New/73008Old of524288reserved. The process grant
commit is4KiB above baseline. There is no new GPU allocation; varying transient
heap/linear figures are not allocation savings. Before the V3 New run, the
storage guard rejected startup; completed old ELF evidence was compressed with
hash checks and the guard passed on retry. No live emulator was restarted.

Next investigate skipping normals for ambient-only materials. The cave log has
37,990enabled-light object publications but only3,081selected point lights.
Environment/specular consumers still require normals, so an empty light list
alone is not a valid skip condition. Prepared compact UVs duplicate UV0 into
UV1 when no secondary source exists; reclaiming that redundant selection flag
may provide a dedicated normal-demand flag without another shader bank. This
is an investigation, not implemented or accepted behavior.
