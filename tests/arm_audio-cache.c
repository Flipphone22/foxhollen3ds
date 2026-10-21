/* Exercise the production ownership filter, including its actual private ring. */
#include "../native-3ds/source/audio.c"
volatile unsigned test_checks;
__attribute__((noinline)) void test_fail(unsigned line){(void)line;for(;;){}}
__attribute__((noinline)) void test_pass(void){for(;;){}}
#define CHECK(x) do{test_checks++;if(!(x))test_fail(__LINE__);}while(0)
char gAttractMovieAudioDmaBuffer[ATTRACT_MOVIE_AUDIO_DMA_BUFFER_BYTES+12];
static unsigned forwards;static const void *forward_address;static size_t forward_bytes;static int forward_written;
void fh_gx_cache_range(void *p,size_t n,int written){forwards++;forward_address=p;forward_bytes=n;forward_written=written;}
void test_main(void){
    const void *bases[]={mix_buffers,gAttractMovieAudioDmaBuffer};
    unsigned lengths[]={sizeof(mix_buffers),ATTRACT_MOVIE_AUDIO_DMA_BUFFER_BYTES};
    for(unsigned enabled=0;enabled<2;enabled++){
        fh_audio_cpu_cache_enabled=enabled;
        for(unsigned ring=0;ring<2;ring++)for(int off=-8;off<=(int)lengths[ring]+8;off++){
            unsigned counts[]={0,1,4,640,1280,0xffffffffu};
            for(unsigned i=0;i<6;i++){
                void *p=(void*)((uintptr_t)bases[ring]+off);unsigned n=counts[i],before=forwards;
                unsigned owned=0;
                for(unsigned b=0;b<2;b++)owned|=n && (uintptr_t)p>=(uintptr_t)bases[b] && (uint64_t)(uintptr_t)p+n<=(uint64_t)(uintptr_t)bases[b]+lengths[b];
                fh_audio_dma_flush(p,n);CHECK(forwards-before==!(enabled && owned));
                if(forwards!=before){CHECK(forward_address==p && forward_bytes==n && forward_written==1);}
                before=forwards;fh_audio_dma_invalidate(p,n);CHECK(forwards-before==!(enabled && owned));
                if(forwards!=before)CHECK(forward_written==0);
            }
        }
    }
    CHECK(!fh_audio_range_contains((void*)0xfffffff0,16,(void*)0xfffffff8,16));
    CHECK(!fh_audio_range_contains((void*)4,16,NULL,4));
    CHECK(!fh_audio_range_contains(NULL,16,(void*)4,4));
    test_pass();
}
