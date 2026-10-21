/* Included after cell(); original recursive route is the image oracle. */
static void movie_batch_device_test(void){
    extern unsigned fh_movie_batch_enabled,fh_movie_batches,fh_movie_batch_triangles;
    extern unsigned fh_movie_disjoint_batches,fh_movie_disjoint_triangles,fh_movie_disjoint_rejections;
    unsigned checks=0,nonzero=0;unsigned char reference[256],actual[256];
    FILE *images=fopen("sdmc:/3ds/foxhollow/movie-batch.bin","wb");
    if(!images)fh_platform_panic("Movie comparison output");
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
        unsigned char *p=pixels[4]+(y*8+x)*4;
        p[0]=x*31;p[1]=y*29;p[2]=(x^y)*27;p[3]=255;
    }
    fh_renderer_invalidate(pixels[4],sizeof(pixels[4]));
    for(movie_fixture_case=1;movie_fixture_case<=24;movie_fixture_case++){
        for(unsigned mode=0;mode<2;mode++){
            fh_movie_batch_enabled=mode?
#ifdef FH_ORDERED_PROGRAM_TEST
                3
#else
                2
#endif
                :0;gpu_transform_test=(movie_fixture_case-1)%12>=6;cell(3);
            uint32_t rect[]={368,360,16,16};FhDrawState state={.pixel_format=1};
            unsigned char *out=mode?actual:reference;
            fh_renderer_copy(out,rect,8,8,6,1,0,0,0,&state);
            fwrite(out,1,256,images);fh_renderer_present();
        }
        for(unsigned i=0;i<256;i++){
            if(reference[i]!=actual[i])fh_platform_panic("Ordered movie batch differs from recursive reference");
            checks++;nonzero+=reference[i]!=0 && !(i%64<32 && !(i&1));
        }
    }
    fclose(images);movie_fixture_case=0;gpu_transform_test=0;fh_movie_batch_enabled=0;
#ifdef FH_ORDERED_PROGRAM_TEST
    extern unsigned fh_ordered_program_chunks,fh_ordered_program_triangles;
    if(fh_movie_batches!=24 || fh_movie_batch_triangles!=336 || !nonzero ||
       fh_ordered_program_chunks!=72 || fh_ordered_program_triangles!=336)fh_platform_panic("Ordered program fixture coverage");
#else
    if(fh_movie_batches!=24 || fh_movie_batch_triangles!=48 || !nonzero ||
       fh_movie_disjoint_batches<6 || fh_movie_disjoint_rejections<12)fh_platform_panic("Movie batch fixture coverage");
#endif
    FILE *f=fopen("sdmc:/3ds/foxhollow/movie-batch.json","wb");if(!f)fh_platform_panic("Movie report");
    fprintf(f,"{\"checks\":%u,\"nonzero_channels\":%u,\"batches\":%u,\"triangles\":%u,\"grouped_batches\":%u,\"grouped_triangles\":%u,\"rejections\":%u}\n",checks,nonzero,fh_movie_batches,fh_movie_batch_triangles,fh_movie_disjoint_batches,fh_movie_disjoint_triangles,fh_movie_disjoint_rejections);fclose(f);
}
