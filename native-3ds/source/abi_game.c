#include <dolphin/types.h>
#include <dolphin/dvd.h>
#include <dolphin/gx.h>
#include <dolphin/pad.h>
#include <dolphin/mtx.h>
#include <stddef.h>
#include "main/model.h"
#include "fh_platform.h"
_Static_assert(sizeof(void*)==4,"Game pointers must be 32 bit");
_Static_assert(sizeof(ModelDisplayListEntry)==28 && offsetof(ModelDisplayListEntry,dlistSize)==4,"Native model display-list layout");
_Static_assert(sizeof(BOOL)==4 && sizeof(bool)==1,"Boolean ABI");
_Static_assert(sizeof(GXAttr)==4 && sizeof(GXTexFmt)==4,"Game enum ABI");
_Static_assert(sizeof(Mtx)==48 && sizeof(Mtx44)==64 && sizeof(Vec)==12,"Matrix ABI");
_Static_assert(sizeof(GXTexObj)==64 && sizeof(GXTlutObj)==40,"Foxhollow texture object ABI");
_Static_assert(sizeof(GXRenderModeObj)==60,"Render-mode ABI");
_Static_assert(sizeof(DVDCommandBlock)==48 && sizeof(DVDFileInfo)==60,"DVD ABI");
_Static_assert(sizeof(PADStatus)==16 && sizeof(FhPadState)==16,"Input bridge ABI");
_Static_assert(offsetof(PADStatus,extButton)==offsetof(FhPadState,extended),"Input extended buttons ABI");
_Static_assert(offsetof(PADStatus,err)==offsetof(FhPadState,error),"Input error ABI");
