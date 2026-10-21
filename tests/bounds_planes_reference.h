/* Frozen pre-cache scalar oracle; ordered float arithmetic is intentional. */
static unsigned reference_bounds_planes(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates){
    float extent[4]={0,0,0,1},eye_magnitude[4]={0,0,0,1};
    for(unsigned c=0;c<3;c++){
        if(!__builtin_isfinite(low[c]) || !__builtin_isfinite(high[c]) || low[c]>high[c])return 0;
        float a=__builtin_fabsf(low[c]),b=__builtin_fabsf(high[c]);extent[c]=a>b?a:b;
        if(extent[c]>1e18f)return 0;
    }
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++){
        float m=state->model_palette[matrix+r][c];if(!__builtin_isfinite(m) || __builtin_fabsf(m)>1e18f)return 0;
        eye_magnitude[r]+=__builtin_fabsf(m)*extent[c];
    }
    unsigned mask=0;
    for(unsigned plane=0;plane<6;plane++)if(candidates&(1u<<plane)){
        unsigned axis=plane/2;float sign=plane&1?-1:1,w=plane==5?0:1;
        float p[4],local[4],magnitude=1;
        for(unsigned c=0;c<4;c++){
            float a=projection[axis][c],b=projection[3][c];
            if(!__builtin_isfinite(a) || !__builtin_isfinite(b) || __builtin_fabsf(a)>1e18f || __builtin_fabsf(b)>1e18f)return 0;
            p[c]=w*b+sign*a;magnitude+=(w*__builtin_fabsf(b)+__builtin_fabsf(a))*eye_magnitude[c];
        }
        for(unsigned c=0;c<4;c++){
            local[c]=c==3?p[3]:0;
            for(unsigned r=0;r<3;r++)local[c]+=p[r]*state->model_palette[matrix+r][c];
        }
        float maximum=local[3];for(unsigned c=0;c<3;c++)maximum+=local[c]*(local[c]>=0?high[c]:low[c]);
        if(__builtin_isfinite(maximum) && __builtin_isfinite(magnitude) && magnitude<1e18f && maximum<-.008f*magnitude)mask|=1u<<plane;
    }
    return mask;
}
