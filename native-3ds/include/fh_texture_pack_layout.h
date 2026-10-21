#ifndef FH_TEXTURE_PACK_LAYOUT_H
#define FH_TEXTURE_PACK_LAYOUT_H
/* Typed, non-paletted source images used by the original disc. Palette-bearing
 * images need an explicit palette identity; never alias them with these keys. */
static inline unsigned fh_pack_extent(unsigned n){unsigned v=8;while(v<n)v*=2;return v;}
static inline unsigned fh_pack_source_bytes(unsigned f,unsigned w,unsigned h){
    if(!w||!h||w>1024||h>1024)return 0;
    unsigned bw=4,bh=4,n=32;
    switch(f){case 0:case 14:bw=bh=8;break;case 1:case 2:bw=8;break;
        case 3:case 4:case 5:break;case 6:n=64;break;default:return 0;}
    return ((w+bw-1)/bw)*((h+bh-1)/bh)*n;
}
static inline unsigned fh_pack_exact_format(unsigned f){
    return f==0?10:f==1?7:f==2?9:f==3?5:f==4?3:0;
}
static inline unsigned fh_pack_native_bytes(unsigned f,unsigned w,unsigned h){
    unsigned bits=f==10||f==12?4:f==7||f==9||f==13?8:f==3||f==5?16:f==0?32:0;
    return w*h*bits/8;
}
static inline int fh_pack_dimensions(unsigned n){return n>=8&&n<=1024&&!(n&(n-1));}
static inline int fh_pack_entry_valid(unsigned w,unsigned h,unsigned source_format,unsigned source_bytes,
                                      unsigned format,unsigned gw,unsigned gh,unsigned bytes){
    unsigned needed=fh_pack_source_bytes(source_format,w,h);
    if(!needed||source_bytes!=needed||!fh_pack_dimensions(gw)||!fh_pack_dimensions(gh))return 0;
    if(format==12||format==13){
        if(source_format!=14||!fh_pack_dimensions(w)||!fh_pack_dimensions(h)||gw>w||gh>h||
           (format==13&&(gw!=w||gh!=h)))return 0;
    }else if(format!=fh_pack_exact_format(source_format)||gw!=fh_pack_extent(w)||gh!=fh_pack_extent(h))return 0;
    return bytes==fh_pack_native_bytes(format,gw,gh);
}
#endif
