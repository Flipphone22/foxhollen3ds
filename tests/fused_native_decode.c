/* Independent composition of the scalar decoder output and existing renderer
 * packing rules; the candidate reads indexed attributes directly. */
static unsigned native_checks,native_fallbacks;
static void check_native_packet(const FhVertex *reference,const FastCompiled *fast,const u8 *packet,unsigned trial){
    FhNativePacking packing={0};
    packing.units=trial%4;
    for(unsigned c=0;c<4;c++)packing.primary[c]=(int)((trial/(c+1))%9)-1;
    for(unsigned u=0;u<packing.units;u++){
        packing.coord[u]=(trial+u)%fast->texgens;
        packing.scale[u][0]=(float)(1+(trial%127))/128;
        packing.scale[u][1]=(float)(1+(trial%255))/256;
        if((fh_gx.draw.raw_position&4) && !packing.coord[u])packing.generated_units|=1u<<u;
    }
    FhNativeVertex base,actual,expected={0};
    assert(prepare_native_compiled(&base,fast,&packing));
    if(!decode_native_compiled(&actual,packet,fast,&packing,&base)){native_fallbacks++;return;}
    memcpy(expected.position,reference->position,sizeof(expected.position));
    memcpy(expected.normal,reference->uv[7],sizeof(expected.normal));
    expected.matrix_index=reference->matrix_index;
    for(unsigned c=0;c<4;c++)expected.color[c]=packing.primary[c]<0?0:((const float*)reference->color)[packing.primary[c]];
    for(unsigned u=0;u<packing.units;u++){
        const float *uv=reference->uv[packing.coord[u]];
        float q=uv[2];if(fabsf(q)<1e-20f)q=1;
        float s=uv[0],t=uv[1];if(u){s/=q;t/=q;}
        expected.uv[u][0]=s*packing.scale[u][0];
        expected.uv[u][1]=(u?1:q)-t*packing.scale[u][1];expected.uv[u][2]=u?1:q;
    }
    if((fh_gx.draw.raw_position&4) && reference->uv[6][0] && expected.matrix_index>=0){
        expected.generated_normal_uv=1;
        for(unsigned u=0;u<3;u++)if(packing.generated_units&(1u<<u)){
            expected.uv[u][0]=packing.scale[u][0];expected.uv[u][1]=packing.scale[u][1];
        }
    }
    if(memcmp(&actual,&expected,sizeof(actual))){
        fprintf(stderr,"Fused pack mismatch trial=%u units=%u\n",trial,packing.units);
        for(unsigned i=0;i<sizeof(actual)/4;i++)if(((uint32_t*)&actual)[i]!=((uint32_t*)&expected)[i])
            fprintf(stderr,"word=%u actual=%08x expected=%08x\n",i,((uint32_t*)&actual)[i],((uint32_t*)&expected)[i]);
        abort();
    }
    packing.units=4;assert(!prepare_native_compiled(&base,fast,&packing));
    packing.units=1;packing.coord[0]=fast->texgens;assert(!prepare_native_compiled(&base,fast,&packing));
    native_checks++;
}
