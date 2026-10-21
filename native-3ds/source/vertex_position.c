#include "fh_graphics.h"

/* Shared by CPU fragment submission and its conservative early visibility
 * check. Identical ordered float operations avoid cross-compiler clip tests. */
void fh_vertex_clip_position(float clip[4],const float position[4],unsigned matrix,const FhDrawState *state) {
    float eye[4]={0,0,0,position[3]};
    for(unsigned row=0;row<3;row++)for(unsigned c=0;c<4;c++)eye[row]+=state->model_palette[matrix+row][c]*position[c];
    for(unsigned row=0;row<4;row++) {
        float value=0;for(unsigned c=0;c<4;c++)value+=state->position_matrix[row][c]*eye[c];
        clip[row]=value;
    }
}

void fh_vertex_pica_projection(float matrix[4][4],const FhDrawState *state) {
    float sx=state->viewport[2]/640.f,sy=state->viewport[3]/FH_EFB_HEIGHT;
    float ox=(2*state->viewport[0]+state->viewport[2]-640)/640.f,oy=(FH_EFB_HEIGHT-2*state->viewport[1]-state->viewport[3])/FH_EFB_HEIGHT;
    float sz=state->viewport[5]-state->viewport[4],oz=state->viewport[5]-1;
    for(unsigned c=0;c<4;c++){
        matrix[0][c]=sy*state->position_matrix[1][c]+oy*state->position_matrix[3][c];
        matrix[1][c]=-(sx*state->position_matrix[0][c]+ox*state->position_matrix[3][c]);
        matrix[2][c]=sz*state->position_matrix[2][c]+oz*state->position_matrix[3][c];
        matrix[3][c]=state->position_matrix[3][c];
    }
}
/* Melee's bounds use a .002 magnitude margin for PICA float24. Propagate a
 * separate model-transform error here, so cancellation does not hide large
 * intermediate terms. Require both the original GX and actual PICA side
 * half-spaces to reject. This also retains CPU material fallback semantics.
 * No near/depth-plane or perspective-division rejection is used. */
unsigned fh_vertex_conservative_outside(const float position[4],unsigned matrix,const FhDrawState *state,const float pica[4][4]) {
    float eye[4]={0,0,0,position[3]},error[4]={0,0,0,0};
    for(unsigned c=0;c<4;c++)if(!__builtin_isfinite(position[c]) || __builtin_fabsf(position[c])>1e18f)return 0;
    for(unsigned row=0;row<3;row++){
        float magnitude=1;
        for(unsigned c=0;c<4;c++){
            float m=state->model_palette[matrix+row][c];
            if(!__builtin_isfinite(m) || __builtin_fabsf(m)>1e18f)return 0;
            float term=m*position[c];eye[row]+=term;magnitude+=__builtin_fabsf(term);
        }
        if(!__builtin_isfinite(magnitude) || magnitude>1e18f)return 0;
        error[row]=.002f*magnitude;
    }
    unsigned result=0;
    for(unsigned view=0;view<2;view++){
        const float (*projection)[4]=view?pica:state->position_matrix;
        float q[3],e[3]; /* x, y, w: depth clipping remains the renderer's job. */
        for(unsigned axis=0;axis<3;axis++){
            unsigned row=axis==2?3:axis;float value=0,magnitude=1,propagated=0;
            for(unsigned c=0;c<4;c++){
                float m=projection[row][c];
                if(!__builtin_isfinite(m) || __builtin_fabsf(m)>1e18f)return 0;
                float term=m*eye[c];value+=term;magnitude+=__builtin_fabsf(term);
                propagated+=__builtin_fabsf(m)*error[c];
            }
            if(!__builtin_isfinite(magnitude) || magnitude>1e18f || !__builtin_isfinite(propagated))return 0;
            q[axis]=value;e[axis]=.002f*magnitude+propagated;
        }
        for(unsigned axis=0;axis<2;axis++){
            float margin=e[2]+e[axis];
            if(q[2]+q[axis]<-margin)result|=1u<<(view*4+axis*2);
            if(q[2]-q[axis]<-margin)result|=2u<<(view*4+axis*2);
        }
    }
    return result;
}

/* Transform clip planes into local space, then maximize over each box.
 * The error estimate uses absolute intermediate terms, not the cancelled
 * local coefficients. Like pinned Melee geometry_bounds.h this needs no
 * perspective division or eight corner transforms. */
static unsigned bounds_prepare(float eye_magnitude[4],const float low[3],const float high[3],unsigned matrix,const FhDrawState *state){
    float extent[4]={0,0,0,1};
    eye_magnitude[0]=eye_magnitude[1]=eye_magnitude[2]=0;eye_magnitude[3]=1;
    for(unsigned c=0;c<3;c++){
        if(!__builtin_isfinite(low[c]) || !__builtin_isfinite(high[c]) || low[c]>high[c])return 0;
        float a=__builtin_fabsf(low[c]),b=__builtin_fabsf(high[c]);extent[c]=a>b?a:b;
        if(extent[c]>1e18f)return 0;
    }
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++){
        float m=state->model_palette[matrix+r][c];if(!__builtin_isfinite(m) || __builtin_fabsf(m)>1e18f)return 0;
        eye_magnitude[r]+=__builtin_fabsf(m)*extent[c];
    }
    return 1;
}
static unsigned bounds_evaluate(const float eye_magnitude[4],const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates,unsigned first_only){
    /* Validate all candidate rows before an early return. A NaN in a later
     * plane must retain the original fallback even if an earlier one rejects. */
    for(unsigned row=0;row<4;row++)if((row==3 && candidates) || (row<3 && (candidates&(3u<<(row*2)))))
        for(unsigned c=0;c<4;c++)if(!__builtin_isfinite(projection[row][c]) || __builtin_fabsf(projection[row][c])>1e18f)return 0;
    unsigned mask=0;
    for(unsigned plane=0;plane<6;plane++)if(candidates&(1u<<plane)){
        unsigned axis=plane/2;float sign=plane&1?-1:1,w=plane==5?0:1;
        float p[4],local[4],magnitude=1;
        for(unsigned c=0;c<4;c++){
            float a=projection[axis][c],b=projection[3][c];
            p[c]=w*b+sign*a;magnitude+=(w*__builtin_fabsf(b)+__builtin_fabsf(a))*eye_magnitude[c];
        }
        for(unsigned c=0;c<4;c++){
            local[c]=c==3?p[3]:0;
            for(unsigned r=0;r<3;r++)local[c]+=p[r]*state->model_palette[matrix+r][c];
        }
        float maximum=local[3];for(unsigned c=0;c<3;c++)maximum+=local[c]*(local[c]>=0?high[c]:low[c]);
        if(__builtin_isfinite(maximum) && __builtin_isfinite(magnitude) && magnitude<1e18f && maximum<-.008f*magnitude){
            mask|=1u<<plane;
            if(first_only)return mask;
        }
    }
    return mask;
}
static unsigned bounds_planes(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates,unsigned first_only){
    float eye_magnitude[4];
    if(!bounds_prepare(eye_magnitude,low,high,matrix,state))return 0;
    return bounds_evaluate(eye_magnitude,low,high,matrix,state,projection,candidates,first_only);
}
/* Share only the current model/box preparation. No visibility or matrix state
 * survives a call, and both original projection/error tests remain required. */
unsigned fh_bounds_pair_enabled,fh_bounds_pair_calls;
unsigned fh_vertex_bounds_pair(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float pica[4][4],unsigned gx,unsigned gpu){
    float eye_magnitude[4];fh_bounds_pair_calls++;
    if(!bounds_prepare(eye_magnitude,low,high,matrix,state))return 0;
    return !!bounds_evaluate(eye_magnitude,low,high,matrix,state,state->position_matrix,gx,0) &&
           !!bounds_evaluate(eye_magnitude,low,high,matrix,state,pica,gpu,0);
}
unsigned fh_vertex_bounds_planes(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates){
    return bounds_planes(low,high,matrix,state,projection,candidates,0);
}
unsigned fh_vertex_bounds_any_plane(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates){
    return bounds_planes(low,high,matrix,state,projection,candidates,1);
}
