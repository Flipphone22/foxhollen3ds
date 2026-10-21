/* Real linear-heap fragmentation, not an injected allocation return value. */
static void texture_fragmentation_device_test(void) {
    enum { BYTES=512*256*4,BLOCK=65536,LIMIT=512 };
    extern unsigned fh_texture_allocation_retries,fh_texture_cache_evictions;
    unsigned char *source[2]={malloc(BYTES),malloc(BYTES)};
    void *blocks[LIMIT]={0};unsigned count=0;
    unsigned char expected[1024],actual[1024];
    if(!source[0] || !source[1])fh_platform_panic("Fragmentation fixture source allocation");
    for(unsigned i=0;i<BYTES;i++)source[0][i]=(unsigned char)(i*73+(i>>11));
    memcpy(source[1],source[0],BYTES);
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=32;s.viewport[5]=1;s.scissor[2]=s.scissor[3]=32;
    s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=1;
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    s.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++) {
        unsigned k=corners[i];v[i].position[0]=-1+2*xy[k][0];v[i].position[1]=1-2*xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
        v[i].uv[0][0]=xy[k][0];v[i].uv[0][1]=xy[k][1];v[i].uv[0][2]=1;
    }
    unsigned before=fh_texture_allocation_retries;
    for(unsigned route=0;route<2;route++) {
        s.textures[0]=(FhTexture){.pixels=source[route],.width=512,.height=256,.format=0x46};
        fh_renderer_invalidate(source[route],BYTES);
        fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,32,32};
        fh_renderer_copy(route?actual:expected,rect,16,16,6,1,0,0,0,&s);fh_renderer_present();
        if(!route) {
            while(count<LIMIT && (blocks[count]=linearAlloc(BLOCK)))count++;
            if(count==LIMIT)fh_platform_panic("Fragmentation fixture did not exhaust heap");
            for(unsigned i=0;i<count;i++)if(i%8!=7){linearFree(blocks[i]);blocks[i]=NULL;}
            if(linearSpaceFree()<4*1024*1024+BYTES)fh_platform_panic("Fragmentation fixture lacks aggregate reserve");
            void *probe=linearAlloc(BYTES);
            if(probe){linearFree(probe);fh_platform_panic("Fragmentation fixture still has contiguous space");}
        }
    }
    for(unsigned i=0;i<count;i++)if(blocks[i])linearFree(blocks[i]);
    if(memcmp(expected,actual,sizeof(actual)))fh_platform_panic("Reclaimed texture differs from reference");
    if(fh_texture_allocation_retries==before)fh_platform_panic("Fragmentation fixture did not exercise retry");
    FILE *f=fopen("sdmc:/3ds/foxhollow/texture-fragmentation.txt","wb");if(!f)fh_platform_panic("Fragmentation report");
    fprintf(f,"1024 %u %u %u\n",count,fh_texture_allocation_retries-before,fh_texture_cache_evictions);fclose(f);
    for(unsigned i=0;i<2;i++){fh_renderer_invalidate(source[i],BYTES);free(source[i]);}
}
