# Native actor index ordering

This is an offline experiment, not a runtime change or console performance
claim. `tools/audit_actor_vertex_cache.py` reads the original prepared pack
without replacing it and uses the pinned meshoptimizer cache optimizers.

All835models,1609draw groups and988,530indices are checked. Every candidate
preserves the multiset of exact oriented triangle index triples. No vertex
position, UV, colour, normal or bone binding is changed. Each group remains a
separate analysis unit, matching the SDK's post-vertex cache clear after a draw.

The general optimizer's aggregate results are:

| Modeled FIFO entries | Original vertex invocations | Reordered | Reduction |
| --- | ---: | ---: | ---: |
|4|532979|525034|1.49%|
|8|517051|365545|29.30%|
|16|497907|332681|33.18%|
|32|473742|324463|31.51%|
|64|442099|320153|27.58%|

These are synthetic cache models, unweighted by actual scene frequency, not
elapsed GPU times. The pinned Azahar software pipeline uses a64-entry circular
cache; its accelerated renderer and real PICA behavior must not be assumed
identical. [3dbrew's register documentation](https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_POST_VERTEX_CACHE_NUM)
identifies a cache configuration register but does not establish the effective
cache policy for this workload. No register is changed by this experiment.

This has a substantially larger modeled workload reduction than the rejected
rigid-culling experiment, but applying it universally would be incorrect:
triangle order affects blending and can affect equal-depth overlaps. A native
consumer must retain source order for those cases. Model morphs, shared index
ranges, shadow plans and runtime material state also need explicit handling.
No optimized index pack has been installed or selected by gameplay. Report and
per-group counts: `actor-vertex-cache-audit/report.json`.
