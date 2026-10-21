#include <dolphin/ai.h>
#include <dolphin/dvd.h>
#include <musyx/pc.h>
#include <string.h>
#include <stdio.h>
#include "fh_platform.h"
#include "fh_audio_profile.h"
#include "fh_audio_worker.h"
#include "fh_audio_controls.h"
#include "fh_audio_cache.h"
#include "fh_audio_timeline.h"
#include "fh_audio_game_dsp.h"
#include "fh_audio_cost.h"
extern void fh_platform_audio_native_initialize(void);
#include "main/dll/FRONT/attract_movie.h"
uint64_t fh_audio_profile_ticks[4];
uint64_t fh_audio_pump_ticks;

enum { MIX_FRAMES=160, STREAM_BUFFER=32768, STREAM_BLOCKS=54 };
static AIDCallback dma_callback;
static AISCallback stream_callback;
static uintptr_t dma_address;
static u32 dma_bytes,dma_running,dsp_rate,stream_state,stream_count,stream_trigger;
static u8 volume_left=255,volume_right=255;
static DVDFileInfo stream_file;
static u32 stream_read_offset,stream_buffer_offset,stream_buffer_used;
static u8 compressed[STREAM_BUFFER];
static s32 history[2][2];
static s16 mix_buffers[4][MIX_FRAMES*2];
static unsigned mix_index;
static FhAudioTimeline source_timeline;
static FhAudioMap dma_map;
static unsigned source_timeline_bound,source_generation;
static uint64_t source_next_frame;
extern void fh_platform_audio_queue_mapped(unsigned,const int16_t *,unsigned,unsigned,const FhAudioMap *);
static void source_bind(void) {
    if(source_timeline_bound)return;
    extern char gAttractMovieAudioDmaBuffer[];
    const int16_t *buffers[FH_AUDIO_MAP_BUFFERS]={mix_buffers[0],mix_buffers[1],mix_buffers[2],mix_buffers[3],
        (const int16_t *)gAttractMovieAudioDmaBuffer,
        (const int16_t *)(gAttractMovieAudioDmaBuffer+ATTRACT_MOVIE_AUDIO_DMA_BUFFER_SIZE)};
    if(!fh_audio_timeline_bind(&source_timeline,buffers))fh_platform_panic("Audio source buffers invalid");
    source_timeline_bound=1;
}
static void source_copy(short *destination,const short *source,unsigned frames) {
    source_bind();
    if(!fh_audio_timeline_copy(&source_timeline,destination,source,frames))fh_platform_panic("Unknown audio mix destination");
}
static void source_packet(const short *source,unsigned offset,unsigned frames) {
    source_bind();
    if(!fh_audio_map_append(&dma_map,fh_audio_timeline_read(&source_timeline,source,frames),offset,frames))
        fh_platform_panic("Audio source packet mapping overflow");
}
static void source_submit(unsigned frames,unsigned rate);
/* One native synth block may run on core 2 while the movie callback and GX
 * stay on main. The recursive synth lock protects this slot and both rings.
 * A synth API entry drains an unstarted job before changing synth state. */
unsigned fh_audio_synth_jobs_enabled;
unsigned fh_audio_synth_jobs_queued,fh_audio_synth_jobs_worker,fh_audio_synth_jobs_inline;
unsigned fh_audio_synth_jobs_urgent;
static unsigned synth_bulk;
static s16 *pending_synth;
extern void AttractMovieAudio_DmaCallback(void);
static s16 dma_pcm[2048*2];
static void source_submit(unsigned frames,unsigned rate) {
    if(!fh_audio_map_valid(&dma_map,frames))fh_platform_panic("Audio source packet mapping invalid");
    fh_platform_audio_queue_mapped(0,dma_pcm,frames,rate,&dma_map);
    dma_map.count=0;
}
static unsigned dma_pcm_frames,dma_pcm_slices,dma_pcm_rate;
static void render_mix(s16 *next);
static void pipeline_drain(void);
static int mix_pending(unsigned limit,unsigned slice_budget);
static int finish_synth(unsigned worker) {
    s16 *next=pending_synth;if(!next)return 0;
    pending_synth=NULL; /* sndPCRender recursively enters the synth lock. */
    if(worker)__atomic_add_fetch(&fh_audio_synth_jobs_worker,1,__ATOMIC_RELAXED);
    else __atomic_add_fetch(&fh_audio_synth_jobs_inline,1,__ATOMIC_RELAXED);
    render_mix(next);return 1;
}
void fh_audio_drain_pending(void) {fh_audio_lock();fh_audio_controls_drain();pipeline_drain();finish_synth(0);fh_audio_unlock();}
/* Both rings are CPU PCM, never GX texture/vertex sources. NDSP consumes an
 * owned linear copy whose actual flush remains in platform_audio.c. Restrict
 * this exemption to movie-callback notifications and entire owned ranges. */
extern char gAttractMovieAudioDmaBuffer[];
unsigned fh_audio_cpu_cache_enabled,fh_audio_cpu_cache_hits,fh_audio_cpu_cache_fallbacks;
static void audio_dma_cache(void *memory,uint32_t bytes,int written){
    if(fh_audio_cpu_cache_enabled &&
       (fh_audio_range_contains(mix_buffers,sizeof(mix_buffers),memory,bytes) ||
        fh_audio_range_contains(gAttractMovieAudioDmaBuffer,ATTRACT_MOVIE_AUDIO_DMA_BUFFER_BYTES,memory,bytes))){
        fh_audio_cpu_cache_hits++;return;
    }
    fh_audio_cpu_cache_fallbacks++;fh_gx_cache_range(memory,bytes,written);
}
void fh_audio_dma_flush(void *memory,uint32_t bytes){audio_dma_cache(memory,bytes,1);}
void fh_audio_dma_invalidate(void *memory,uint32_t bytes){audio_dma_cache(memory,bytes,0);}

static int pumping,critical;
static unsigned music_blocks;
static int worker_step(void);
static uint64_t queue_ticks,queue_peak;
extern void salPCExternalEnter(void),salPCExternalLeave(void);
static void enter(void) { fh_audio_lock();if(!critical){pipeline_drain();finish_synth(0);}++critical; }
static void leave(void) { if(!critical) fh_platform_panic("Unbalanced audio lock");--critical;fh_audio_unlock(); }
void hwInitIrq(void) { salPCExternalEnter(); }
void hwExitIrq(void) {}
void hwDisableIrq(void) { salPCExternalEnter(); }
void hwEnableIrq(void) { salPCExternalLeave(); }
void hwIRQEnterCritical(void) { salPCExternalEnter(); }
void hwIRQLeaveCritical(void) { salPCExternalLeave(); }
void sndPCStopAudio(void) { AIStopDMA(); }
void fhMusyxConfigure(void) {
    SND_PC_CONFIG config={32000,2};
    if(!sndPCSetSynchronization(enter,leave) || !sndPCConfigure(&config)) fh_platform_panic("MusyX output configuration rejected");
    fh_platform_audio_native_initialize();
}
static void render_mix(s16 *next) {
    static unsigned samples;static uint64_t total,peak;
    uint64_t cost_start=fh_audio_cost_now();
    uint64_t start=fh_platform_ticks();
    source_bind();
    SND_PC_RENDER_INFO info=sndPCGetRenderInfo();
    if(info.mixRate!=32000 || info.channels!=2 || info.outputFrames>UINT64_MAX-MIX_FRAMES)
        fh_platform_panic("Audio source timeline format changed");
    if(!source_generation || info.outputFrames!=source_next_frame) {
        if(source_generation==UINT32_MAX)fh_platform_panic("Audio source generation overflow");
        source_generation++;
    }
    fh_platform_audio_native_initialize();
    fh_audio_game_dsp_prepare(info.outputFrames,source_generation);
    uint32_t effect_token=fh_audio_game_dsp_effect_token();
    if(!sndPCRender(next,MIX_FRAMES)) fh_platform_panic("MusyX failed to render PCM");
    if(!fh_audio_timeline_stamp_effect(&source_timeline,next,MIX_FRAMES,info.outputFrames,source_generation,effect_token))
        fh_platform_panic("Audio source render destination invalid");
    source_next_frame=info.outputFrames+MIX_FRAMES;
    fh_audio_cost_finish(FH_AC_PRODUCER,cost_start);
    uint64_t elapsed=fh_platform_ticks()-start;total+=elapsed;if(elapsed>peak)peak=elapsed;
    if(++samples%256==0) {
        extern volatile u32 fh_audio_voice_starts,fh_audio_voice_stops;
        char message[200];snprintf(message,sizeof(message),"audio mix blocks=%u avg_us=%lu max_us=%lu queue_us=%lu queue_max=%lu starts=%lu stops=%lu\n",samples,(unsigned long)(total/256*1000000/40500000),(unsigned long)(peak*1000000/40500000),(unsigned long)(queue_ticks/256*1000000/40500000),(unsigned long)(queue_peak*1000000/40500000),(unsigned long)fh_audio_voice_starts,(unsigned long)fh_audio_voice_stops);
        fh_platform_log(message);total=peak=queue_ticks=queue_peak=0;
        if(fh_audio_profile_ticks[0] || fh_audio_profile_ticks[3]) {
            snprintf(message,sizeof(message),"audio sampled stages stride=16 decode_us=%lu itd_us=%lu gain_us=%lu effects_us=%lu\n",(unsigned long)(fh_audio_profile_ticks[0]/256*1000000/40500000),(unsigned long)(fh_audio_profile_ticks[1]/256*1000000/40500000),(unsigned long)(fh_audio_profile_ticks[2]/256*1000000/40500000),(unsigned long)(fh_audio_profile_ticks[3]/256*1000000/40500000));
            fh_platform_log(message);memset(fh_audio_profile_ticks,0,sizeof(fh_audio_profile_ticks));
        }
    }
}

/* All access is under the existing recursive synth lock. Commands preserve the
 * original DMA order, including copies preceding callbacks and the two-buffer
 * synth delay. Main never reads queued PCM; only the worker/explicit drain does.
 * Movie samples are snapshotted after the original main-thread volume/queue
 * updates. Saturating addition occurs once, at the original mix boundary. */
enum { PIPE_COMMANDS=64,PIPE_SAMPLES=160,PIPE_SYNTH=1,PIPE_COPY,PIPE_MOVIE,PIPE_SUBMIT };
typedef struct {unsigned type,count,rate;short *dest;const short *source;
    short movie[PIPE_SAMPLES*2];} AudioCommand;
static AudioCommand pipeline[PIPE_COMMANDS];
static unsigned pipeline_read,pipeline_write,pipeline_reserved,pipeline_executing,pipeline_building;
static unsigned pipeline_callback_owned;
unsigned fh_audio_pipeline_enabled,fh_audio_pipeline_worker,fh_audio_pipeline_inline;
unsigned fh_audio_pipeline_packets,fh_audio_pipeline_peak;
static int pipeline_step(unsigned worker){
    if(pipeline_read==pipeline_write || pipeline_executing)return 0;
    AudioCommand *c=&pipeline[pipeline_read%PIPE_COMMANDS];pipeline_executing=1;
    switch(c->type){
    case PIPE_SYNTH:render_mix(c->dest);break;
    case PIPE_COPY:
        source_packet(c->source,(unsigned)(c->dest-dma_pcm)/2,c->count);
        memcpy(c->dest,c->source,c->count*4);break;
    case PIPE_MOVIE:
        source_copy(c->dest,c->source,c->count);
        for(unsigned i=0;i<c->count*2;i++){
            int value=(c->source?c->source[i]:0)+(int)c->movie[i];
            c->dest[i]=value<-32768?-32768:value>32767?32767:value;
        }
        break;
    case PIPE_SUBMIT:
        source_submit(c->count,c->rate);
        if(!pipeline_reserved)fh_platform_panic("Audio pipeline reservation underflow");
        pipeline_reserved--;__atomic_add_fetch(&fh_audio_pipeline_packets,1,__ATOMIC_RELAXED);break;
    default:fh_platform_panic("Invalid audio pipeline command");
    }
    pipeline_read++;pipeline_executing=0;
    if(worker)__atomic_add_fetch(&fh_audio_pipeline_worker,1,__ATOMIC_RELAXED);
    else __atomic_add_fetch(&fh_audio_pipeline_inline,1,__ATOMIC_RELAXED);
    return c->type;
}
static void pipeline_drain(void){while(pipeline_step(0)){} }
static AudioCommand *pipeline_push(unsigned type,short *dest,const short *source,unsigned count,unsigned rate){
    if(pipeline_executing)fh_platform_panic("Recursive audio pipeline publication");
    if(pipeline_write-pipeline_read==PIPE_COMMANDS)pipeline_drain();
    AudioCommand *c=&pipeline[pipeline_write++%PIPE_COMMANDS];
    c->type=type;c->dest=dest;c->source=source;c->count=count;c->rate=rate;
    unsigned pending=pipeline_write-pipeline_read;if(pending>fh_audio_pipeline_peak)fh_audio_pipeline_peak=pending;
    return c;
}
void fh_audio_movie_mix(short *destination,short *source,unsigned samples,
    void (*mix)(short *,short *,unsigned)){
    if(!pipeline_building){fh_audio_lock();pipeline_drain();source_copy(destination,source,samples);mix(destination,source,samples);fh_audio_unlock();return;}
    if(samples!=PIPE_SAMPLES)fh_platform_panic("Unexpected movie audio packet size");
    short movie[PIPE_SAMPLES*2];mix(movie,NULL,samples);
    AudioCommand *c=pipeline_push(PIPE_MOVIE,destination,source,samples,0);
    memcpy(c->movie,movie,sizeof(movie));
}
static void mix_callback(void) {
    if(pending_synth)fh_platform_panic("Synth buffer reused before completion");
    mix_index=(mix_index+1)%4;
    AIInitDMA((uintptr_t)mix_buffers[mix_index],sizeof(mix_buffers[0]));
    s16 *next=mix_buffers[(mix_index+2)%4];
    if(pipeline_building){pipeline_push(PIPE_SYNTH,next,NULL,0,0);return;}
    if(fh_audio_synth_jobs_enabled && fh_audio_worker_active() && dma_callback==AttractMovieAudio_DmaCallback && !critical && !synth_bulk){
        pending_synth=next;__atomic_add_fetch(&fh_audio_synth_jobs_queued,1,__ATOMIC_RELAXED);
        fh_audio_worker_wake();
    }else render_mix(next);
}
void fhMusyxStartOutput(void) {
    fh_audio_lock();pipeline_drain();finish_synth(0);memset(mix_buffers,0,sizeof(mix_buffers));mix_index=1;
    source_bind();
    for(unsigned i=0;i<4;i++)if(!fh_audio_timeline_stamp(&source_timeline,mix_buffers[i],MIX_FRAMES,0,0))
        fh_platform_panic("Audio source silence mapping invalid");
    AIRegisterDMACallback(mix_callback);
    AIInitDMA((uintptr_t)mix_buffers[mix_index],sizeof(mix_buffers[0]));AIStartDMA();fh_audio_unlock();fh_audio_worker_start(worker_step);
}
void fhMusyxStopOutput(void) { AIRegisterDMACallback(NULL);AIStopDMA(); }
void AIInit(u8 *stack) { (void)stack; }
void AIReset(void) { AIStopDMA();AISetStreamPlayState(AI_STREAM_STOP); }
AIDCallback AIRegisterDMACallback(AIDCallback callback) { fh_audio_lock();pipeline_drain();finish_synth(0);AIDCallback old=dma_callback;
    /* A normal synth packet can now span worker locks. Complete it under its
     * original callback before handing DMA ownership to the movie pipeline. */
    if(callback!=old && dma_running && dma_pcm_slices && old==mix_callback){
        mix_pending(1,~0u);
        if(dma_pcm_slices)fh_platform_panic("Audio callback handoff retained a partial packet");
    }
    pipeline_callback_owned=callback==AttractMovieAudio_DmaCallback &&
        (old==mix_callback || (old==callback && pipeline_callback_owned));
    dma_callback=callback;fh_audio_unlock();return old; }
void AIInitDMA(uintptr_t address,u32 bytes) {
    if(!address || !bytes || bytes%4 || bytes>8192) fh_platform_panic("Invalid audio DMA buffer");
    fh_audio_lock();dma_address=address;dma_bytes=bytes;fh_audio_unlock();
}
uintptr_t fhAIGetDMAStartAddr(void) { fh_audio_lock();uintptr_t value=dma_address;fh_audio_unlock();return value; }
u32 AIGetDMAStartAddr(void) { return fhAIGetDMAStartAddr(); }
u32 AIGetDMALength(void) { fh_audio_lock();u32 value=dma_bytes;fh_audio_unlock();return value; }
u32 AIGetDMABytesLeft(void) { return AIGetDMALength(); }
BOOL AIGetDMAEnableFlag(void) { fh_audio_lock();BOOL value=dma_running!=0;fh_audio_unlock();return value; }
BOOL AICheckInit(void) { return 1; }
void AIStartDMA(void) { fh_audio_lock();dma_running=1;fh_platform_audio_pause(0,0);fh_audio_unlock(); }
void AIStopDMA(void) { fh_audio_lock();pipeline_drain();finish_synth(0);dma_running=0;dma_pcm_frames=dma_pcm_slices=0;dma_map.count=0;fh_platform_audio_reset(0);fh_audio_unlock(); }
void AISetDSPSampleRate(u32 rate) { fh_audio_lock();pipeline_drain();dsp_rate=rate;fh_audio_unlock(); }
u32 AIGetDSPSampleRate(void) { fh_audio_lock();u32 value=dsp_rate;fh_audio_unlock();return value; }
static s32 clamp(s32 value,s32 lo,s32 hi) { return value<lo?lo:value>hi?hi:value; }
static s16 decode(unsigned nibble,unsigned scale,s32 *hist) {
    s32 prediction=0;
    switch(scale>>4) {
    case 1: prediction=hist[0]*60;break;
    case 2: prediction=hist[0]*115-hist[1]*52;break;
    case 3: prediction=hist[0]*98-hist[1]*55;break;
    }
    prediction=clamp((prediction+32)>>6,-0x200000,0x1fffff);
    s32 signed_bits=nibble>=8?(s32)nibble-16:(s32)nibble;
    s32 value=((signed_bits*4096)>>(scale&15))*64+prediction;
    hist[1]=hist[0];hist[0]=value;
    return clamp(value>>6,-32768,32767);
}
int fhAIPrepareStream(void *file_pointer) {
    DVDFileInfo *file=file_pointer;
    if(!file || !file->length || file->length%32) return 0;
    stream_file=*file;stream_read_offset=stream_buffer_offset=stream_buffer_used=stream_count=0;
    extern void fh_dvd_stream_prepare(DVDFileInfo *);
    fh_dvd_stream_prepare(&stream_file);
    memset(history,0,sizeof(history));stream_state=AI_STREAM_STOP;fh_platform_audio_reset(1);return 1;
}
static const u8 *stream_block(void) {
    if(stream_buffer_offset==stream_buffer_used) {
        if(stream_read_offset==stream_file.length) return NULL;
        unsigned bytes=stream_file.length-stream_read_offset;if(bytes>STREAM_BUFFER) bytes=STREAM_BUFFER;
        if(DVDReadPrio(&stream_file,compressed,bytes,stream_read_offset,2)!=(s32)bytes) fh_platform_panic("Stream audio disc read failed");
        stream_read_offset+=bytes;stream_buffer_offset=0;stream_buffer_used=bytes;
    }
    const u8 *block=compressed+stream_buffer_offset;stream_buffer_offset+=32;return block;
}
static int mix_pending(unsigned limit,unsigned slice_budget) {
    unsigned before=music_blocks;
    unsigned slices=0;
    if(fh_audio_pipeline_enabled && pipeline_callback_owned && fh_audio_worker_active() && dma_callback==AttractMovieAudio_DmaCallback){
        /* Reserve at most two queued 20ms output packets. They count against
         * the same eight NDSP slots, rather than silently growing latency. */
        unsigned buffered=fh_platform_audio_buffered(0)/640;
        pipeline_building=1;
        for(unsigned n=0;n<limit && dma_running && pipeline_reserved<2 && buffered+pipeline_reserved<8;n++){
            if(dma_pcm_frames || dma_pcm_slices || pending_synth)fh_platform_panic("Audio pipeline entered with partial legacy packet");
            pipeline_reserved++;
            unsigned frames=0;
            for(unsigned slice=0;slice<4;slice++){
                if(!dma_address || dma_bytes!=640)fh_platform_panic("Audio pipeline DMA contract changed");
                pipeline_push(PIPE_COPY,dma_pcm+frames*2,(const short*)dma_address,160,0);frames+=160;
                dma_callback();++music_blocks;
            }
            pipeline_push(PIPE_SUBMIT,NULL,NULL,frames,dsp_rate==AI_SAMPLERATE_48KHZ?48000:32000);
        }
        pipeline_building=0;fh_audio_worker_wake();return music_blocks!=before;
    }
    pipeline_drain();
    for(unsigned n=0;n<limit && dma_running && fh_platform_audio_space(0);n++) {
        /* Keep six complete packets beyond the playing packet.
         * Long game/SD phases can delay the producer. Refill synchronously
         * before underrun instead of claiming speed by losing audio. */
        synth_bulk=fh_audio_synth_jobs_enabled && dma_callback==AttractMovieAudio_DmaCallback && fh_platform_audio_buffered(0)<7*640;
        if(synth_bulk){__atomic_add_fetch(&fh_audio_synth_jobs_urgent,1,__ATOMIC_RELAXED);finish_synth(0);}
        if(pending_synth)break;
        if(!dma_pcm_frames)dma_pcm_rate=dsp_rate;
        for(;dma_pcm_slices<4 && dma_running && dsp_rate==dma_pcm_rate && slices<slice_budget;) {
            if(!dma_address || !dma_bytes) fh_platform_panic("Audio started without DMA buffer");
            unsigned count=dma_bytes/4;if(dma_pcm_frames+count>2048)break;
            source_packet((const s16*)dma_address,dma_pcm_frames,count);
            memcpy(dma_pcm+dma_pcm_frames*2,(const s16*)dma_address,dma_bytes);dma_pcm_frames+=count;dma_pcm_slices++;
            if(dma_callback)dma_callback();
            ++music_blocks;++slices;
            if(pending_synth)break;
        }
        if(!dma_running)break;
        /* Publish the same four-slice packet as the synchronous route. A
         * suspended packet owns its copy; no game/DMA source is borrowed. */
        if((pending_synth || slices==slice_budget) && dma_pcm_slices<4)break;
        uint64_t start=fh_platform_ticks();
        source_submit(dma_pcm_frames,dma_pcm_rate==AI_SAMPLERATE_48KHZ?48000:32000);
        dma_pcm_frames=dma_pcm_slices=0;
        uint64_t elapsed=fh_platform_ticks()-start;queue_ticks+=elapsed;if(elapsed>queue_peak)queue_peak=elapsed;
    }
    synth_bulk=0;return music_blocks!=before;
}
static int worker_step(void) {
    /* Called with the synth lock held. THP/movie callbacks can touch main
     * thread decode state, so their complete DMA chain remains synchronous. */
    if(critical)return 0;
    if(pipeline_read!=pipeline_write){
        /* Retire cheap copies/mixes together, yielding after one synth block
         * so game API mutations can acquire the lock between blocks. */
        for(unsigned i=0;i<PIPE_COMMANDS;i++){int kind=pipeline_step(1);if(!kind || kind==PIPE_SYNTH)break;}
        return 1;
    }
    if(finish_synth(1))return 1;
    /* One 5ms synth slice per lock acquisition. Keep the same four-slice
     * output packet in dma_pcm, publishing only when complete. Main-thread
     * synth API calls can now run between slices instead of waiting for an
     * entire 20ms output packet to be computed under the recursive lock. */
    return dma_callback==mix_callback?mix_pending(1,1):0;
}
void fh_audio_pump(void) {
    if(pumping)return;
    static unsigned calls,blocks;static uint64_t elapsed_total;
    uint64_t pump_start=fh_platform_ticks();
    pumping=1;
    int locked=fh_audio_worker_active()?fh_audio_try_lock():(fh_audio_lock(),1);
    if(locked) {
        if(critical){fh_audio_unlock();pumping=0;return;}
        unsigned before=music_blocks;
        if(!fh_audio_worker_active() || dma_callback!=mix_callback)mix_pending(8,~0u);
        blocks+=music_blocks-before;fh_audio_unlock();
    }
    for(unsigned n=0;n<8 && stream_state==AI_STREAM_START && fh_platform_audio_space(1);n++) {
        s16 pcm[STREAM_BLOCKS*28*2];unsigned frames=0;
        for(unsigned b=0;b<STREAM_BLOCKS;b++) {
            const u8 *block=stream_block();if(!block) { stream_state=AI_STREAM_STOP;break; }
            for(unsigned i=0;i<28;i++) {
                unsigned packed=block[4+i];
                pcm[(frames+i)*2]=(decode(packed&15,block[0],history[0])*(s32)volume_left)/255;
                pcm[(frames+i)*2+1]=(decode(packed>>4,block[1],history[1])*(s32)volume_right)/255;
            }
            frames+=28;stream_count+=28;
        }
        if(frames) fh_platform_audio_queue(1,pcm,frames,48043);
        if(stream_callback && stream_trigger && stream_count>=stream_trigger) stream_callback(stream_count);
    }
    if(stream_state==AI_STREAM_STOP) fh_platform_audio_finish(1);
    pumping=0;
    uint64_t elapsed=fh_platform_ticks()-pump_start;
    elapsed_total+=elapsed;fh_audio_pump_ticks+=elapsed;
    if(++calls%512==0) {
        char line[128];snprintf(line,sizeof(line),"audio pump calls=%u blocks=%u total_us=%llu\n",calls,blocks,(unsigned long long)(elapsed_total*1000000/40500000));
        fh_platform_log(line);blocks=0;elapsed_total=0;
    }
}
void fhAIPump(void) { fh_audio_pump(); }
AISCallback AIRegisterStreamCallback(AISCallback callback) { AISCallback old=stream_callback;stream_callback=callback;return old; }
void AISetStreamPlayState(u32 state) {
    stream_state=state;fh_platform_audio_pause(1,state!=AI_STREAM_START);
    if(state!=AI_STREAM_START) fh_platform_audio_reset(1);
}
u32 AIGetStreamPlayState(void) { return stream_state; }
u32 AIGetStreamSampleCount(void) { return stream_count; }
void AIResetStreamSampleCount(void) { stream_count=0; }
void AISetStreamTrigger(u32 trigger) { stream_trigger=trigger; }
u32 AIGetStreamTrigger(void) { return stream_trigger; }
void AISetStreamVolLeft(u8 value) { volume_left=value; }
void AISetStreamVolRight(u8 value) { volume_right=value; }
u8 AIGetStreamVolLeft(void) { return volume_left; }
u8 AIGetStreamVolRight(void) { return volume_right; }
void AISetStreamSampleRate(u32 rate) { if(rate!=AI_SAMPLERATE_48KHZ) fh_platform_panic("Unsupported stream audio rate"); }
u32 AIGetStreamSampleRate(void) { return AI_SAMPLERATE_48KHZ; }
