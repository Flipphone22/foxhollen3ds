/* Exercise the physical-stall sequence: color copy, then depth copy with
 * no intervening draw commands. Values span every byte of the 24-bit depth.
 * No disc assets, approximate pixel checks, or raw-copy reference needed. */
static void efb_readback_device_test(void) {
    extern unsigned fh_depth_snapshot_reads,fh_depth_snapshot_reuses;
    const uint32_t values[]={0,1,0x123456,0x800000,0xfffffe,0xffffff};
    const uint32_t rect[]={0,0,640,FH_EFB_HEIGHT};
    enum { W=320,H=FH_EFB_NATIVE_HEIGHT };
    unsigned char *copy=malloc(W*H*2);
    if(!copy)fh_platform_panic("Readback fixture allocation failed");
    unsigned checks=0;
    FhDrawState state={.pixel_format=1};
    for(unsigned test=0;test<sizeof(values)/sizeof(*values);test++) {
        unsigned reads_before=fh_depth_snapshot_reads,reuses_before=fh_depth_snapshot_reuses;
        fh_renderer_clear(0x123456a5,values[test]);
        unsigned char color[64];const uint32_t small[]={0,0,8,8};
        fh_renderer_copy(color,small,4,4,6,1,0,0,0,&state);
        for(unsigned i=0;i<16;i++) {
            if(color[2*i]!=0xa5 || color[2*i+1]!=0x12 || color[32+2*i]!=0x34 || color[33+2*i]!=0x56)
                fh_platform_panic("Readback color bytes changed");
            checks++;
        }
        for(unsigned fmt=0x11;fmt<=0x13;fmt+=2) {
            fh_renderer_copy(copy,rect,W,H,fmt,1,0,0,0,&state);
            for(unsigned i=0;i<W*H;i++) {
                unsigned actual=fmt==0x11?copy[i]:(copy[2*i]<<8)|copy[2*i+1];
                if(actual!=(values[test]>>(fmt==0x11?16:8)))fh_platform_panic("Readback depth bytes changed");
                checks++;
            }
        }
        const unsigned positions[][2]={{0,0},{639,0},{0,FH_EFB_HEIGHT-1},{639,FH_EFB_HEIGHT-1},{127,129}};
        for(unsigned i=0;i<sizeof(positions)/sizeof(*positions);i++) {
            if(fh_renderer_depth(positions[i][0],positions[i][1])!=values[test])fh_platform_panic("Readback depth peek changed");
            checks++;
        }
        if(fh_depth_snapshot_reads!=reads_before+1 || fh_depth_snapshot_reuses!=reuses_before+6)
            fh_platform_panic("Unchanged depth snapshot was transferred again");
        /* A color transfer shares the readback allocation and must invalidate
         * depth ownership even when there was no intervening draw. */
        fh_renderer_copy(color,small,4,4,6,1,0,0,0,&state);
        if(fh_renderer_depth(127,129)!=values[test] || fh_depth_snapshot_reads!=reads_before+2)
            fh_platform_panic("Color readback failed to invalidate depth snapshot");
        /* A copy with clear issues an actual depth-writing draw. Observe its
         * changed center and unchanged exterior before the next frame. */
        const uint32_t center[]={160,120,320,240};
        state.depth_write=1;
        unsigned changed=values[test]==0x800000?0:0x800000;
        fh_renderer_copy(color,center,4,4,6,0,1,0,changed,&state);
        unsigned actual_center=fh_renderer_depth(320,240),actual_corner=fh_renderer_depth(0,0);
        /* Drawing can round D24 through the emulator's float depth target;
         * untouched clear/readback bytes above remain strict equality. */
        if(abs((int)actual_center-(int)changed)>2 || abs((int)actual_corner-(int)values[test])>2) {
            char error[192];snprintf(error,sizeof(error),"Depth clear test=%u center=%x expected=%x corner=%x expected=%x reads=%u reuses=%u",test,actual_center,changed,actual_corner,values[test],fh_depth_snapshot_reads,fh_depth_snapshot_reuses);
            fh_platform_panic(error);
        }
        /* Another clear follows a live depth snapshot: draw count, rather
         * than a preceding color transfer, must invalidate this one. */
        const uint32_t depth_clear[]={240,180,160,120};
        fh_renderer_copy(copy,depth_clear,4,4,0x13,0,1,0,values[test],&state);
        if(abs((int)fh_renderer_depth(320,240)-(int)values[test])>2)
            fh_platform_panic("Depth snapshot survived an intervening draw");
        checks+=6;
        fh_renderer_present();
    }
    /* Independent presentation coordinates: two paths agreeing on a shifted
     * EFB are not a correctness oracle. Host checks the actual display image,
     * including background around a known rectangular color clear. */
    fh_renderer_clear(0x123456ff,0xffffff);
    state.color_write=state.alpha_write=1;state.depth_write=0;
    const uint32_t marker[]={160,120,320,240};
    fh_renderer_copy(copy,marker,4,4,6,0,1,0xa02b6cff,0,&state);
    fh_renderer_present();
    free(copy);
    FILE *f=fopen("sdmc:/3ds/foxhollow/readback.txt","wb");
    if(!f)fh_platform_panic("Readback result file failed");
    fprintf(f,"%u\n",checks);fclose(f);
}
