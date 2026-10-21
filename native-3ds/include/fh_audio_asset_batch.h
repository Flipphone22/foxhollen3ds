/* Explicit immutable-input scope around the game's consecutive SFX pushes.
 * A batch owns one conversion; successful groups retain individual references.
 * Outside that scope, decoder semantics remain independent per invocation.
 */
void fh_audio_asset_batch_begin(void);
void fh_audio_asset_batch_end(void);
