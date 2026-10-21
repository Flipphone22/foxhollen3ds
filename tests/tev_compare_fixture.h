
static void tev_compare_device_test(void) {
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    const unsigned bytes[4][4]={{17,31,47,63},{25,51,76,102},{51,25,76,127},{85,85,85,85}};
    FhVertex v[6]={0};const unsigned corners[]={0,1,2,0,2,3};const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    for(unsigned i=0;i<6;i++){unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;}
    unsigned checks=0,maximum=0;unsigned char copy[256];FILE *captures=fopen("sdmc:/3ds/foxhollow/tev-compare.bin","wb");
    if(!captures)fh_platform_panic("TEV compare capture open failed");
    for(unsigned scenario=0;scenario<24;scenario++) {
        for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)s.tev_colors[r][c]=bytes[r][c]/255.f;
        unsigned op=8+scenario%8;
        s.tev[0]=(FhTevStage){.color={2,4,6,0},.alpha={1,2,3,0},.map=255,.coord=255,.channel=255,
            .color_op=op,.alpha_op=op,.color_clamp=1,.alpha_clamp=1,.color_dest=1,.alpha_dest=2};
        if(scenario>=8) {
            s.tev[0].color_op=s.tev[0].alpha_op=0;
            memcpy(s.tev[0].color,(unsigned[]){15,15,15,2},16);memcpy(s.tev[0].alpha,(unsigned[]){7,7,7,2},16);
            s.tev[0].color_dest=(scenario-8)&3;s.tev[0].alpha_dest=(scenario-8)>>2;
        }
        fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,16,16};fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);
        fwrite(copy,1,sizeof(copy),captures);
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++) {
            unsigned expected=bytes[c==3?2:1][c];
            if(scenario<8) {
                unsigned kind=(op-8)/2,a=0,b=0;
                if(kind==3){a=bytes[1][c];b=bytes[2][c];}
                else for(unsigned k=0;k<=kind;k++){a|=bytes[1][k]<<(8*k);b|=bytes[2][k]<<(8*k);}
                unsigned pass=op&1?a==b:a>b;expected=bytes[0][c]+(pass?bytes[3][c]:0);
            }
            unsigned at=((y/4)*2+x/4)*64+((y%4)*4+x%4)*2,indices[]={at+1,at+32,at+33,at};
            unsigned error=abs((int)copy[indices[c]]-(int)expected);if(error>maximum)maximum=error;
            if(error>2)fh_platform_panic("TEV compare or final-register rendered mismatch");checks++;
        }
    }
    fclose(captures);FILE *f=fopen("sdmc:/3ds/foxhollow/tev-compare.txt","wb");if(!f)fh_platform_panic("TEV compare report open failed");
    fprintf(f,"%u %u\n",checks,maximum);fclose(f);fh_renderer_present();
}
