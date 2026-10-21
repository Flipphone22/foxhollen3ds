/* Native approximation of addWarpedNoiseTevStages. The animated base image,
 * second-UV mask, lit base, opacity and outer blend remain live. PICA noise
 * replaces GX dependent-coordinate noise (cosmetic detail only). No CPU pixel
 * processing, render target, second pass or extra vertex instructions.
 *
 * Melee's renderer owns/flushes GPU command storage; these persistent tables
 * are copied by Citro3D into that command list, not sampled from CPU memory.
 * ProcTex is a Foxhollow adaptation using the pinned Citro3D implementation.
 */
static C3D_ProcTex decal_proc;
static C3D_ProcTexLut decal_map, decal_noise;
static C3D_ProcTexColorLut decal_colors;
static unsigned decal_initialized, decal_uploaded_frame=~0u;

static void model_decal_noise(unsigned frame,const float phase[2]){
    if(!decal_initialized){
        float ramp[129];u32 colors[128];
        for(unsigned i=0;i<=128;i++)ramp[i]=i/128.f;
        ProcTexLut_FromArray(&decal_map,ramp);
        /* Smooth interpolation of PICA's own two-dimensional noise. */
        for(unsigned i=0;i<=128;i++){float x=i/128.f;ramp[i]=x*x*(3-2*x);}
        ProcTexLut_FromArray(&decal_noise,ramp);
        for(unsigned i=0;i<128;i++){
            /* Independent deterministic palette; do not consume game RNG.
             * Original noise has blue128..255, red=blue-[0,64],
             * green=blue-[64,128], quantized RGB565. */
            unsigned b=128+i,r=(b-32)&248,g=(b-96)&252;
            b&=248;r|=r>>5;g|=g>>6;b|=b>>5;
            colors[i]=r|(g<<8)|(b<<16)|0xff000000u;
        }
        ProcTexColorLut_Write(&decal_colors,colors,0,128);
        C3D_ProcTexInit(&decal_proc,0,128);
        C3D_ProcTexClamp(&decal_proc,GPU_PT_REPEAT,GPU_PT_REPEAT);
        C3D_ProcTexCombiner(&decal_proc,false,GPU_PT_U,GPU_PT_U);
        C3D_ProcTexFilter(&decal_proc,GPU_PT_LINEAR);
        decal_initialized=1;
    }
    /* Original reflection phase; elapsed game time remains authoritative. */
    C3D_ProcTexNoiseCoefs(&decal_proc,C3D_ProcTex_U,1.f,32.f,phase[0]);
    C3D_ProcTexNoiseCoefs(&decal_proc,C3D_ProcTex_V,1.f,32.f,phase[1]);
    if(decal_uploaded_frame!=frame){
        C3D_ProcTexLutBind(GPU_LUT_RGBMAP,&decal_map);
        C3D_ProcTexLutBind(GPU_LUT_NOISE,&decal_noise);
        C3D_ProcTexColorLutBind(&decal_colors);
        decal_uploaded_frame=frame;
    }
    C3D_ProcTexBind(0,&decal_proc);
}

static void model_decal_env(unsigned mapped,unsigned intensity,unsigned alpha){
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    /* Stage1's lit base reaches PREVIOUS_BUFFER at stage3. */
    C3D_TexEnvBufUpdate(C3D_RGB,1u<<1);
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    if(mapped){
        C3D_TexEnvColor(e,(intensity&255)*0x010101u|0xff000000u);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
    }else{
        C3D_TexEnvSrc(e,C3D_RGB,GPU_PRIMARY_COLOR,0,0);
    }
    e=C3D_GetTexEnv(1);
    C3D_TexEnvColor(e,0x00ffffffu|((alpha&255)<<24));
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_PREVIOUS,0);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    e=C3D_GetTexEnv(2);
    C3D_TexEnvColor(e,0xff002028u); /* Original K=(40,32,0). */
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE3,GPU_CONSTANT,GPU_TEXTURE2);
    /* GX_ITBA_U reads green (indirect sample .abg), not blue. PICA uses
     * the full channel, approximating GX's normalized top-five-bit value. */
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_G);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    e=C3D_GetTexEnv(3);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS_BUFFER,GPU_PREVIOUS,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    /* Alpha passes through stages2..5. Stage5 remains available for the
     * existing live object fade; fog/blending/depth stay in the caller. */
}
