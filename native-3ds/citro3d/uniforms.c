#include "internal.h"
#include <c3d/uniforms.h>
#include <string.h>
/* Pinned Melee uniform_upload.h compares against installed values. Here an
 * unchanged prefix/suffix is removed without fragmenting a dirty batch.
 * Snapshots contain values copied into immutable commands, never pointers. */
unsigned fh_uniform_reuse_enabled=1;
unsigned long long fh_uniform_reuse_counts[3]; /* requested, uploaded, reused */
static C3D_FVec fh_uploaded[2][C3D_FVUNIF_COUNT];
static bool fh_uploaded_valid[2][C3D_FVUNIF_COUNT] __attribute__((aligned(4)));
typedef u32 fh_uniform_word __attribute__((may_alias));
_Static_assert(sizeof(C3D_FVec)==16,"Uniform row layout");
_Static_assert(sizeof(bool)==1 && C3D_FVUNIF_COUNT%4==0,"Uniform flags layout");
static inline bool fh_uniform_row_same(GPU_SHADER_TYPE type,int row){
    if(!fh_uploaded_valid[type][row])return false;
    const fh_uniform_word *a=(const fh_uniform_word*)&fh_uploaded[type][row];
    const fh_uniform_word *b=(const fh_uniform_word*)&C3D_FVUnif[type][row];
    /* Bit equality preserves signed zeros and NaN payloads. GCC's may_alias
     * word type avoids aliasing UB and out-of-line tiny memcmp calls. */
    return a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3];
}
/* Same wire sequence as SDK CONFIG + DATA uploads, including its 256-word
 * chunk limit. Fuse publication and the reuse snapshot so each source value
 * is loaded once. The existing command owner still flushes and retires it.
 * Adapted from pinned Melee/Citro3D checked packet construction. */
extern void fh_platform_panic(const char*) __attribute__((noreturn));
static void fh_uniform_packet(GPU_SHADER_TYPE type,unsigned offset,int first,int end){
    unsigned rows=(unsigned)(end-first),words=2+rows*4+2*((rows+63)/64);
    if(first<0 || end>C3D_FVUNIF_COUNT || first>=end || !gpuCmdBuf ||
       gpuCmdBufOffset>gpuCmdBufSize || words>gpuCmdBufSize-gpuCmdBufOffset)
        fh_platform_panic("PICA uniform packet capacity");
    u32 *dst=gpuCmdBuf+gpuCmdBufOffset;
    const fh_uniform_word *src=(const void*)&C3D_FVUnif[type][first];
    fh_uniform_word *cache=(void*)&fh_uploaded[type][first];
    *dst++=0x80000000u|(unsigned)first;
    *dst++=GPUCMD_HEADER(0,15,GPUREG_VSH_FLOATUNIFORM_CONFIG+offset);
    while(rows){
        unsigned batch=rows>64?64:rows;
        u32 value=*src++;*dst++=value;*cache++=value;
        *dst++=GPUCMD_HEADER(0,15,GPUREG_VSH_FLOATUNIFORM_DATA+offset)|((batch*4-1)<<20);
        for(unsigned i=0;i<3;i++){value=*src++;*dst++=value;*cache++=value;}
        for(unsigned i=1;i<batch;i++){
            u32 a=src[0],b=src[1],c=src[2],d=src[3];
            dst[0]=cache[0]=a;dst[1]=cache[1]=b;
            dst[2]=cache[2]=c;dst[3]=cache[3]=d;
            src+=4;dst+=4;cache+=4;
        }
        *dst++=0;rows-=batch;
    }
    gpuCmdBufOffset+=words;
}


C3D_FVec C3D_FVUnif[2][C3D_FVUNIF_COUNT];
C3D_IVec C3D_IVUnif[2][C3D_IVUNIF_COUNT];
u16      C3D_BoolUnifs[2];

bool C3D_FVUnifDirty[2][C3D_FVUNIF_COUNT] __attribute__((aligned(4)));
bool C3D_IVUnifDirty[2][C3D_IVUNIF_COUNT];
bool C3D_BoolUnifsDirty[2];

static struct
{
	bool dirty;
	int count;
	float24Uniform_s* data;
} C3Di_ShaderFVecData[2];

static bool C3Di_FVUnifEverDirty[2][C3D_FVUNIF_COUNT] __attribute__((aligned(4)));
static bool C3Di_IVUnifEverDirty[2][C3D_IVUNIF_COUNT];

void C3D_UpdateUniforms(GPU_SHADER_TYPE type)
{
	int offset = type == GPU_GEOMETRY_SHADER ? (GPUREG_GSH_BOOLUNIFORM-GPUREG_VSH_BOOLUNIFORM) : 0;
	int i = 0;

	// Update FVec uniforms that come from shader constants
	if (C3Di_ShaderFVecData[type].dirty)
	{
        unsigned constant_count=(unsigned)C3Di_ShaderFVecData[type].count;
        /* Six words per SDK incremental CONFIG+three-DATA float24 upload.
         * Check the complete batch before modifying commands or dirty state. */
        if(constant_count && (!gpuCmdBuf || gpuCmdBufOffset>gpuCmdBufSize ||
           constant_count>(gpuCmdBufSize-gpuCmdBufOffset)/6))
            fh_platform_panic("PICA float24 constant capacity");
		while (i < C3Di_ShaderFVecData[type].count)
		{
			float24Uniform_s* u = &C3Di_ShaderFVecData[type].data[i++];
            const fh_uniform_word *src=(const void*)u;
            u32 *dst=gpuCmdBuf+gpuCmdBufOffset;
            dst[0]=src[0];
            dst[1]=GPUCMD_HEADER(1,15,GPUREG_VSH_FLOATUNIFORM_CONFIG+offset)|(3u<<20);
            dst[2]=src[1];dst[3]=src[2];dst[4]=src[3];dst[5]=0;
            gpuCmdBufOffset+=6;
			C3D_FVUnifDirty[type][u->id] = false;
            /* Float24 constants overwrite the physical row independently
             * of the application's float32 shadow, even across programs. */
            fh_uploaded_valid[type][u->id] = false;
		}
		C3Di_ShaderFVecData[type].dirty = false;
		i = 0;
	}

	// Update FVec uniforms
	while (i < C3D_FVUNIF_COUNT)
	{
        /* Preserve the public dirty-byte ABI, including direct SDK writes.
         * Skip four clean rows per read; never infer dirtiness from setters. */
        if(!(i&3) && i+4<=C3D_FVUNIF_COUNT &&
           !*(const fh_uniform_word*)(C3D_FVUnifDirty[type]+i)){
            i+=4;continue;
        }
		if (!C3D_FVUnifDirty[type][i])
		{
			i ++;
			continue;
		}

		// Find the number of consecutive dirty uniforms
		int j;
		for (j=i; j<C3D_FVUNIF_COUNT;) {
            if (!(j&3) && j+4<=C3D_FVUNIF_COUNT &&
                *(const fh_uniform_word*)(C3D_FVUnifDirty[type]+j)==0x01010101u) {
                j+=4;continue;
            }
            if (!C3D_FVUnifDirty[type][j])break;
            j++;
        }

		// Upload the uniforms
        fh_uniform_reuse_counts[0] += j-i;
        int first=i,end=j;
        if(fh_uniform_reuse_enabled){
            while(first<end && fh_uniform_row_same(type,first))first++;
            while(end>first && fh_uniform_row_same(type,end-1))end--;
        }
        fh_uniform_reuse_counts[2] += (j-i)-(end-first);
        if(first<end){
            fh_uniform_packet(type,offset,first,end);
            fh_uniform_reuse_counts[1] += end-first;
        }

		// Clear the dirty flag
		int k;
        /* Every dirty row is now installed: trimmed rows were already valid
         * and equal; the middle was copied into the command list above. Keep
         * the SDK byte ABI, but retire four adjacent rows per aligned store. */
        for (k=i; k<j;) {
            if (!(k&3) && k+4<=j) {
                *(fh_uniform_word*)(C3D_FVUnifDirty[type]+k)=0;
                *(fh_uniform_word*)(C3Di_FVUnifEverDirty[type]+k)=0x01010101u;
                *(fh_uniform_word*)(fh_uploaded_valid[type]+k)=0x01010101u;
                k+=4;
            } else {
                C3D_FVUnifDirty[type][k]=false;
                C3Di_FVUnifEverDirty[type][k]=true;
                fh_uploaded_valid[type][k]=true;
                k++;
            }
        }

		// Advance
		i = j;
	}

	// Update IVec uniforms
	for (i = 0; i < C3D_IVUNIF_COUNT; i ++)
	{
		if (!C3D_IVUnifDirty[type][i]) continue;

		GPUCMD_AddWrite(GPUREG_VSH_INTUNIFORM_I0+offset+i, C3D_IVUnif[type][i]);
		C3D_IVUnifDirty[type][i] = false;
		C3Di_IVUnifEverDirty[type][i] = false;
	}

	// Update bool uniforms
	if (C3D_BoolUnifsDirty[type])
	{
		GPUCMD_AddWrite(GPUREG_VSH_BOOLUNIFORM+offset, 0x7FFF0000 | C3D_BoolUnifs[type]);
		C3D_BoolUnifsDirty[type] = false;
	}
}

void C3Di_DirtyUniforms(GPU_SHADER_TYPE type)
{
    /* GPU state may have been lost or modified outside Citro3D. Never
     * suppress an explicit context restoration, even for equal CPU values. */
    memset(fh_uploaded_valid[type],0,sizeof(fh_uploaded_valid[type]));
	int i;
	C3D_BoolUnifsDirty[type] = true;
	if (C3Di_ShaderFVecData[type].count)
		C3Di_ShaderFVecData[type].dirty = true;
	for (i = 0; i < C3D_FVUNIF_COUNT; i ++)
		C3D_FVUnifDirty[type][i] = C3D_FVUnifDirty[type][i] || C3Di_FVUnifEverDirty[type][i];
	for (i = 0; i < C3D_IVUNIF_COUNT; i ++)
		C3D_IVUnifDirty[type][i] = C3D_IVUnifDirty[type][i] || C3Di_IVUnifEverDirty[type][i];
}

void C3Di_LoadShaderUniforms(shaderInstance_s* si)
{
	GPU_SHADER_TYPE type = si->dvle->type;
	if (si->boolUniformMask)
	{
		C3D_BoolUnifs[type] &= ~si->boolUniformMask;
		C3D_BoolUnifs[type] |= si->boolUniforms;
	}

	if (type == GPU_GEOMETRY_SHADER)
		C3D_BoolUnifs[type] &= ~BIT(15);
	C3D_BoolUnifsDirty[type] = true;

	if (si->intUniformMask)
	{
		int i;
		for (i = 0; i < 4; i ++)
		{
			if (si->intUniformMask & BIT(i))
			{
				C3D_IVUnif[type][i] = si->intUniforms[i];
				C3D_IVUnifDirty[type][i] = true;
			}
		}
	}
	C3Di_ShaderFVecData[type].dirty = true;
	C3Di_ShaderFVecData[type].count = si->numFloat24Uniforms;
	C3Di_ShaderFVecData[type].data = si->float24Uniforms;
}

void C3Di_ClearShaderUniforms(GPU_SHADER_TYPE type)
{
    memset(fh_uploaded_valid[type],0,sizeof(fh_uploaded_valid[type]));
	C3Di_ShaderFVecData[type].dirty = false;
	C3Di_ShaderFVecData[type].count = 0;
	C3Di_ShaderFVecData[type].data = NULL;
}


/* Raw ordinary draws own only float32 rows7..9. Restore their CPU shadows on
 * SDK departure, even if a row has never previously been dirty. Invalidate the
 * retained physical-value cache as well; marking SDK dirty alone can be lost.
 * No invalidation of unrelated camera/palette/light rows or GS uniforms. */
void fh_pica_stream_restore_uniforms(void){
    for(unsigned row=7;row<=9;row++){
        fh_uploaded_valid[GPU_VERTEX_SHADER][row]=false;
        C3D_FVUnifDirty[GPU_VERTEX_SHADER][row]=true;
        C3Di_FVUnifEverDirty[GPU_VERTEX_SHADER][row]=true;
    }
}


/* Preserve pending SDK values for its next real consumer. Native publication
 * replaces rows7..76 directly, so restoring their old SDK values here would
 * duplicate the palette traffic. Shared camera/light/bool/float24 publication
 * still executes in SDK order after the GS boundary's physical invalidation.
 * Actor begin rejects float24 constants inside this borrowed range. */
void fh_pica_stream_update_actor_shared_uniforms(void){
    bool pending[70];
    memcpy(pending,C3D_FVUnifDirty[GPU_VERTEX_SHADER]+7,sizeof pending);
    memset(C3D_FVUnifDirty[GPU_VERTEX_SHADER]+7,0,sizeof pending);
    C3D_UpdateUniforms(GPU_VERTEX_SHADER);
    memcpy(C3D_FVUnifDirty[GPU_VERTEX_SHADER]+7,pending,sizeof pending);
}
void fh_pica_stream_restore_actor_uniforms(const uint32_t mask[3]){
    for(unsigned block=0;block<3;block++)for(unsigned bits=mask[block];bits;bits&=bits-1){
        unsigned row=block*32+__builtin_ctz(bits);
        fh_uploaded_valid[GPU_VERTEX_SHADER][row]=false;
        C3D_FVUnifDirty[GPU_VERTEX_SHADER][row]=true;
        C3Di_FVUnifEverDirty[GPU_VERTEX_SHADER][row]=true;
    }
}
