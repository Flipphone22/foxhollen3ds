#include "fh_retained_geometry.h"
static uint64_t indexed_fixture_identity;
static void indexed_fixture_decode(FhVertex *out,const unsigned char *packet){memcpy(out,packet,sizeof(*out));}
static uint64_t indexed_fixture_batch(FhVertex *out,const FhCompiledMesh *mesh){
    if(out)for(unsigned i=0;i<mesh->unique;i++)indexed_fixture_decode(out+i,mesh->packets+mesh->offsets[i]);
    return indexed_fixture_identity;
}
static void native_geometry_device_test(void) {
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;s.scissor[2]=s.scissor[3]=16;
    s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=2;
    for(unsigned c=0;c<4;c++){s.position_matrix[c][c]=1;s.swaps[0][c]=c;}
    for(unsigned unit=0;unit<2;unit++){
        s.textures[unit]=(FhTexture){.pixels=pixels[unit],.width=8,.height=8,.format=0x46};
        s.tev[unit]=(FhTevStage){.color={15,8,unit?0:10,15},.alpha={7,4,unit?0:5,7},.channel=4,.coord=unit,.map=unit,.color_clamp=1,.alpha_clamp=1};
    }
    FhVertex v[6]={0};uint32_t ids[6]={1,2,3,1,3,4};
    const unsigned corners[]={0,1,2,0,2,3};const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    unsigned char reference[256],actual[256];uint32_t rect[]={0,0,16,16};unsigned checks=0,hits,vertices,bytes;
    unsigned indexed_checks=0;extern unsigned fh_indexed_geometry_draws,fh_indexed_geometry_replays;
    unsigned indexed_before=fh_indexed_geometry_draws,replay_before=fh_indexed_geometry_replays;
    uint32_t mesh_offsets[]={0,sizeof(FhVertex),2*sizeof(FhVertex),5*sizeof(FhVertex)};
    uint16_t mesh_indices[]={0,1,2,0,2,3};
    FhCompiledMesh mesh={(const unsigned char*)v,mesh_offsets,mesh_indices,4,6};
    FhGpuLighting lighting={0};lighting.mask=1|FH_LIGHT_MODEL_NORMAL;lighting.ambient[0]=lighting.ambient[1]=lighting.ambient[2]=.2f;
    lighting.lights[0][0][2]=3;lighting.lights[0][1][2]=1;lighting.lights[0][3][0]=lighting.lights[0][4][0]=1;
    lighting.lights[0][2][0]=.8f;lighting.lights[0][2][1]=.7f;lighting.lights[0][2][2]=.6f;
    float post[3][4]={{-.25f,0,0,.6f},{0,-.25f,0,.6f},{0,0,0,1}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        v[i].matrix_index=0;v[i].color[0][0]=.7f;v[i].color[0][1]=.6f;v[i].color[0][2]=.8f;v[i].color[0][3]=1;
        v[i].uv[7][0]=.2f*xy[k][0];v[i].uv[7][1]=.1f*xy[k][1];v[i].uv[7][2]=1;
        for(unsigned unit=0;unit<2;unit++){v[i].uv[unit][0]=.25f+.1f*xy[k][0];v[i].uv[unit][1]=.25f+.1f*xy[k][1];v[i].uv[unit][2]=1;}
    }
    for(unsigned trial=0;trial<48;trial++) {
        unsigned kind=trial/16;s.raw_position=kind==0?1:kind==1?3:7;
        for(unsigned i=0;i<6;i++)v[i].uv[6][0]=kind==2;
        float angle=.025f*(trial%16);s.model_palette[0][0]=cosf(angle);s.model_palette[0][1]=-sinf(angle);
        s.model_palette[1][0]=sinf(angle);s.model_palette[1][1]=cosf(angle);s.model_palette[2][2]=1;
        lighting.lights[0][0][0]=trial*.1f;post[0][3]=.4f+.01f*(trial%16);
        /* A packing change must miss, then become reusable. Live uniforms
         * alone must reuse the existing native vertex/index allocation. */
        s.textures[0].width=trial%16<8?8:6;
        if(kind && !fh_renderer_prepare_lighting(&s,&lighting))fh_platform_panic("Native geometry lighting setup");
        if(kind==2 && !fh_renderer_prepare_texgen(&s,post))fh_platform_panic("Native geometry texgen setup");
        fh_renderer_draw_identified(v,6,&s,ids);fh_renderer_copy(reference,rect,8,8,6,1,0,0,0,&s);
        uint64_t id=0xfeed0000ULL+kind;
        if(!fh_renderer_replay_retained(id,6,&s))fh_renderer_draw_retained(id,v,6,&s,ids);
        fh_renderer_copy(actual,rect,8,8,6,1,0,0,0,&s);
        for(unsigned i=0;i<256;i++){if(reference[i]!=actual[i])fh_platform_panic("Native geometry differs from fresh stream");checks++;}
        indexed_fixture_identity=0xf00d0000ULL+kind;
        if(!fh_renderer_draw_compiled(&mesh,&s,indexed_fixture_decode,indexed_fixture_batch))fh_platform_panic("Indexed geometry declined");
        fh_renderer_copy(actual,rect,8,8,6,1,0,0,0,&s);
        for(unsigned i=0;i<256;i++){if(reference[i]!=actual[i])fh_platform_panic("Indexed geometry differs from fresh stream");indexed_checks++;}
    }
    /* Saturate cache ways/budget while earlier native buffers remain queued.
     * The command limit forces completion boundaries during this sequence. */
    s.raw_position=1;
    for(unsigned i=0;i<6;i++)v[i].uv[6][0]=0;
    unsigned declined=0;
    for(unsigned n=0;n<1200;n++){
        uint64_t id=0xbeef0000ULL+n;
        fh_renderer_draw_retained(id,v,6,&s,ids);
        if(!fh_renderer_replay_retained(id,6,&s)){declined++;fh_renderer_draw_identified(v,6,&s,ids);}
    }
    fh_renderer_copy(actual,rect,8,8,6,1,0,0,0,&s);
    fh_renderer_draw_identified(v,6,&s,ids);fh_renderer_copy(reference,rect,8,8,6,1,0,0,0,&s);
    for(unsigned i=0;i<256;i++){if(reference[i]!=actual[i])fh_platform_panic("Native geometry lifetime mismatch");checks++;}
    fh_renderer_native_geometry_stats(&hits,&vertices,&bytes);
    if(hits<100 || !bytes || bytes>FH_NATIVE_GEOMETRY_BUDGET)fh_platform_panic("Native geometry coverage or budget");
    FILE *f=fopen("sdmc:/3ds/foxhollow/native-geometry.txt","wb");if(!f)fh_platform_panic("Native geometry report open");
    fprintf(f,"%u %u %u %u %u\n",checks,hits,vertices,bytes,declined);fclose(f);
    if(fh_indexed_geometry_draws-indexed_before!=48 || fh_indexed_geometry_replays-replay_before!=42)fh_platform_panic("Indexed geometry replay coverage");
    f=fopen("sdmc:/3ds/foxhollow/indexed-geometry.txt","wb");if(!f)fh_platform_panic("Indexed geometry report open");
    fprintf(f,"%u %u %u\n",indexed_checks,fh_indexed_geometry_draws-indexed_before,fh_indexed_geometry_replays-replay_before);fclose(f);
    /* Compare expanded draws with large indexed packets, including both sides
     * of the staging boundary. Every index and the changed tail are rendered. */
    FhVertex *large=malloc(2049*sizeof(*large)),*expanded=malloc(2049*sizeof(*expanded));
    uint32_t *offsets=malloc(2049*sizeof(*offsets));uint16_t *indices=malloc(2049*sizeof(*indices));
    if(!large || !expanded || !offsets || !indices)fh_platform_panic("Large geometry fixture allocation");
    const unsigned sizes[]={1023,1024,1025,1083,2047,2048,2049};unsigned large_checks=0,large_hits=0;
    for(unsigned i=0;i<2049;i++){large[i]=v[i%6];offsets[i]=i*sizeof(*large);}
    for(unsigned shape=0;shape<sizeof(sizes)/sizeof(*sizes);shape++){
        mesh=(FhCompiledMesh){(const unsigned char*)large,offsets,indices,sizes[shape],(sizes[shape]+2)/3*3};
        for(unsigned i=0;i<mesh.count;i++)indices[i]=i%mesh.unique;
        for(unsigned revision=0;revision<2;revision++){
            large[mesh.unique-1].position[0]-=.05f;
            indexed_fixture_identity=0xf1000000ULL+shape*2+revision;
            for(unsigned i=0;i<mesh.count;i++)expanded[i]=large[indices[i]];
            fh_renderer_draw(expanded,mesh.count,&s);fh_renderer_copy(reference,rect,8,8,6,1,0,0,0,&s);
            unsigned before=fh_indexed_geometry_replays;
            for(unsigned repeat=0;repeat<2;repeat++){
                if(!fh_renderer_draw_compiled(&mesh,&s,indexed_fixture_decode,indexed_fixture_batch))fh_platform_panic("Large indexed geometry declined");
                fh_renderer_copy(actual,rect,8,8,6,1,0,0,0,&s);
                for(unsigned i=0;i<256;i++){if(reference[i]!=actual[i])fh_platform_panic("Large indexed geometry mismatch");large_checks++;}
            }
            unsigned hits=fh_indexed_geometry_replays-before;
            if(hits!=(mesh.unique<=FH_COMPILED_RETAINED_UNIQUE?1:0))fh_platform_panic("Large indexed replay boundary");
            large_hits+=hits;
        }
    }
    free(indices);free(offsets);free(expanded);free(large);
    f=fopen("sdmc:/3ds/foxhollow/large-indexed-geometry.txt","wb");if(!f)fh_platform_panic("Large geometry report open");
    fprintf(f,"%u %u\n",large_checks,large_hits);fclose(f);
}
