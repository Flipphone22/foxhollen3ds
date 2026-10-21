#ifndef FH_AUDIO_REVERB_H
#define FH_AUDIO_REVERB_H

/* Private block-local cursors. The game's delay buffers remain the owners.
 * Delay lines contain aligned floats with valid byte offsets into their own
 * allocations. Publish the final offsets/history before returning to MusyX.
 * No allocation, sample approximation, or cross-callback cache is involved. */
typedef struct { float *input, *output, *base, *end; float lastOutput; } FhReverbCursor;
static inline FhReverbCursor fh_rev_cursor(float *base, int in, int out, int bytes, float last) {
  return (FhReverbCursor){(float*)((unsigned char*)base+in),
    (float*)((unsigned char*)base+out),base,(float*)((unsigned char*)base+bytes),last};
}
static inline void fh_rev_write(FhReverbCursor *cursor, float value) {
  *cursor->input++=value;
  if(cursor->input==cursor->end)cursor->input=cursor->base;
}
static inline float fh_rev_read(FhReverbCursor *cursor) {
  float value=*cursor->output++;
  if(cursor->output==cursor->end)cursor->output=cursor->base;
  cursor->lastOutput=value;
  return value;
}

#endif
