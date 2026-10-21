/* Scene ownership, load/release boundaries only. No linear/VRAM allocation. */
static unsigned fur_plan_cpu,fur_plan_peak,fur_plan_builds,fur_plan_declined;
static void prepare_fur_plan(SceneRecord*r,const void*raw,unsigned bytes){
    if(!r->actor_plan)return;
    unsigned count=fh_pica_fur_compile(&r->asset,r->actor_plan,raw,bytes,NULL,0);
    if(!count)return;
    unsigned size=sizeof(FhPicaFurPlan)+count*sizeof(FhPicaFurShape);
    if(size>32*1024-fur_plan_cpu || !normal_reserve(size)){fur_plan_declined++;return;}
    FhPicaFurPlan*p=malloc(size);if(!p){fur_plan_declined++;return;}
    if(fh_pica_fur_compile(&r->asset,r->actor_plan,raw,bytes,p->shape,count)!=count){free(p);fur_plan_declined++;return;}
    const FhPicaModelDraw *first=NULL,*last=NULL;
    for(unsigned i=0;i<r->actor_plan->count;i++)if(r->actor_plan->draw[i].flags&512){
        last=r->actor_plan->draw+i;if(!first)first=last;
    }
    p->wrap_changed=0;
    for(unsigned b=0;b<first->bones;b++)if(b>=last->bones || first->bone[b]!=last->bone[b])p->wrap_changed|=1u<<b;
    p->count=count;p->groom=NULL;r->fur_plan=p;fur_plan_cpu+=size;fh_native_scene_cpu_bytes+=size;
    if(fur_plan_cpu>fur_plan_peak)fur_plan_peak=fur_plan_cpu;
    fur_plan_builds++;
}
static void release_fur_plan(SceneRecord*r){
    FhPicaFurPlan*p=r->fur_plan;if(!p)return;
    release_fur_groom(p->groom);
    unsigned size=sizeof(*p)+p->count*sizeof(FhPicaFurShape);
    fur_plan_cpu-=size;fh_native_scene_cpu_bytes-=size;free(p);r->fur_plan=NULL;
}
