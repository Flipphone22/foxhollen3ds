#include "internal.h"

#define BUFFER_BASE_PADDR 0x18000000

#include "/work/native-3ds/include/fh_pica_buffers.h"
static const FhPicaBufferLayout *fh_cpu_layout,*fh_gpu_layout;
static u32 fh_gpu_offsets[3];
unsigned long long fh_prepared_buffer_counts[5];
void fh_pica_buffers_invalidate(void){fh_cpu_layout=NULL;fh_gpu_layout=NULL;}
static void fh_buffers_written(C3D_BufInfo *info){
    if(info==&C3Di_GetContext()->bufInfo)fh_cpu_layout=NULL;
}
int fh_pica_buffers_bind(const FhPicaBufferLayout *layout,const void *const data[3]){
    C3D_Context *ctx=C3Di_GetContext();
    if(!(ctx->flags&C3DiF_Active) || !layout || !data || layout->count<1 || layout->count>3)return 0;
    u32 offsets[3];
    for(unsigned i=0;i<layout->count;i++){
        if(!data[i])return 0;
        u32 physical=osConvertVirtToPhys(data[i]);
        if(physical<BUFFER_BASE_PADDR)return 0;
        offsets[i]=physical-BUFFER_BASE_PADDR;
    }
    C3D_BufInfo *info=&ctx->bufInfo;
    if(fh_cpu_layout!=layout){
        /* Ordinary SDK writers invalidate provenance. Clear all their possible
         * streams once; a prepared->prepared transition clears only its tail. */
        if(!fh_cpu_layout)memset(info,0,sizeof(*info));
        else if(fh_cpu_layout->count>layout->count)
            memset(info->buffers+layout->count,0,(fh_cpu_layout->count-layout->count)*sizeof(C3D_BufCfg));
        info->base_paddr=BUFFER_BASE_PADDR;info->bufCount=layout->count;
        for(unsigned i=0;i<layout->count;i++){
            info->buffers[i].flags[0]=layout->flags[i][0];
            info->buffers[i].flags[1]=layout->flags[i][1];
        }
        fh_cpu_layout=layout;++fh_prepared_buffer_counts[1];
    }
    for(unsigned i=0;i<layout->count;i++)info->buffers[i].offset=offsets[i];
    ctx->flags|=C3DiF_BufInfo;++fh_prepared_buffer_counts[0];return 1;
}


void BufInfo_Init(C3D_BufInfo* info)
{
    fh_buffers_written(info);
	memset(info, 0, sizeof(*info));
	info->base_paddr = BUFFER_BASE_PADDR;
}

int BufInfo_Add(C3D_BufInfo* info, const void* data, ptrdiff_t stride, int attribCount, u64 permutation)
{
    fh_buffers_written(info);
	if (info->bufCount == 12) return -1;
	int id = info->bufCount++;

	u32 pa = osConvertVirtToPhys(data);
	if (pa < info->base_paddr) return -2;

	C3D_BufCfg* buf = &info->buffers[id];
	buf->offset = pa - info->base_paddr;
	buf->flags[0] = permutation & 0xFFFFFFFF;
	buf->flags[1] = (permutation >> 32) | (stride << 16) | (attribCount << 28);
	return id;
}

C3D_BufInfo* C3D_GetBufInfo(void)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return NULL;

	fh_cpu_layout=NULL;
	ctx->flags |= C3DiF_BufInfo;
	return &ctx->bufInfo;
}

void C3D_SetBufInfo(C3D_BufInfo* info)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	if (info != &ctx->bufInfo)
		memcpy(&ctx->bufInfo, info, sizeof(*info));
	fh_cpu_layout=NULL;
	ctx->flags |= C3DiF_BufInfo;
}

void C3Di_BufInfoBind(C3D_BufInfo* info)
{
    const FhPicaBufferLayout *layout=info==&C3Di_GetContext()->bufInfo?fh_cpu_layout:NULL;
    if(layout && fh_gpu_layout==layout){
        /* Formats and inactive streams are already published. Only these live
         * offsets can change; no geometry copy, 36-word scan or reconstruction. */
        for(unsigned i=0;i<layout->count;i++){
            u32 offset=info->buffers[i].offset;
            if(offset!=fh_gpu_offsets[i]){
                GPUCMD_AddWrite(GPUREG_ATTRIBBUFFER0_OFFSET+3*i,offset);
                fh_gpu_offsets[i]=offset;++fh_prepared_buffer_counts[3];
            }
        }
        return;
    }
    GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_LOC,info->base_paddr>>3);
    GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFER0_OFFSET,(u32*)info->buffers,sizeof(info->buffers)/(sizeof(u32)));
    fh_gpu_layout=layout;++fh_prepared_buffer_counts[2];
    if(layout)for(unsigned i=0;i<layout->count;i++)fh_gpu_offsets[i]=info->buffers[i].offset;
    else ++fh_prepared_buffer_counts[4];
}
