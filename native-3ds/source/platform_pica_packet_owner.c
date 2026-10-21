#include "fh_command_copy.h"
#include <3ds.h>
#include <string.h>
#include "fh_pica_world_packet_owner.h"

static uint64_t epoch;
/* All packet producers are load-time native compilers. The final command is
 * their channel-1 return. Copy their payload into the SDK segment, excluding
 * that return; preserve any alignment NOP and every draw/state command.
 * SDK cache publication and frame retirement remain the resource owners. */
unsigned fh_pica_packet_leaf(const uint32_t *commands,unsigned count){
    if(!commands || count<4 || count>16384 || (count&3) ||
       commands[count-2]!=1 || commands[count-1]!=0x000f023d ||
       !gpuCmdBuf || (gpuCmdBufOffset&1) || gpuCmdBufOffset>gpuCmdBufSize ||
       count-2>gpuCmdBufSize-gpuCmdBufOffset)return 0;
    fh_command_copy(gpuCmdBuf+gpuCmdBufOffset,commands,count-2);
    gpuCmdBufOffset+=count-2;return count-2;
}
unsigned fh_pica_packet_append(const FhPicaWorldPacket *p){
    if(!p || !p->commands || !p->count || p->count>FH_PICA_WORLD_PACKET_DRAWS ||
       !p->words || p->words>FH_PICA_WORLD_PACKET_WORDS)return 0;
    return fh_pica_packet_leaf(p->commands,p->words);
}
int fh_pica_packet_publish(uint32_t *segment,unsigned bytes){
    return segment && bytes && !(bytes&15);
}
int fh_pica_packet_frame_retired(void){
    if(epoch==UINT64_MAX)return 0;
    ++epoch;return 1;
}
uint64_t fh_pica_packet_epoch(void){return epoch;}
