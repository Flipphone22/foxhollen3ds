/* Linked only through emulator-test wrappers. Observe, never repair, game state. */
#include <stdio.h>
#include <stdint.h>
#include "fh_platform.h"
#include "game/objects/object.h"
#include "main/objanim_internal.h"
#include "main/objseq.h"

static void check_events(GameObject *obj,const char *site) {
    if(!obj || !obj->anim.eventTable)return;
    ObjAnimEventTable *table=obj->anim.eventTable;
    uintptr_t address=(uintptr_t)table;
    if(address>=0x08000000 && address<0x0c000000-sizeof(*table) &&
       (uintptr_t)table->entries==((address+sizeof(*table)+7)&~(uintptr_t)7))return;
    char message[320];
    snprintf(message,sizeof(message),"event corruption site=%s obj=%p rom=%d def=%d class=%d extra=%p table=%p entries=%p count=%d\n",
        site,(void*)obj,obj->anim.romDefNo,obj->anim.defId,obj->anim.classId,obj->extra,(void*)table,
        address>=0x08000000 && address<0x0bfffff8?(void*)table->entries:NULL,
        address>=0x08000000 && address<0x0bfffff8?table->byteCount:-1);
    fh_platform_log(message);
    FILE *file=fopen("sdmc:/3ds/foxhollow/corrupt-object.bin","wb");
    if(file){fwrite(obj,1,sizeof(*obj),file);fclose(file);}
    if(address>=0x08000100 && address<0x0bffff00){
        file=fopen("sdmc:/3ds/foxhollow/corrupt-events.bin","wb");
        if(file){fwrite((void*)(address-256),1,512,file);fclose(file);}
    }
    fh_platform_panic("Object animation event storage corrupted");
}

void __real_ObjAnim_LoadMoveEvents(u8*,int,ObjAnimEventTable*,u32,u8);
void __wrap_ObjAnim_LoadMoveEvents(u8 *obj,int type,ObjAnimEventTable *table,u32 move,u8 load) {
    check_events((GameObject*)obj,"load move");
    __real_ObjAnim_LoadMoveEvents(obj,type,table,move,load);
}
int __real_ObjSeq_update(GameObject*,f32);
int __wrap_ObjSeq_update(GameObject *obj,f32 time) {
    check_events(obj,"sequence entry");
    ObjSeqState *state=obj->extra;
    if(state)check_events(state->targetObj,"sequence target entry");
    int result=__real_ObjSeq_update(obj,time);
    check_events(obj,"sequence exit");
    if(state)check_events(state->targetObj,"sequence target exit");
    return result;
}
void fh_object_check_events(GameObject *obj){check_events(obj,"frame boundary");}
