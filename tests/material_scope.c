#include "fh_material_scope.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FhDrawState state,other;
static FhMaterial backing;
static unsigned calls,action,classification_calls;
void fh_platform_panic(const char *message){fprintf(stderr,"%s\n",message);exit(42);}
int fh_fur_material(const FhDrawState *draw){++classification_calls;return draw->stages==9;}
static const FhMaterial *oracle(const FhDrawState *draw,const FhVertex *vertices,unsigned count,int probe){
    (void)vertices;(void)count;++calls;
    if(action==1){action=0;fh_renderer_material_scope_invalidate();}
    if(action==2){action=0;unsigned nested=fh_renderer_material_scope_begin(draw);fh_renderer_material_scope_end(&nested);}
    if(probe && backing.cpu_fragment==1)return NULL;
    return &backing;
}
static const FhMaterial *get(int probe){return fh_material_scope_get(&state,NULL,0,probe,oracle);}
static void early_return(void){
    unsigned entered __attribute__((cleanup(fh_renderer_material_scope_end)))=fh_renderer_material_scope_begin(&state);
    assert(get(1));return;
}
static void *invalidate_thread(void *unused){
    (void)unused;for(unsigned i=0;i<100000;++i)fh_renderer_material_scope_invalidate();return NULL;
}
int main(int argc,char **argv){
    fh_material_scope_mode=0;unsigned entered=fh_renderer_material_scope_begin(&state);
    assert(!entered);get(1);get(1);assert(calls==2);fh_renderer_material_scope_end(&entered);
    fh_material_scope_mode=1;
    entered=fh_renderer_material_scope_begin(&state);backing.map=7;
    const FhMaterial *owned=get(1);assert(owned!=&backing && owned->map==7);
    unsigned before=calls;backing.map=13;assert(get(0)==owned && get(1)->map==7 && calls==before);
    assert(fh_material_scope_get(&other,NULL,0,0,oracle)==&backing && calls==before+1);
    assert(get(1)->map==7);fh_renderer_material_scope_end(&entered);
    assert(get(1)==&backing && get(1)->map==13);
    backing.cpu_fragment=1;entered=fh_renderer_material_scope_begin(&state);
    before=calls;assert(!get(1) && !get(1) && calls==before+1);
    assert(get(0)->cpu_fragment==1);before=calls;assert(!get(1) && get(0) && calls==before);
    fh_renderer_material_scope_end(&entered);
    for(unsigned fragment=0;fragment<=2;fragment+=2){
        backing.cpu_fragment=fragment;entered=fh_renderer_material_scope_begin(&state);
        before=calls;assert(get(0) && get(1) && calls==before+1);fh_renderer_material_scope_end(&entered);
    }
    entered=fh_renderer_material_scope_begin(&state);get(1);
    unsigned nested=fh_renderer_material_scope_begin(&state);before=calls;get(1);get(1);assert(calls==before+2);
    fh_renderer_material_scope_end(&nested);before=calls;get(1);get(1);assert(calls==before+2);
    fh_renderer_material_scope_end(&entered);early_return();entered=fh_renderer_material_scope_begin(&state);
    before=calls;get(1);get(1);assert(calls==before+1);fh_renderer_material_scope_end(&entered);
    for(unsigned callback=1;callback<=2;++callback){
        entered=fh_renderer_material_scope_begin(&state);action=callback;before=calls;
        assert(get(1)==&backing && get(1)==&backing && calls==before+2);
        fh_renderer_material_scope_end(&entered);
    }
    fh_material_scope_mode=2;entered=fh_renderer_material_scope_begin(&state);get(1);
    if(argc>1 && !strcmp(argv[1],"material-mutation")){backing.map++;get(1);return 1;}
    before=calls;get(1);get(0);assert(calls==before+2 && fh_material_scope_checks==2);
    state.stages=9;assert(fh_material_scope_fur(&state));
    if(argc>1 && !strcmp(argv[1],"class-mutation")){state.stages=8;fh_material_scope_fur(&state);return 1;}
    assert(fh_material_scope_fur(&state));assert(classification_calls==2 && fh_material_scope_class_checks==1);
    action=1;assert(get(1)==&backing);fh_renderer_material_scope_end(&entered);
    fh_material_scope_mode=1;pthread_t thread;assert(!pthread_create(&thread,NULL,invalidate_thread,NULL));
    for(unsigned i=0;i<100000;++i){entered=fh_renderer_material_scope_begin(&state);assert(get(1));assert(get(0));fh_renderer_material_scope_end(&entered);}
    assert(!pthread_join(thread,NULL));puts("material scope ownership, fallback, invalidation, checked oracle and concurrent invalidation passed");
}
