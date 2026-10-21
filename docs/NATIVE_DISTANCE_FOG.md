# Native distance fog candidate

Candidate included in the e783636ad1e8/b49219fa4c26 shared apps; deployment
completion is recorded separately. Hardware correctness and full visual parity
remain unproven. Contact shadows are optional and off by default in normal play.

Original `track/intersect_render.c` publishes an exponential distance fog:
transmission is `2^(-8 * clamp((eye_distance-start)/(end-start),0,1))`.
Its range producer maps the sky's parameters against the current camera planes.
World material flag 4 enables it; actor model flag 0x100 disables it. Heavy fog
uses additional height/texture stages and is not included here.

The fixed PICA fog LUT uses non-linear depth. With the original default camera
near/far 2.5/10000, a uniform 128-interval depth table cannot resolve a distant
transition faithfully. This candidate instead exports linear eye distance in
texcoord2 and uses procedural texture alpha lookup. A final RGB combiner blends
the rendered colour with the live fog colour; alpha stays unchanged. Existing
texture units 0/1, lighting, skinning and geometry stay native. No framebuffer
copy, per-frame mesh rebuilding, CPU vertex colour pass or new linear texture
is involved.

The live range/colour producers publish small parameters directly. No GX state
interpreter or old `platform_native_fog` rendering path is reconnected. The LUT
is rebuilt only when the range changes. It is uploaded through normal Citro3D
command encoding; static storage remains alive across every queued reader.
Pinned Melee's stored fog parameters and shared-DVLP/resource ownership were
consulted. Procedural fog is a Foxhollow-specific adaptation: the pinned Melee
GX setters are not evidence that it implements this effect.

The candidate adds two vertex instructions and one output, reaching seven
vertex outputs, 96 float registers, and 470/512 shared instructions. Float and
output registers have no spare capacity in this entry; further features must
reuse/reorganize registers or use another bounded shader entry. Resident SDK
tables/configuration total 3096 ordinary bytes plus small scalar state; there
is no linear/VRAM allocation. Actor material fog eligibility adds one word per
resident material. SDK frame/context restoration can re-upload LUT command
words, which must be included in measured submission cost.

DISTANCE FOG is a separate bottom-screen option (page 7); profile/restart
options move to page 8. Recording/replay retains its mask. Other pipelines reset
their combiners and the scene invalidates fog state when re-entered. Moon,
movie and UI material paths explicitly disable fog. Rendered height-dependent
heavy fog, effect-specific fog and whole-game coverage remain outstanding.

Host/ASan checks execute the actual owner and pinned SDK LUT encoder: 10000
cached binds cause no repeated table/uniform publication; OFF causes no LUT
work; toggles/handoffs/invalid parameters preserve the intended state. 200020
distance comparisons across 20 representative ranges have maximum
transmission error 0.014346. This is interpolation/quantization error near the
fog start, not a claim of exact original pixels. Very narrow ranges require
additional assessment. The original samples and alpha are not altered to make
tests pass. CPU-side checks do not establish hardware shader execution.

Acceptance budget for the palace stage is at most 0.20 ms incremental native
submission in Azahar, no new linear/VRAM use, and at least 15% ordinary/process
headroom in each tested hardware profile. Console cost and full-frame targets
remain independent gates. Paired runtime checks are recorded under
`artifacts/research/pica-scene-rewrite/distance-fog-v3-checks`.

V3 paired New palace checks passed with visible distance haze and retained
Krystal, room, EarthWalker, purple particles, HUD and contact shadow. ON/OFF
submission was 3.868584/3.832368 ms; both measured 59.831 FPS in Azahar. Only two
fog table builds occurred over the run. Peak heap was 48,150,216/48,149,560;
linear free 19,474,048 and initialized VRAM free 4,858,880 bytes were identical.

Old cave ON/OFF checks also passed: submission 5.341846/5.207009 ms, with
48.521/48.791 FPS. Both had heap 40,738,880, linear free 4,793,984, process
67,416,064/83,886,080 and VRAM free 4,858,880 bytes. The nearby cave lies mostly
before fog start, so its captures validate retained geometry/HUD/particles,
not a strong visible distance-fog change. One inactive-range rejection occurred
in both; its exact initial parameters have not yet been instrumented. The final
valid range is approximately 1466.2 to 10999.75, camera 2.5 to 10000. The sky's
tiny animated changes cause 579 table builds: a quantization-bounded reuse
policy is needed rather than doing 129 exponentials for invisible changes.

The cave checks also exposed an independent contact-shadow regression. With
fog OFF, shadows ON/OFF measured scene preparation 5.215503/1.789037 ms and
48.791/58.210 FPS. Thus the earlier palace-only shadow gate was insufficient.
Do not deploy the archived ground-shadow candidate as performance-accepted.
A candidate moving original zero-alpha admission before receiver queries is
being checked; it preserves the game's fade and visible footprint geometry.

V5 resolves the repeated fog-table work with a quantization-bounded curve cache.
It compares the extrema of the two clamped affine distance curves and uses the
exponential derivative bound; reusing a curve changes continuous transmission
by at most 1/4095. The previous LUT and its distance scale are kept together.
The real Old-profile cave test reduced rebuilds from 579 to 11, with 593 reuse
events. The one inactive range was exactly start=end=near=2.5, far=10000;
this intentionally disables fog until the source publishes a nonzero range.
Host ASan/UBSan checks also passed. This does not remove the existing LUT
interpolation error (up to 0.014345870 over the tested range set).

Moving shadow alpha admission before collision queries saved some CPU work but
did not meet the cave budget. The release candidate therefore exposes contact
shadows as opt-in for ordinary gameplay. Its default-mask checks and optional
all-on diagnostic runs are distinct; timings must identify which was used.
