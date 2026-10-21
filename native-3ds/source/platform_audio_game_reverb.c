#define ReverbSTDCreate fh_game_ReverbSTDCreate
#define ReverbSTDFree fh_game_ReverbSTDFree
#define ReverbSTDModify fh_game_ReverbSTDModify
#define ReverbSTDCallback fh_game_ReverbSTDCallback
#define salMalloc fh_game_reverb_alloc
#define salFree fh_game_reverb_free
/* Derived from the MusyX decompilation, src/musyx/runtime/CheapReverb/creverb.c,
 * as included in https://github.com/JackPriceBurns/foxhollow at commit
 * 33f3079b74649aacdebf7dfd311c6ee751b3fc97 (extern/musyx).
 * Copyright (c) 2023 Axiomatic Data Laboratories, MIT License; see
 * native-3ds/licenses/musyx-MIT.txt. Altered for Foxhollow: symbol renames,
 * frame-count entry points and storage sizing; the PowerPC assembly path was removed. */
#include "musyx/musyx.h"
#include "musyx/sal.h"
#include <float.h>
#include <math.h>
#include <string.h>

static void DLsetdelay(_SND_REVSTD_DELAYLINE *dl, s32 lag) {
  dl->outPoint = dl->inPoint - (lag * sizeof(f32));
  while (dl->outPoint < 0) {
    dl->outPoint += dl->length;
  }
}

static void DLcreate(_SND_REVSTD_DELAYLINE *dl, s32 len) {
  dl->length = (s32)len * sizeof(f32);
  dl->inputs = (f32 *)salMalloc(len * sizeof(f32));
  memset(dl->inputs, 0, len * sizeof(len));
  dl->lastOutput = 0.f;
  DLsetdelay(dl, len >> 1);
  dl->inPoint = 0;
  dl->outPoint = 0;
}

static void DLdelete(_SND_REVSTD_DELAYLINE *dl) { salFree(dl->inputs); }

extern unsigned fh_game_reverb_channels(void);
bool ReverbSTDCreate(_SND_REVSTD_WORK *rv, float coloration, float time, float mix, float damping,
                     float predelay) {
  static u32 lens[4] = {1830, 2045, 443, 152};
  unsigned channels=fh_game_reverb_channels();
  if(channels<2 || channels>3)return FALSE;
  u8 i; // r31
  u8 k; // r29

  if ((coloration < 0.f || coloration > 1.f) || (time < .01f || time > 10.f) ||
      (mix < 0.f || mix > 1.f) || (damping < 0.f || damping > 1.f) ||
      (predelay < 0.f || predelay > 0.1f)) {
    return FALSE;
  }

  memset(rv, 0, sizeof(_SND_REVSTD_WORK));
  for (k = 0; k < channels; ++k) {
    for (i = 0; i < 2; ++i) {
      DLcreate(&rv->C[i + k * 2], lens[i] + 2);
      DLsetdelay(&rv->C[i + k * 2], lens[i]);
      rv->combCoef[i + k * 2] = powf(10.f, (int)(lens[i] * -3) / (time * (16756991.f/512.f)));
    }
    for (i = 0; i < 2; ++i) {
      DLcreate(&rv->AP[i + k * 2], lens[i + 2] + 2);
      DLsetdelay(&rv->AP[i + k * 2], lens[i + 2]);
    }
    rv->lpLastout[k] = 0.f;
  }

  rv->allPassCoeff = coloration;
  rv->level = mix;
  rv->damping = damping;
  if (rv->damping < 0.05f) {
    rv->damping = 0.05f;
  }

  rv->damping = 1.f - (rv->damping * 0.8f + 0.05f);

  if (predelay != 0.f) {
    rv->preDelayTime = (int)(predelay * (16756991.f/512.f));
    for (k = 0; k < channels; ++k) {
      rv->preDelayLine[k] = salMalloc(rv->preDelayTime << 2);
      memset(rv->preDelayLine[k], 0, rv->preDelayTime << 2);
      rv->preDelayPtr[k] = rv->preDelayLine[k];
    }
  } else {
    rv->preDelayTime = 0;
    for (k = 0; k < channels; ++k) {
      rv->preDelayPtr[k] = NULL;
      rv->preDelayLine[k] = NULL;
    }
  }

  return TRUE;
}

bool ReverbSTDModify(_SND_REVSTD_WORK *rv, float coloration, float time, float mix, float damping,
                     float predelay) {
  unsigned char i; // r31

  if ((coloration < 0.f || coloration > 1.f) || (time < .01f || time > 10.f) ||
      (mix < 0.f || mix > 1.f) || (damping < 0.f || damping > 1.f) ||
      (predelay < 0.f || predelay > 100.f)) {
    return FALSE;
  }

  rv->allPassCoeff = coloration;
  rv->level = mix;
  rv->damping = damping;
  if (rv->damping < 0.05f) {
    rv->damping = 0.05;
  }

  rv->damping = 1.f - (rv->damping * 0.8f + 0.05f);

  for (i = 0; i < 6; ++i) {
    DLdelete(&rv->AP[i]);
  }

  for (i = 0; i < 6; ++i) {
    DLdelete(&rv->C[i]);
  }

  if (rv->preDelayTime != 0) {
    for (i = 0; i < 3; ++i) {
      salFree(rv->preDelayLine[i]);
    }
  }

  return ReverbSTDCreate(rv, coloration, time, mix, damping, predelay);
}







static void HandleReverbChannel(s32 *sptr, _SND_REVSTD_WORK *rv, s32 k, unsigned frames) {
  if (!frames) return;
  _SND_REVSTD_DELAYLINE *rings[4] = {&rv->C[k*2], &rv->C[k*2+1], &rv->AP[k*2], &rv->AP[k*2+1]};
  const f32 wet = rv->level * 0.6f, dry = 0.6f - wet;
  const f32 coef0 = rv->combCoef[k*2], coef1 = rv->combCoef[k*2+1];
  const f32 pass = rv->allPassCoeff, damping = rv->damping;
  f32 c0last = rings[0]->lastOutput, c1last = rings[1]->lastOutput;
  f32 a0last = rings[2]->lastOutput, a1last = rings[3]->lastOutput;
  f32 low = rv->lpLastout[k];
  while (frames) {
    unsigned count = frames;
    for (unsigned j=0;j<4;j++) {
      unsigned in = (unsigned)(rings[j]->length-rings[j]->inPoint)/sizeof(f32);
      unsigned out = (unsigned)(rings[j]->length-rings[j]->outPoint)/sizeof(f32);
      if (in<count) count=in;
      if (out<count) count=out;
    }
    f32 *pre = rv->preDelayPtr[k];
    if (rv->preDelayTime>=32) {
      unsigned left = (unsigned)(rv->preDelayLine[k]+rv->preDelayTime-pre);
      if (left<count) count=left;
    }
    /* Constructor-owned arrays are disjoint; each ring's read and write
     * pointers may overlap, so restrict is deliberately NOT used here. */
    f32 *cw0=rings[0]->inputs+rings[0]->inPoint/sizeof(f32);
    f32 *cr0=rings[0]->inputs+rings[0]->outPoint/sizeof(f32);
    f32 *cw1=rings[1]->inputs+rings[1]->inPoint/sizeof(f32);
    f32 *cr1=rings[1]->inputs+rings[1]->outPoint/sizeof(f32);
    f32 *aw0=rings[2]->inputs+rings[2]->inPoint/sizeof(f32);
    f32 *ar0=rings[2]->inputs+rings[2]->outPoint/sizeof(f32);
    f32 *aw1=rings[3]->inputs+rings[3]->inPoint/sizeof(f32);
    f32 *ar1=rings[3]->inputs+rings[3]->outPoint/sizeof(f32);
    for (unsigned i=0;i<count;i++) {
      f32 sample=(f32)sptr[i], delayed=sample;
      if (rv->preDelayTime) {
        delayed=*pre;*pre++=sample;
        /* Tiny legal delays must not turn every sample into a new span. */
        if (rv->preDelayTime<32 && pre==rv->preDelayLine[k]+rv->preDelayTime)
          pre=rv->preDelayLine[k];
      }
      cw0[i]=coef0*c0last+delayed;
      cw1[i]=coef1*c1last+delayed;
      c0last=cr0[i];c1last=cr1[i];
      f32 sum=c0last+c1last;
      f32 a0input=pass*a0last+sum;
      aw0[i]=a0input;
      f32 lp=a0last-pass*a0input;
      a0last=ar0[i];
      lp*=0.3f;
      low=damping*low+lp;
      f32 a1input=pass*a1last+low;
      aw1[i]=a1input;
      f32 output=a1last-pass*a1input;
      a1last=ar1[i];
      sptr[i]=(s32)(dry*sample+wet*output);
    }
    for (unsigned j=0;j<4;j++) {
      rings[j]->inPoint+=(s32)(count*sizeof(f32));
      rings[j]->outPoint+=(s32)(count*sizeof(f32));
      if (rings[j]->inPoint==rings[j]->length) rings[j]->inPoint=0;
      if (rings[j]->outPoint==rings[j]->length) rings[j]->outPoint=0;
    }
    if (rv->preDelayTime) {
      if (pre==rv->preDelayLine[k]+rv->preDelayTime) pre=rv->preDelayLine[k];
      rv->preDelayPtr[k]=pre;
    }
    sptr+=count;frames-=count;
  }
  rings[0]->lastOutput=c0last;rings[1]->lastOutput=c1last;
  rings[2]->lastOutput=a0last;rings[3]->lastOutput=a1last;
  rv->lpLastout[k]=low;
}

static void HandleReverb(s32 *sptr, _SND_REVSTD_WORK *rv) {
  HandleReverbChannel(sptr, rv, 0, 160);
  HandleReverbChannel(sptr + 160, rv, 1, 160);
  HandleReverbChannel(sptr + 320, rv, 2, 160);
}

void ReverbSTDCallback(s32 *left, s32 *right, s32 *surround, _SND_REVSTD_WORK *rv) {
#if MUSY_TARGET == MUSY_TARGET_PC
  // TODO: i don't know why this is necessary yet...
  u8 i;
  for (i = 0; i < 6; ++i) {
    if (rv->AP[i].inputs == NULL || rv->C[i].inputs == NULL) {
      return;
    }
  }
#endif
  (void)right;
  (void)surround;
  HandleReverb(left, rv);
}

void ReverbSTDFree(_SND_REVSTD_WORK *rv) {
  u8 i; // r31
  for (i = 0; i < 6; ++i) {
    DLdelete(&rv->AP[i]);
  }
  for (i = 0; i < 6; ++i) {
    DLdelete(&rv->C[i]);
  }
  if (rv->preDelayTime != 0) {
    for (i = 0; i < 3; ++i) {
      salFree(rv->preDelayLine[i]);
    }
  }
}

int fh_audio_reverb_std_frames(s32 *left,s32 *right,s32 *surround,unsigned frames,_SND_REVSTD_WORK *rv) {
    if(!left || !right || !surround || !rv || frames>160)return 0;
    for(unsigned i=0;i<6;i++)if(!rv->AP[i].inputs || !rv->C[i].inputs)return 0;
    HandleReverbChannel(left,rv,0,frames);
    HandleReverbChannel(right,rv,1,frames);
    HandleReverbChannel(surround,rv,2,frames);
    return 1;
}
/* DPL2's original stereo mixer discards the returned surround channel. Each
 * original reverb channel has independent delay/filter state, so an immutable
 * DPL2 owner does not need to produce that unused signal. L/R arithmetic and
 * storage are exactly the same as the full source kernel above. */
int fh_audio_reverb_std_stereo_frames(s32 *left,s32 *right,unsigned frames,_SND_REVSTD_WORK *rv) {
    if(!left || !right || !rv || frames>160)return 0;
    for(unsigned i=0;i<4;i++)if(!rv->AP[i].inputs || !rv->C[i].inputs)return 0;
    HandleReverbChannel(left,rv,0,frames);
    HandleReverbChannel(right,rv,1,frames);
    return 1;
}

/* Same four authored rings and allocator alignment as the private constructor.
 * Checked against its actual allocation calls, including zero-length predelay. */
unsigned fh_game_reverb_storage_bytes(float predelay,unsigned channels) {
  if((channels!=2 && channels!=3) || !isfinite(predelay) || predelay<0 || predelay>.1f)return 0;
  static const unsigned lens[4]={1830,2045,443,152};
  unsigned per_channel=0;
  for(unsigned i=0;i<4;i++)per_channel+=((lens[i]+2)*4+7)&~7u;
  unsigned delay=(unsigned)(predelay*(16756991.f/512.f));
  per_channel+=(delay*4+7)&~7u;
  return channels*per_channel;
}
