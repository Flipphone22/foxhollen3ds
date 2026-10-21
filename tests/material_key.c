#include "fh_material_key.h"
#include <assert.h>
#include <stdio.h>
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
void *fh_memory_copy(void *a,const void *b,size_t n){return memcpy(a,b,n);}
static unsigned rng=71,checks;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
/* Exact pre-change contiguous serialization, kept separate from view traversal. */
static unsigned original(unsigned char *key,const FhDrawState *state){
    FhRegisterMaterial registers;unsigned used=0;
    if(fh_material_register_key(&registers,state)){memcpy(key,&registers,sizeof(registers));return sizeof(registers);}
    unsigned active_bytes=state->stages*sizeof(FhTevStage);memcpy(key,state->tev,active_bytes);used+=active_bytes;
    memcpy(key+used,state->tev_colors,128);used+=128;
    memcpy(key+used,state->swaps,72);used+=72;
    memcpy(key+used,&state->constant_raster_mask,36);used+=36;
    for(unsigned i=0;i<8;i++){
        memcpy(key+used,&state->textures[i].width,12);used+=12;
        memcpy(key+used,&state->textures[i].wrap_s,16);used+=16;
    }
    unsigned raster[]={state->blend_mode,state->alpha_write,state->color_write,state->depth_test,state->depth_write,state->depth_compare};
    memcpy(key+used,raster,sizeof(raster));used+=sizeof(raster);
    memcpy(key+used,state->alpha_compare,20);used+=20;
    memcpy(key+used,&state->indirect_count,36);return used+36;
}
static void verify(const FhDrawState *s){
    unsigned char reference[4096],storage[4100];FhMaterialKey key;
    unsigned n=original(reference,s);fh_material_key_init(&key,s);
    assert(n==key.bytes && key.count<=23);
    assert(fh_material_key_hash(&key)==fh_hash_bytes(2166136261u,reference,n));
    for(unsigned alignment=0;alignment<4;alignment++){
        memset(storage,0xa5,sizeof(storage));unsigned char *out=storage+alignment;
        fh_material_key_copy(out,&key);
        assert(!memcmp(reference,out,n) && fh_material_key_equal(&key,out,n));
        assert(out[n]==0xa5 && (!alignment || out[-1]==0xa5));
        assert(!fh_material_key_equal(&key,out,n-1));
        unsigned byte=next()%n;out[byte]^=1;assert(!fh_material_key_equal(&key,out,n));
        checks++;
    }
}
int main(void){
    FhDrawState s;
    unsigned char before[4096],after[4096];
    for(unsigned trial=0;trial<8192;trial++){
        for(unsigned i=0;i<sizeof(s);i++)((unsigned char*)&s)[i]=next()>>24;
        s.stages=1+trial%16;
        if(trial%3==0){
            memset(&s,0,sizeof(s));s.stages=1;
            s.tev[0]=(FhTevStage){.color={15,8,0,15},.alpha={7,4,0,7},.color_clamp=1,.alpha_clamp=1,.map=trial%8,.coord=(trial/8)%8};
            for(unsigned c=0;c<4;c++){s.swaps[0][c]=c;s.tev_colors[0][c]=(next()%257)/256.f;}
        }
        verify(&s);
        unsigned n=original(before,&s),at=next()%sizeof(s);
        if(at>=offsetof(FhDrawState,stages) && at<offsetof(FhDrawState,stages)+4)continue;
        ((unsigned char*)&s)[at]^=0x40;
        unsigned m=original(after,&s);FhMaterialKey key;fh_material_key_init(&key,&s);
        assert(fh_material_key_equal(&key,before,n)==(n==m && !memcmp(before,after,n)));checks++;
    }
    /* Exhaustively change every byte of a complete sixteen-stage state. This
     * covers included fields and verifies that excluded state stays excluded. */
    memset(&s,0,sizeof(s));s.stages=16;unsigned n=original(before,&s);
    for(unsigned at=0;at<sizeof(s);at++){
        if(at>=offsetof(FhDrawState,stages) && at<offsetof(FhDrawState,stages)+4)continue;
        ((unsigned char*)&s)[at]^=1;unsigned m=original(after,&s);
        FhMaterialKey key;fh_material_key_init(&key,&s);
        assert(fh_material_key_equal(&key,before,n)==(n==m && !memcmp(before,after,n)));checks++;
        ((unsigned char*)&s)[at]^=1;
    }
    printf("PASS: %u material-key serialization/hash/equality/mutation checks\n",checks);
}
