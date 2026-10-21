#include "fh_audio_controls.h"
#include "fh_audio_worker.h"
#include <musyx/musyx.h>

enum { CONTROL_PACKETS=64 };
typedef struct { uint32_t voice; uint8_t pan,surround,volume,spatial; } ControlPacket;
_Static_assert(sizeof(ControlPacket)==8,"bounded value-only audio command");
static ControlPacket packets[CONTROL_PACKETS];
static unsigned head,tail,draining;
unsigned fh_audio_controls_enabled;
unsigned fh_audio_controls_queued,fh_audio_controls_applied,fh_audio_controls_backpressure;

static void apply(const ControlPacket *p) {
    if(p->spatial){(void)sndFXCtrl(p->voice,0x0a,p->pan);(void)sndFXCtrl(p->voice,0x83,p->surround);}
    (void)sndFXCtrl(p->voice,7,p->volume);
}
void fh_audio_controls_drain(void) {
    /* sndFXCtrl re-enters the synth API. The guard is protected by the same
     * recursive lock, and prevents replaying the current packet recursively. */
    if(draining)return;
    draining=1;
    unsigned end=__atomic_load_n(&head,__ATOMIC_ACQUIRE);
    unsigned cursor=__atomic_load_n(&tail,__ATOMIC_RELAXED);
    while(cursor!=end){
        apply(&packets[cursor%CONTROL_PACKETS]);
        ++cursor;
        __atomic_store_n(&tail,cursor,__ATOMIC_RELEASE);
        __atomic_add_fetch(&fh_audio_controls_applied,1,__ATOMIC_RELAXED);
    }
    draining=0;
}
void fh_audio_spatial_controls(uint32_t voice,unsigned pan,unsigned surround,unsigned volume,unsigned spatial) {
    ControlPacket packet={voice,(uint8_t)pan,(uint8_t)surround,(uint8_t)volume,spatial!=0};
    if(!fh_audio_controls_can_queue()) {apply(&packet);return;}
    /* One main-thread producer. Never overwrite/drop controls to conceal a
     * saturated owner: the bounded slow path executes the same pending work. */
    unsigned write=__atomic_load_n(&head,__ATOMIC_RELAXED);
    if(write-__atomic_load_n(&tail,__ATOMIC_ACQUIRE)==CONTROL_PACKETS){
        fh_audio_controls_backpressure++;
        fh_audio_lock();fh_audio_controls_drain();fh_audio_unlock();
    }
    packets[write%CONTROL_PACKETS]=packet;
    __atomic_store_n(&head,write+1,__ATOMIC_RELEASE);
    fh_audio_controls_queued++;
    fh_audio_worker_wake();
}
