/* Real CPU lighting/texgen decoder, current arrays, and independent renderer
 * packing composition. No cached vertex values replace any shading work. */
static FhNativeVertex pack_reference(const FhVertex *v,const FhNativePacking *p){
    FhVertex sphere;
    if(p->sphere){
        sphere=*v;v=&sphere;
        for(unsigned j=0;j<2;j++){
            sphere.uv[6+j][0]=(.5f+15*v->color[1][j*2])/16;
            sphere.uv[6+j][1]=(.5f+15*v->color[1][j*2+1])/16;sphere.uv[6+j][2]=1;
        }
    }
    FhNativeVertex out={0};memcpy(out.position,v->position,16);out.matrix_index=v->matrix_index;
    for(unsigned c=0;c<4;c++)out.color[c]=p->affine?v->color[0][c]:p->primary[c]<0?0:((const float*)v->color)[p->primary[c]];
    for(unsigned u=0;u<p->units;u++){
        const float *uv=v->uv[p->coord[u]];float q=uv[2];if(fabsf(q)<1e-20f)q=1;
        float s=uv[0],t=uv[1];if(u){s/=q;t/=q;}
        out.uv[u][0]=s*p->scale[u][0];out.uv[u][1]=(u?1:q)-t*p->scale[u][1];out.uv[u][2]=u?1:q;
    }
    if(p->affine){memcpy(out.uv[1],v->color[1],12);out.uv[2][0]=v->color[1][3];}
    return out;
}
int main(void){
    unsigned checks=0;u8 packet[4][128],arrays[3][192];uint32_t offsets[4];
    FhNativeVertex actual[4],reference[4];
    expected=GX_AUTO;list_batching=1;cache_enabled=0;
    for(unsigned trial=0;trial<8192;trial++){
        memset(&fh_gx,0,sizeof(fh_gx));memset(fields,0,sizeof(fields));memset(prepared_texgen,0,sizeof(prepared_texgen));
        fh_gx.draw.raw_position=1;gpu_lighting=gpu_lighting_flat=gpu_lighting_palette=gpu_texgen=0;
        lighting_needs_normal=1;texgen_needs_eye=constant_lighting=cpu_light_state_id=compiled_post_template=0;
        raster_channels=trial&1?3:1;fh_gx_active_texgens=2;
        for(unsigned c=0;c<4;c++){
            lighting_group[c]=c;
            fh_gx.channel[c]=(FhChannel){.enabled=1,.mask=3,.material_source=GX_SRC_REG,.ambient_source=GX_SRC_REG,
                .diffuse=(trial+c)%3,.attenuation=(trial/3+c)%3};
            for(unsigned ch=0;ch<2;ch++){
                fh_gx.material[ch][c]=(next_random()%256)/255.f;
                fh_gx.ambient[ch][c]=(next_random()%64)/255.f;
            }
        }
        for(unsigned l=0;l<2;l++){
            FhLight *v=fh_gx.lights+l;v->position[0]=l+1;v->position[1]=2;v->position[2]=3;
            v->direction[2]=1;v->a[0]=1;v->a[1]=.25f;v->k[0]=1;v->k[2]=.01f;
            for(unsigned c=0;c<4;c++)v->color[c]=(next_random()%128)/255.f;
        }
        for(unsigned m=0;m<64;m++)for(unsigned r=0;r<3;r++){
            fh_gx.position[m][r][r]=1+(m%3)*.125f;fh_gx.position[m][r][3]=r*.1f;
            fh_gx.normal[m][r][r]=1+(m%4)*.25f;
        }
        for(unsigned u=0;u<2;u++){
            fh_gx.texgens[u]=(FhTexGen){GX_TG_MTX3x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTTEXMTX0+u*3};
            prepared_texgen[u].alias=u;prepared_texgen[u].post=fh_gx.post[u];
            fh_gx.post[u][0][0]=1;fh_gx.post[u][0][3]=u*.2f;
            fh_gx.post[u][1][1]=1;fh_gx.post[u][1][3]=trial*.0001f;
            fh_gx.post[u][2][0]=(trial%3)*.25f;fh_gx.post[u][2][3]=trial&2?0:1;
        }
        field_count=0;stride=1;fh_gx.descriptors[GX_VA_PNMTXIDX]=GX_DIRECT;
        fields[field_count++]=(PreparedField){.attr=GX_VA_PNMTXIDX,.mode=GX_DIRECT,.offset=0};
        unsigned mode=1+(trial%3),le=(trial/3)&1;
        for(unsigned a=0;a<3;a++){
            unsigned nrm_kind=(trial/128)%3;
            unsigned attr=a==0?GX_VA_POS:a==1?GX_VA_NRM:GX_VA_TEX0,n=a==2?2:a==1 && nrm_kind?9:3,size=n*4;
            fh_gx.descriptors[attr]=mode;
            fields[field_count++]=(PreparedField){.attr=attr,.mode=mode,.offset=stride,.bytes=size,.components=n,.component_bytes=4,.scale=1,
                .format={a==0?GX_POS_XYZ:a==1?nrm_kind:GX_TEX_ST,GX_F32,0}};
            for(unsigned i=0;i<4;i++){
                u8 *p=packet[i]+stride,*data=p;
                if(mode!=GX_DIRECT){*p=mode==GX_INDEX8?i:0;if(mode==GX_INDEX16)p[1]=i;
                    if(a==1 && nrm_kind==GX_NRM_NBT3)for(unsigned slice=1;slice<3;slice++){
                        unsigned at=slice*(mode==GX_INDEX8?1:2),index=(i+slice)%4;
                        p[at]=mode==GX_INDEX8?index:0;if(mode==GX_INDEX16)p[at+1]=index;
                    }
                    fh_gx.arrays[attr]=(FhVtxArray){arrays[a],sizeof(arrays[a]),size,le};data=arrays[a]+i*size;}
                for(unsigned c=0;c<n;c++)put_component(data+4*c,GX_F32,mode==GX_DIRECT?0:le);
            }
            stride+=mode==GX_DIRECT?size:(mode==GX_INDEX8?1:2)*(a==1 && nrm_kind==GX_NRM_NBT3?3:1);
        }
        for(unsigned i=0;i<4;i++){packet[i][0]=i*3;offsets[i]=i*sizeof(packet[0]);}
        /* Exercise the lowered program's dependency graph, rather than only
         * independent texture coordinates. The original decoder is the oracle. */
        switch((trial/16)%8){
        case 1:
            fh_gx.texgens[1].type=GX_TG_BUMP0;fh_gx.texgens[1].source=GX_TG_TEXCOORD0;
            texgen_needs_eye=1;break;
        case 2:
            fh_gx.texgens[0].source=GX_TG_NRM;fh_gx.texgens[0].normalize=1;break;
        case 3:
            fh_gx.texgens[0].source=GX_TG_POS;fh_gx.texgens[0].matrix=0;
            prepared_texgen[0].matrix=fh_gx.position[0];break;
        case 4:
            fh_gx.texgens[0].source=GX_TG_COLOR0;fh_gx.texgens[0].type=GX_TG_SRTG;break;
        case 5:
            fh_gx.texgens[0].source=GX_TG_TEXCOORD1;
            fh_gx.texgens[1].source=GX_TG_TEXCOORD0;break;
        case 6:
            fh_gx.texgens[1].source=GX_TG_TEXCOORD0;fh_gx.texgens[1].normalize=1;break;
        case 7:
            fh_gx.texgens[1].source=GX_TG_POS;fh_gx.texgens[1].matrix=0;
            prepared_texgen[1].dynamic=1;
            fh_gx.descriptors[GX_VA_TEX1MTXIDX]=GX_DIRECT;
            fields[field_count++]=(PreparedField){.attr=GX_VA_TEX1MTXIDX,.mode=GX_DIRECT,.offset=stride};
            for(unsigned i=0;i<4;i++)packet[i][stride]=i*3;
            ++stride;break;
        }
        FhCompiledMesh mesh={.packets=&packet[0][0],.offsets=offsets,.unique=4,.count=6};
        FhNativePacking pack={.units=trial%4};
        for(unsigned c=0;c<4;c++)pack.primary[c]=(int)((trial+c)%9)-1;
        for(unsigned u=0;u<pack.units;u++){pack.coord[u]=u%2;pack.scale[u][0]=.5f;pack.scale[u][1]=.75f;}
        if(trial%7==0){pack.affine=1;pack.units=trial&1;}
        else if(trial%7==1){pack.sphere=1;pack.units=3;pack.coord[0]=0;pack.coord[1]=6;pack.coord[2]=7;}
        for(unsigned i=0;i<4;i++){FhVertex v;decode_compiled(&v,packet[i]);reference[i]=pack_reference(&v,&pack);}
        fh_compiled_native_mode=trial&1?2:1;
        unsigned before=decoded_vertices,prior=received,identity_before=vertex_identity;
        assert(decode_compiled_native(NULL,&mesh,NULL)==1);
        assert(decode_compiled_native(actual,&mesh,&pack)==fh_compiled_native_mode);
        assert(!memcmp(actual,reference,sizeof(actual)));
        assert(decoded_vertices-before==(fh_compiled_native_mode==2?0:4));
        assert(received-prior==(fh_compiled_native_mode==2?0:4));
        if(fh_compiled_native_mode==2)assert(vertex_identity==identity_before);
        /* A late CPU-projection fallback must not publish a partial packet or
         * advance logical decoder accounting before the ordinary retry. */
        before=decoded_vertices;prior=received;identity_before=vertex_identity;packet[3][0]=30;
        assert(!decode_compiled_native(actual,&mesh,&pack));
        assert(decoded_vertices==before && received==prior && vertex_identity==identity_before);
        checks+=4;
    }
    printf("PASS: %u CPU-lit/projective vertices match scalar decode + renderer packing; late-failure accounting restored\n",checks);
    return 0;
}
