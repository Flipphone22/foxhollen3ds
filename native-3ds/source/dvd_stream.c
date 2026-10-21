#include <dolphin/dvd.h>
#include <dolphin/ai.h>
#include "fh_platform.h"

BOOL DVDPrepareStreamAsync(DVDFileInfo *file,u32 length,u32 offset,DVDCallback callback) {
    if(!file || !file->cb.userData || offset>=file->length) return 0;
    if(!length) length=file->length-offset;
    if(length>file->length-offset) return 0;
    /* The game prepares the PCM source via fhAIPrepareStream before this call. */
    if(offset || length!=file->length) fh_platform_panic("Partial DVD audio stream requires a source range");
    file->cb.command=DVD_COMMAND_INITSTREAM;file->cb.state=DVD_STATE_END;
    file->cb.offset=offset;file->cb.length=length;file->callback=callback;
    if(callback) callback(DVD_RESULT_IGNORED,file);
    return 1;
}
int DVDCancelStreamAsync(DVDCommandBlock *block,DVDCBCallback callback) {
    if(!block) return 0;
    block->state=DVD_STATE_CANCELED;
    if(callback) callback(DVD_RESULT_CANCELED,block);
    return 1;
}
int DVDStopStreamAtEndAsync(DVDCommandBlock *block,DVDCBCallback callback) {
    if(!block) return 0;
    block->state=DVD_STATE_END;
    if(callback) callback(DVD_RESULT_GOOD,block);
    return 1;
}
int DVDGetStreamPlayAddrAsync(DVDCommandBlock *block,DVDCBCallback callback) {
    if(!block) return 0;
    block->state=DVD_STATE_END;
    if(callback) callback(AIGetStreamPlayState(),block);
    return 1;
}
