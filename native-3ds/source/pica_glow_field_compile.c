#include "fh_pica_glow_field.h"

static int disjoint(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=UINTPTR_MAX-an&&y<=UINTPTR_MAX-bn&&
        (x+an<=y||y+bn<=x);
}
static int compile(FhGlowMesh *output,FhGlowMeshVertex *scratch,
                   size_t scratch_count,const FhGlowField *field,
                   FhGlowMeshSample sample,void *context,size_t context_bytes,unsigned budget,int shared)
{
    float probe[2];
    if(budget<(shared?4u:2u)||budget>FH_GLOW_MESH_TRIANGLES||
       !output||!scratch||!field||scratch_count<FH_GLOW_MESH_TRAINING||
       !sample(context,0,0,probe))return 0;
    size_t bytes=sizeof(*scratch)*FH_GLOW_MESH_TRAINING;
    size_t source=fh_glow_field_bytes(field);
    if(!disjoint(output,sizeof(*output),scratch,bytes)||
       !disjoint(output,sizeof(*output),field->rgba,source)||
       !disjoint(scratch,bytes,field->rgba,source)||
       !disjoint(output,sizeof(*output),context,context_bytes)||
       !disjoint(scratch,bytes,context,context_bytes))return 0;
    for(unsigned y=0;y<=64;y++)for(unsigned x=0;x<=64;x++){
        FhGlowMeshVertex *v=scratch+y*65+x;float value[2];
        v->u=x/64.f;v->v=y/64.f;
        if(!sample(context,v->u,v->v,value)){
            output->vertices=output->triangles=0;return 0;
        }
        v->dx=value[0];v->dy=value[1];
    }
    return (shared?fh_glow_mesh_compile_shared:fh_glow_mesh_compile)(output,scratch,FH_GLOW_MESH_TRAINING,
                                budget,
                                sample,context);
}
int fh_glow_field_compile(FhGlowMesh *output,FhGlowMeshVertex *scratch,
                          size_t scratch_count,const FhGlowField *field)
{
    return compile(output,scratch,scratch_count,field,fh_glow_field_sample,
                   (void*)field,sizeof(*field),FH_GLOW_MESH_DEFAULT_TRIANGLES,0);
}
int fh_glow_field_region_compile(FhGlowMesh *output,FhGlowMeshVertex *scratch,
                                 size_t scratch_count,const FhGlowFieldRegion *region)
{
    return fh_glow_field_region_compile_budget(output,scratch,scratch_count,region,
                                              FH_GLOW_MESH_DEFAULT_TRIANGLES);
}
int fh_glow_field_region_compile_budget(FhGlowMesh *output,FhGlowMeshVertex *scratch,
    size_t scratch_count,const FhGlowFieldRegion *region,unsigned budget)
{
    if(!region)return 0;
    return compile(output,scratch,scratch_count,&region->field,fh_glow_field_region_sample,
                   (void*)region,sizeof(*region),budget,0);
}
int fh_glow_field_shared_compile(FhGlowMesh *output,FhGlowMeshVertex *scratch,
    size_t scratch_count,const FhGlowField *field,unsigned budget)
{
    return compile(output,scratch,scratch_count,field,fh_glow_field_sample,
                   (void*)field,sizeof(*field),budget,1);
}
