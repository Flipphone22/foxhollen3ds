#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FH_GX_DISPATCH_BUILD 1
#include "../native-3ds/source/gx_retained_geometry.c"
const FhOwnedSourceLease *fh_owned_active_sources;
unsigned fh_gx_dispatch_enabled;
static unsigned dispatch_begins,dispatch_ends;
int fh_gx_dispatch_begin(const void *const addresses[4],const unsigned bytes[4]){
    dispatch_begins++;fh_retained_reader_scope_begin(addresses,bytes);return 1;
}
void fh_gx_dispatch_end(void){dispatch_ends++;fh_retained_reader_scope_end();}
void fh_gx_dispatch_join_deferred(void){}
void fh_platform_panic(const char *message){fprintf(stderr,"%s\n",message);abort();}
void fh_gx_require(int condition,const char *message){if(!condition){fprintf(stderr,"%s\n",message);abort();}}
void *fh_renderer_retained_alloc(unsigned bytes){return malloc(bytes);}
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
void *fh_memory_copy(void *a,const void *b,size_t n){return memcpy(a,b,n);}

int main(void){
    unsigned char data[256],unrelated[64];
    memset(data,0x5a,sizeof(data));memset(unrelated,0,sizeof(unrelated));
    const void *ranges[4]={data,NULL,NULL,NULL};unsigned sizes[4]={sizeof(data),0,0,0};
    unsigned token=fh_retained_source_acquire(data+64,32);
    assert(token && fh_retained_source_valid(token,data+64,32));
    assert(!fh_retained_model_scope_begin(ranges,sizes));
    fh_retained_model_scope_enabled=1;
    assert(fh_retained_model_scope_begin(ranges,sizes));
    assert(!fh_retained_model_scope_begin(ranges,sizes));
    assert(fh_retained_source_valid(token,data+64,32));
    unsigned verified=source_entries[token&255].verified_scope;
    assert(verified==scope_serial);
    fh_retained_scope_invalidate(unrelated,sizeof(unrelated));
    assert(source_entries[token&255].verified_scope==verified);
    fh_retained_scope_invalidate(data+96,1); /* exactly after the source */
    assert(source_entries[token&255].verified_scope==verified);
    fh_retained_scope_invalidate(data+63,1); /* exactly before the source */
    assert(source_entries[token&255].verified_scope==verified);
    fh_retained_scope_invalidate(data+63,2);
    assert(!source_entries[token&255].verified_scope);
    data[64]^=1;assert(!fh_retained_source_valid(token,data+64,32));
    data[64]^=1;assert(fh_retained_source_valid(token,data+64,32));
    fh_retained_scope_invalidate(NULL,0);
    assert(!source_entries[token&255].verified_scope);
    assert(fh_retained_source_valid(token,data+64,32));
    fh_retained_scope_end();
    data[70]^=1;assert(!fh_retained_source_valid(token,data+64,32));
    data[70]^=1;
    /* Checked mode must detect a caller breaking the immutable contract. */
    fh_retained_scope_validate=1;
    assert(fh_retained_model_scope_begin(ranges,sizes));
    assert(fh_retained_source_valid(token,data+64,32));
    data[71]^=1;assert(!fh_retained_source_valid(token,data+64,32));
    assert(fh_retained_scope_violations==1 && !scope_active);data[71]^=1;
    fh_retained_scope_end();
    /* Serial wrap must not resurrect a verification from an old scope. */
    scope_serial=~0u;
    assert(fh_retained_model_scope_begin(ranges,sizes));
    assert(scope_serial==1 && !source_entries[token&255].verified_scope);
    fh_retained_scope_end();
    /* The identical ownership proof can operate without a worker scope. No
     * extra begin/end or join is permitted, even when the GX worker is on. */
    unsigned begins=dispatch_begins,ends=dispatch_ends;
    fh_gx_dispatch_enabled=2;
    fh_retained_model_scope_enabled=2;
    assert(fh_retained_model_scope_begin(ranges,sizes));
    assert(fh_retained_source_valid(token,data+64,32));
    uint64_t prior_bytes=fh_retained_scope_reused_bytes;
    assert(fh_retained_source_valid(token,data+64,32));
    assert(fh_retained_scope_reused_bytes==prior_bytes+32);
    assert(fh_retained_source_acquire(data+64,32)==token);
    assert(fh_retained_scope_reused_bytes==prior_bytes+64);
    assert(!fh_retained_model_scope_begin(ranges,sizes));
    fh_retained_scope_invalidate(data+64,32);data[70]^=1;
    assert(!fh_retained_source_valid(token,data+64,32));
    unsigned changed_token=fh_retained_source_acquire(data+64,32);
    assert(changed_token && changed_token!=token);
    assert(fh_retained_source_valid(changed_token,data+64,32));data[70]^=1;
    fh_retained_scope_end();
    assert(dispatch_begins==begins && dispatch_ends==ends);
    /* Ending an inactive source scope does not disturb another executor. */
    fh_retained_scope_end();assert(dispatch_ends==ends);
    fh_retained_scope_begin(ranges,sizes);
    assert(dispatch_begins==begins+1);fh_retained_scope_end();
    assert(dispatch_ends==ends+1);
    fh_retained_model_scope_enabled=1;assert(fh_retained_model_scope_begin(ranges,sizes));
    assert(fh_retained_scope_contains(data,sizeof(data)));
    assert(!fh_retained_scope_contains(data+1,sizeof(data)));
    assert(!fh_retained_model_scope_begin(ranges,sizes));
    fh_retained_scope_end();
    fh_gx_dispatch_enabled=0;fh_retained_scope_disabled=1;
    assert(!fh_retained_model_scope_begin(ranges,sizes));
    /* Reader source validation must not consult a later producer range set.
     * Drive the separated sides explicitly; production still joins at end. */
    fh_retained_scope_disabled=0;
    assert(fh_retained_model_scope_begin(ranges,sizes));
    assert(fh_retained_source_valid(token,data+64,32));
    verified=source_entries[token&255].verified_scope;
    producer_ranges[0]=(typeof(producer_ranges[0])){unrelated,sizeof(unrelated)};
    assert(!fh_retained_producer_scope_contains(data+64,32));
    assert(fh_retained_producer_scope_contains(unrelated,sizeof(unrelated)));
    assert(fh_retained_scope_contains(data+64,32));
    assert(fh_retained_source_valid(token,data+64,32) && source_entries[token&255].verified_scope==verified);
    fh_retained_scope_end();assert(!fh_retained_scope_contains(data+64,32));
    /* Overlap filtering is equivalent to invalidating each verified source.
     * Exhaust every boundary around a contained source and outer range. */
    fh_retained_scope_disabled=0;fh_retained_scope_validate=0;
    unsigned checks=0;
    for(unsigned start=0;start<256;start++)for(unsigned length=0;length<260;length++){
        assert(fh_retained_model_scope_begin(ranges,sizes));
        assert(fh_retained_source_valid(token,data+64,32));
        fh_retained_scope_invalidate(data+start,length);
        int overlap=length && start<96 && (start>=64 || length>64-start);
        assert((source_entries[token&255].verified_scope==0)==overlap);
        fh_retained_scope_end();checks++;
    }
    /* Owned readers retain their own version after the original data changes
     * and after producer scopes close. Retained arena replacement must revoke
     * cached verification even if an old owned lease is still alive. */
    unsigned char storage[512] __attribute__((aligned(8)));FhOwnedSources pool;
    FhOwnedSourceLease first={0},second={0},third={0};
    assert(fh_owned_sources_init(&pool,storage,sizeof(storage)));
    assert(fh_owned_source_acquire(&pool,&first,data+64,32));fh_owned_active_sources=&first;
    const void *copy=first.source[0].data;
    unsigned first_token=fh_retained_source_acquire(copy,32);assert(first_token);
    assert(fh_retained_source_valid(first_token,copy,32));uint64_t compared=fh_retained_validation_bytes;
    data[64]^=3;scope_serial++;assert(!scope_active);
    assert(fh_retained_source_valid(first_token,copy,32) && fh_retained_validation_bytes==compared);
    assert(fh_owned_source_acquire(&pool,&second,data+64,32));fh_owned_active_sources=&second;
    assert(!fh_retained_source_valid(first_token,second.source[0].data,32));
    unsigned second_token=fh_retained_source_acquire(second.source[0].data,32);assert(second_token);
    assert(fh_retained_source_valid(second_token,second.source[0].data,32));
    data[64]^=5;assert(fh_owned_source_acquire(&pool,&third,data+64,32));fh_owned_active_sources=&third;
    unsigned third_token=fh_retained_source_acquire(third.source[0].data,32);assert(third_token && third_token!=second_token);
    fh_owned_active_sources=&first;assert(!fh_retained_source_valid(first_token,copy,32));
    fh_owned_active_sources=NULL;
    assert(fh_owned_source_release(&pool,&first));assert(fh_owned_source_release(&pool,&second));assert(fh_owned_source_release(&pool,&third));
    /* File-backed geometry versions must agree with the actual retained-source
     * cache, including mutation notifications outside any render scope. */
    fh_model_resources_mode=2;
    FhModelResourceRange resource={data,sizeof(data)};
    fh_model_resources_register(data,sizeof(data),&resource,1);
    unsigned model_token=fh_retained_source_acquire(data+64,32);
    assert(model_token && fh_retained_source_valid(model_token,data+64,32));
    unsigned resource_checks=fh_model_resources_checks;
    assert(fh_retained_source_valid(model_token,data+64,32));
    assert(fh_model_resources_checks==resource_checks+1);
    data[65]^=7;fh_retained_scope_invalidate(data+65,1);
    assert(!fh_retained_source_valid(model_token,data+64,32));
    model_token=fh_retained_source_acquire(data+64,32);
    assert(model_token && fh_retained_source_valid(model_token,data+64,32));
    fh_model_resources_escape(data);
    assert(!fh_model_resource_version(data+64,32));
    data[65]^=2; /* An escaped writer no longer promises notifications. */
    assert(!fh_retained_source_valid(model_token,data+64,32));
    fh_model_resources_register(data,sizeof(data),&resource,1);
    assert(!fh_model_resource_version(data+64,32));
    fh_model_resources_forget(data);
    fh_model_resources_register(data,sizeof(data),&resource,1);
    assert(fh_model_resource_version(data+64,32));
    assert(!fh_retained_source_valid(model_token,data+64,32));
    model_token=fh_retained_source_acquire(data+64,32);
    assert(model_token && fh_retained_source_valid(model_token,data+64,32));
    fh_model_resources_forget(data);
    data[65]^=4;
    assert(!fh_retained_source_valid(model_token,data+64,32));
    free(arena);arena=NULL;
    printf("%u overlap cases, owned versions, model resource mutation/escape/release, replacement, worker admission and serial wrap passed\n",checks);
}
