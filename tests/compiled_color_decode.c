/* Production scalar + fused renderer packing are the independent references.
 * Every iteration changes live packet/array colors, not cached output values. */
int main(void){
    unsigned checks=0;u8 arrays[2][64],packet[128];
    cache_enabled=0;expected=GX_AUTO;list_batching=1;
    for(unsigned trial=0;trial<36864;trial++){
        memset(&fh_gx,0,sizeof(fh_gx));memset(fields,0,sizeof(fields));
        memset(prepared_texgen,0,sizeof(prepared_texgen));
        unsigned raw=(trial&1?7:3)|(trial&2?32:0);
        gpu_lighting=1;gpu_lighting_flat=!!(raw&32);gpu_lighting_palette=2;
        gpu_texgen=!!(raw&4);lighting_needs_normal=texgen_needs_eye=constant_lighting=cpu_light_state_id=0;
        raster_channels=3;fh_gx.draw.raw_position=raw;fh_gx_active_texgens=gpu_texgen?2:1;
        fh_gx.texgens[0]=(FhTexGen){GX_TG_MTX2x4,gpu_texgen?GX_TG_NRM:GX_TG_TEX0,gpu_texgen?GX_TEXMTX0:GX_IDENTITY,0,GX_PTIDENTITY};
        if(gpu_texgen)fh_gx.texgens[1]=(FhTexGen){GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY};
        unsigned coord=gpu_texgen?1:0;
        if(trial&4){
            fh_gx.texgens[coord].postmatrix=GX_PTTEXMTX0;
            prepared_texgen[coord].post=fh_gx.post[0];
            for(unsigned r=0;r<2;r++)for(unsigned c=0;c<4;c++)fh_gx.post[0][r][c]=((int)(next_random()%257)-128)/128.f;
            fh_gx.post[0][2][trial&8?2:3]=1;
        }
        for(unsigned i=0;i<4;i++){
            fh_gx.channel[i].material_source=((trial/4)>>i)&1?GX_SRC_VTX:GX_SRC_REG;
            if(i==0 || (i==2 && (raw&32)))fh_gx.channel[i].enabled=1;
            /* Inactive constant CPU channels have the scalar's precedence. */
            else if((trial/64)&1){constant_lighting|=1u<<i;fh_gx.channel[i].enabled=1;}
        }
        for(unsigned c=0;c<8;c++){
            ((float*)fh_gx.material)[c]=(next_random()%256)/255.f;
            ((float*)constant_colors)[c]=(next_random()%256)/255.f;
        }
        field_count=0;stride=0;
        fh_gx.descriptors[GX_VA_PNMTXIDX]=GX_DIRECT;
        fields[field_count++]=(PreparedField){.attr=GX_VA_PNMTXIDX,.mode=GX_DIRECT,.offset=stride};packet[stride++]=(trial%10)*3;
        if(gpu_texgen){fh_gx.descriptors[GX_VA_TEX0MTXIDX]=GX_DIRECT;
            fields[field_count++]=(PreparedField){.attr=GX_VA_TEX0MTXIDX,.mode=GX_DIRECT,.offset=stride};packet[stride++]=packet[0]+GX_TEXMTX0;}
        for(unsigned a=0;a<3;a++){
            if(a==1 && gpu_lighting_flat && (trial/18432))continue;
            unsigned attr=a==0?GX_VA_POS:a==1?GX_VA_NRM:GX_VA_TEX0,n=a==2?2:3;
            fh_gx.descriptors[attr]=GX_DIRECT;
            fields[field_count++]=(PreparedField){.attr=attr,.mode=GX_DIRECT,.bytes=n*4,.components=n,.component_bytes=4,.offset=stride,.scale=1,
                .format={a==0?GX_POS_XYZ:a==1?GX_NRM_XYZ:GX_TEX_ST,GX_F32,0}};
            for(unsigned c=0;c<n;c++)put_component(packet+stride+c*4,GX_F32,0);
            stride+=n*4;
        }
        for(unsigned channel=0;channel<2;channel++){
            unsigned type=(trial/128+channel)%6,mode=1+(trial/768)%3,le=(trial/2304)&1;
            static const unsigned sizes[]={2,3,4,2,3,4};unsigned size=sizes[type],attr=GX_VA_CLR0+channel;
            if((trial/4608)&(1u<<channel))continue; /* Default white without a color descriptor. */
            fh_gx.descriptors[attr]=mode;
            fields[field_count++]=(PreparedField){.attr=attr,.mode=mode,.bytes=size,.offset=stride,.format={GX_CLR_RGBA,type,0}};
            u8 *data;
            if(mode==GX_DIRECT){data=packet+stride;stride+=size;}
            else{
                unsigned index=1+(trial&3);
                packet[stride++]=mode==GX_INDEX8?index:0;if(mode==GX_INDEX16)packet[stride++]=index;
                fh_gx.arrays[attr]=(FhVtxArray){arrays[channel],sizeof(arrays[channel]),size,le};data=arrays[channel]+index*size;
            }
            for(unsigned c=0;c<size;c++)data[c]=next_random()>>24;
        }
        FastCompiled fast;assert(prepare_fast_compiled(&fast));FhVertex actual,reference;
        assert(decode_fast_compiled(&actual,packet,&fast));decode_compiled(&reference,packet);
        if(memcmp(&actual,&reference,sizeof(actual))){fprintf(stderr,"Colored decode mismatch trial=%u raw=%u mask=%x\n",trial,raw,fast.color_mask);abort();}
        check_native_packet(&reference,&fast,packet,trial);
        if(trial&4){
            fh_gx.post[0][2][0]=.25f;assert(!prepare_fast_compiled(&fast));fh_gx.post[0][2][0]=0;
            fh_gx.post[0][0][0]=INFINITY;assert(!prepare_fast_compiled(&fast));fh_gx.post[0][0][0]=1;
            assert(prepare_fast_compiled(&fast));
            const PreparedField *uv=fast.uv[0];u8 saved[4];memcpy(saved,packet+uv->offset,4);
            const u8 nan[4]={0x7f,0xc0,0,0};memcpy(packet+uv->offset,nan,4);
            assert(!decode_fast_compiled(&actual,packet,&fast));
            FhNativePacking pack={.units=1,.coord={coord},.primary={0,1,2,3},.scale={{1,1}}};
            FhNativeVertex base,native;assert(prepare_native_compiled(&base,&fast,&pack));
            assert(!decode_native_compiled(&native,packet,&fast,&pack,&base));
            memcpy(packet+uv->offset,saved,4);
        }
        FastCompiled compact;if(fast.color_mask || fast.color[0] || fast.color[1])assert(!prepare_compact_compiled(&compact));
        constant_lighting&=~2u;fh_gx.channel[1].enabled=1;assert(!prepare_fast_compiled(&fast));
        checks++;
    }
    assert(native_checks==checks && !native_fallbacks);
    printf("PASS: %u live colored vertices and native packets exactly match scalar; CPU-lit channels rejected\n",checks);
    return 0;
}
