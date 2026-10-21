# Two-layer opacity audit

The native actor loader declines a second texture layer when the first has a
separate opacity plane. A complete read-only audit of the current PAL revision 1
model collection found **no material affected by that particular restriction**.
It is not a confirmed cause of the reported rendering defects.

`tools/audit_pica_masked_layers.py` examines the used material declarations in
829 unique models. All 66 two-layer materials resolve both textures, including
all 267 referenced animation frames. None has a separate opacity plane on either
layer, and none changes opacity-plane presence between frames. Eight models have
instruction streams the existing audit decoder cannot resolve; conservatively
checking every material declaration in those models finds no additional
two-layer candidates. The private metadata report is
`artifacts/research/pica-scene-rewrite/masked-layer-audit/report.json`.

The initial audit left 47 materials unresolved because it interpreted animated
texture offset tables as compression headers. The corrected resolver follows
`textureLoadInternal` and `tex1GetFrame`: table bits 24–29 give the frame count;
multi-frame records begin with count+1 relative offsets. Every compressed or
stored frame is bounded and decoded separately. No asset or runtime renderer was
changed, and this audit is neither visual parity nor performance evidence.

Do not add a three-sampler masked-overlay path on the basis of the initial
unresolved count. Other layered-material limitations require their own source
and live-scene evidence. The skybox repair and requested two-console deployment
remain separate, unresolved work.
