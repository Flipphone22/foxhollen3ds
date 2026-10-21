/* Altered libctru ndsp.c: Foxhollow audio cost timing, AUX effect servicing,
 * DSP processor selection and logged fault handling.
 * Original: devkitPro/libctru, libctru/source/ndsp/ndsp.c, as shipped in
 * devkitpro/devkitarm@sha256:15b79ce75822c289538d8153da5fa7aafe5e6adc32ad8a575a197beca0f0761b.
 * See native-3ds/licenses/libctru-zlib.txt. */
#include "ndsp-internal.h"
#include "fh_audio_dsp_aux.h"
#include "fh_audio_cost.h"
static u64 cost_state_ticks;
#include <3ds/services/cfgu.h>
#include <3ds/services/fs.h>
#include <3ds/env.h>
#include <3ds/thread.h>

#define NDSP_THREAD_STACK_SIZE 0x1000

u16 ndspFrameId, ndspBufferCurId, ndspBufferId;
void* ndspVars[16][2];

static bool bDspReady, bEnteringSleep, bSleeping, bCancelReceived;
static u32 droppedFrames, frameCount;

static const void* componentBin;
static u32 componentSize;
static u16 componentProgMask, componentDataMask;
static bool componentFree;

static dspHookCookie ndspHookCookie;

static Handle irqEvent, dspSem;
static LightEvent sleepEvent;
static LightLock ndspMutex;

static u8 dspVar5Backup[0x1080];

static volatile bool ndspThreadRun;
static Thread ndspThread;
static Handle ndspThreadHandle;
static Handle ndspStopEvent;

static inline bool ndspWaitForIrq(u64 timeout_ns,bool stoppable)
{
	LightLock_Lock(&ndspMutex);

	// BUG: Official sw has a race condition here when entering sleep mode. DSP state might
	// have already been torn down, and thus this ends up waiting on an invalid (0) handle.
	// There's code that tries to panic if an error happens, however said error handler fails
	// to actually panic because it checks that the result code level is specifically 'Fatal'.
	// Note that the "invalid handle" result code has a 'Permanent' level...
	// We will instead handle invalid handles properly and immediately return.
	bool waitOk = false;
	if (irqEvent)
	{
		Handle events[2]={ndspStopEvent,irqEvent};
        s32 selected=1;
        Result rc = stoppable ? svcWaitSynchronizationN(&selected,events,2,false,timeout_ns)
                             : svcWaitSynchronization(irqEvent,timeout_ns);
		if (R_FAILED(rc))
			svcBreak(USERBREAK_PANIC); // Shouldn't happen.

		waitOk = R_DESCRIPTION(rc) != RD_TIMEOUT && selected==1;
	}

	if (waitOk)
		svcClearEvent(irqEvent);

	LightLock_Unlock(&ndspMutex);
	return waitOk;
}

static inline void ndspSetCounter(int a, int counter)
{
	*(vu16*)ndspVars[0][a] = counter;
}

static inline int ndspGetCounter(int a)
{
	return *(vu16*)ndspVars[0][a];
}

enum
{
	MFLAG_MASTERVOL    = BIT(0),
	MFLAG_OUTPUTMODE   = BIT(1),
	MFLAG_CLIPPINGMODE = BIT(2),
	MFLAG_OUTPUTCOUNT  = BIT(3),
	MFLAG_SYNCMODE     = BIT(4),
	MFLAG_SURR_DEPTH   = BIT(5),
	MFLAG_SURR_POS     = BIT(6),
	MFLAG_SURR_RRATIO  = BIT(7),

#define MFLAG_AUX_ENABLE(i) BIT(8+(i))
#define MFLAG_AUX_BYPASS(i) BIT(10+(i))
#define MFLAG_AUX_VOLUME(i) BIT(12+(i))
};

static struct
{
	LightLock lock;
	u32 flags;
	float masterVol;
	u16 outputMode, clippingMode, outputCount, syncMode;
	ndspWaveBuf* capture;
	ndspCallback callback;
	void* callbackData;

	struct
	{
		u16 depth, pos, rearRatio;
	} surround;

	struct
	{
		u16 enable, frontBypass;
		float volume;
		ndspAuxCallback callback;
		void* callbackData;
	} aux[2];
} ndspMaster;

static void ndspDirtyMaster(void)
{
	ndspMaster.flags = ~0;
}

static void ndspInitMaster(void)
{
	memset(&ndspMaster, 0, sizeof(ndspMaster));
	LightLock_Init(&ndspMaster.lock);
	ndspMaster.flags = ~0;
	ndspMaster.masterVol = 1.0f;
	ndspMaster.outputMode = NDSP_OUTPUT_STEREO;
	ndspMaster.clippingMode = NDSP_CLIP_SOFT;
	ndspMaster.outputCount = 2;
	ndspMaster.surround.depth = 0x7FFF;
	ndspMaster.surround.rearRatio = 0x8000;

	// Use the output mode set in system settings, if available
	Result rc = cfguInit();
	if (R_SUCCEEDED(rc))
	{
		u8 outMode;
		rc = CFGU_GetConfigInfoBlk2(sizeof(outMode), 0x70001, &outMode);
		if (R_SUCCEEDED(rc))
			ndspMaster.outputMode = outMode;
		cfguExit();
	}
}

static void ndspUpdateMaster(void)
{
	DspMasterStatus* m = ndspiGetMasterStatus();
	LightLock_Lock(&ndspMaster.lock);

	u32 flags = m->flags, mflags = ndspMaster.flags;
	int i;

	m->headsetConnected = osIsHeadsetConnected();
	flags |= 0x10000000;

	if (mflags & MFLAG_MASTERVOL)
	{
		m->masterVol = ndspMaster.masterVol;
		flags |= 0x00010000;
	}

	if (mflags & MFLAG_OUTPUTMODE)
	{
		m->outputMode = ndspMaster.outputMode;
		flags |= 0x04000000;
	}

	if (mflags & MFLAG_CLIPPINGMODE)
	{
		m->clippingMode = ndspMaster.clippingMode;
		flags |= 0x08000000;
	}

	if (mflags & MFLAG_OUTPUTCOUNT)
	{
		m->outBufCount = ndspMaster.outputCount;
		flags |= 0x00008000;
	}

	if (mflags & MFLAG_SYNCMODE)
	{
		m->syncMode = ndspMaster.syncMode;
		m->unknown |= 0x10000; //?
	}

	if (mflags & MFLAG_SURR_DEPTH)
	{
		m->surroundDepth = ndspMaster.surround.depth;
		flags |= 0x20000000;
	}

	if (mflags & MFLAG_SURR_POS)
	{
		m->surroundSpeakerPos = ndspMaster.surround.pos;
		flags |= 0x40000000;
	}

	if (mflags & MFLAG_SURR_RRATIO)
	{
		m->rearRatio = ndspMaster.surround.rearRatio;
		flags |= 0x80000000;
	}

	for (i = 0; i < 2; i ++)
	{
		if (mflags & MFLAG_AUX_ENABLE(i))
		{
			m->auxBusEnable[i] = ndspMaster.aux[i].enable;
			flags |= 0x00000100 << i;
		}

		if (mflags & MFLAG_AUX_BYPASS(i))
		{
			m->auxFrontBypass[i] = ndspMaster.aux[i].frontBypass;
			flags |= 0x00000040 << i;
		}

		if (mflags & MFLAG_AUX_VOLUME(i))
		{
			m->auxReturnVol[i] = ndspMaster.aux[i].volume;
			flags |= 0x01000000 << i;
		}
	}

	m->flags = flags;
	ndspMaster.flags = 0;

	LightLock_Unlock(&ndspMaster.lock);
}

static void ndspUpdateCapture(s16* samples, u32 count)
{
	ndspWaveBuf* buf = ndspMaster.capture;
	if (!buf) return;
	memcpy(&buf->data_pcm16[buf->offset*2], samples, count*4);
	buf->offset += count;
	if (buf->offset >= buf->nsamples)
		buf->offset = 0;
}

static Result ndspInitialize(bool resume)
{
	Result rc;

	rc = svcCreateEvent(&irqEvent, RESET_STICKY);
	if (R_FAILED(rc)) goto _fail1;

	rc = DSP_RegisterInterruptEvents(irqEvent, DSP_INTERRUPT_PIPE, 2);
	if (R_FAILED(rc)) goto _fail2;

	rc = DSP_GetSemaphoreHandle(&dspSem);
	if (R_FAILED(rc)) goto _fail3;

	DSP_SetSemaphoreMask(0x2000);

	if (resume)
	{
		memcpy(ndspVars[5][0], dspVar5Backup, sizeof(dspVar5Backup));
		__dsb();
	}

	u16 val = resume ? 2 : 0;
	DSP_WriteProcessPipe(2, &val, 4);

	DSP_SetSemaphore(0x4000);
	ndspWaitForIrq(U64_MAX,false);

	DSP_ReadPipeIfPossible(2, 0, &val, sizeof(val), NULL);

	u16 vars[16];
	DSP_ReadPipeIfPossible(2, 0, vars, val*2, NULL);
	for (unsigned i = 0; i < val; i ++)
	{
		DSP_ConvertProcessAddressFromDspDram(vars[i],           (u32*)&ndspVars[i][0]);
		DSP_ConvertProcessAddressFromDspDram(vars[i] | 0x10000, (u32*)&ndspVars[i][1]);
	}

	DSP_SetSemaphore(0x4000);
	frameCount = 0;
	ndspFrameId = 4;
	ndspSetCounter(0, 4);
	ndspFrameId++;
	svcSignalEvent(dspSem);

	ndspBufferCurId = ndspFrameId & 1;
	ndspBufferId = ndspFrameId & 1;
	bDspReady = true;

	if (resume)
	{
		ndspiDirtyChn();
		ndspiUpdateChn();

		ndspDirtyMaster();
		ndspUpdateMaster();

		// TODO: force update effect params
	}
	return 0;

_fail3:
	DSP_RegisterInterruptEvents(0, 2, 2);
_fail2:
	svcCloseHandle(irqEvent);
_fail1:
	return rc;
}

static void ndspFinalize(bool suspend)
{
	u16 val = suspend ? 3 : 1;
	DSP_WriteProcessPipe(2, &val, 4);

	for (;;)
	{
		bool ready = false;
		DSP_RecvDataIsReady(0, &ready);
		if (ready)
		{
			val = 0;
			DSP_RecvData(0, &val);
			if (val == 1)
				break;
		}
		svcSleepThread(4888000); // 4.888ms (approx. one sound frame)
	}

	if (suspend)
		memcpy(dspVar5Backup, ndspVars[5][0], sizeof(dspVar5Backup));

	LightLock_Lock(&ndspMutex);
	bDspReady = false;

	svcCloseHandle(irqEvent);
	irqEvent = 0;
	DSP_RegisterInterruptEvents(0, DSP_INTERRUPT_PIPE, 2);

	svcCloseHandle(dspSem);
	dspSem = 0;

	LightLock_Unlock(&ndspMutex);
}

static void ndspHookCallback(DSP_HookType hook)
{
	switch (hook)
	{
		case DSPHOOK_ONSLEEP:
			if (!bSleeping)
			{
				bEnteringSleep = true;
				ndspFinalize(true);
				bSleeping = true;
			}
			break;

		case DSPHOOK_ONWAKEUP:
			if (bSleeping)
			{
				Result res = ndspInitialize(true);
				if (R_FAILED(res))
					svcBreak(USERBREAK_PANIC); // Shouldn't happen.
				{extern unsigned fh_audio_resume_reset;__atomic_store_n(&fh_audio_resume_reset,1,__ATOMIC_RELEASE);}

				bSleeping = false;
				bEnteringSleep = false;
				__dsb();
				LightEvent_Signal(&sleepEvent);
			}
			break;

		case DSPHOOK_ONCANCEL:
			if (bSleeping)
			{
				bCancelReceived = true;
				bSleeping = false;
				bEnteringSleep = false;
				__dsb();
				LightEvent_Signal(&sleepEvent);
			}
			break;

		default:
			break;
	}
}

static void ndspSync(void)
{
	// If we are about to sleep...
	if (bEnteringSleep)
	{
		// Check whether the DSP is still running by attempting to wait with a timeout
		if (ndspWaitForIrq(9776000,true)) // 9.776ms (approx. two sound frames)
			goto _receiveState; // it's not, so just proceed as usual

		// The wait failed, so the DSP is indeed sleeping
		bSleeping = true;
	}

	// If we are sleeping, wait for the DSP to wake up
	if (bSleeping)
	{
		LightEvent_Wait(&sleepEvent);
		LightEvent_Clear(&sleepEvent);
	}
    if (!__atomic_load_n(&ndspThreadRun,__ATOMIC_ACQUIRE)) return;

	// If we were cancelled, do a dummy wait instead
	if (bCancelReceived)
	{
		svcSleepThread(4888000); // 4.888ms (approx. one sound frame)
		return;
	}

	// In any other case - wait for the DSP to notify us
	ndspWaitForIrq(U64_MAX,true);

_receiveState:
    if (!__atomic_load_n(&ndspThreadRun,__ATOMIC_ACQUIRE)) return;
	// Receive and update state (if the DSP is ready)
	if (bDspReady)
	{
		int counter = ndspGetCounter(~ndspFrameId & 1);
		if (counter)
		{
			int next = (counter + 1) & 0xFFFF;
			ndspFrameId = next ? next : 2;
			ndspBufferId = ndspFrameId & 1;
			u64 cost_state_start=fh_audio_cost_now();
			ndspiReadChnState();
			//memcpy(dspVar9Backup, dspVars[9][ndspBufferId], sizeof(dspVar9Backup));
			ndspUpdateCapture((s16*)ndspVars[6][ndspBufferId], 160);
			droppedFrames += *((u16*)ndspVars[5][ndspBufferId] + 1);
			cost_state_ticks=fh_audio_cost_now()-cost_state_start;
			fh_audio_cost_finish(FH_AC_STATE,cost_state_start);
		}
	}
}

static void ndspThreadMain(void* arg)
{
    (void)arg;
	while (__atomic_load_n(&ndspThreadRun,__ATOMIC_ACQUIRE))
	{
		cost_state_ticks=0;
		ndspSync();
        if (!__atomic_load_n(&ndspThreadRun,__ATOMIC_ACQUIRE)) break;
		u64 cost_active=fh_audio_cost_now(),cost_callback=cost_active;

		{
			void (*callback)(void*)=ndspMaster.callback;
			if (callback)
				callback(ndspMaster.callbackData);
		}

		fh_audio_cost_finish(FH_AC_CALLBACK,cost_callback);
		if (bSleeping || bCancelReceived || !bDspReady) {
			fh_audio_cost_finish(FH_AC_SDK_ACTIVE,cost_active-cost_state_ticks);
			continue;
		}

		ndspUpdateMaster();
		u64 cost_aux=fh_audio_cost_now();
		fh_audio_dsp_aux_service();
		fh_audio_cost_finish(FH_AC_AUX,cost_aux);
		// TODO: execute DSP effects
		u64 cost_publish=fh_audio_cost_now();
		ndspiUpdateChn();

		ndspSetCounter(ndspBufferCurId, ndspFrameId++);
		svcSignalEvent(dspSem);
		ndspBufferCurId = ndspFrameId & 1;

		frameCount++;
		fh_audio_cost_finish(FH_AC_PUBLISH,cost_publish);
		fh_audio_cost_finish(FH_AC_SDK_ACTIVE,cost_active-cost_state_ticks);
	}
}

void ndspUseComponent(const void* binary, u32 size, u16 progMask, u16 dataMask)
{
	componentBin = binary;
	componentSize = size;
	componentProgMask = progMask;
	componentDataMask = dataMask;
	componentFree = false;
}

static bool ndspFindAndLoadComponent(void)
{
	Result rc;
	Handle rsrc;
	void* bin;

	componentProgMask = 0xFF;
	componentDataMask = 0xFF;

	// Try loading the DSP component from the filesystem
	do
	{
		static const char dsp_filename[] = "/3ds/dspfirm.cdc";
		FS_Path archPath = { PATH_EMPTY, 1, "" };
		FS_Path filePath = { PATH_ASCII, sizeof(dsp_filename), dsp_filename };

		rc = FSUSER_OpenFileDirectly(&rsrc, ARCHIVE_SDMC, archPath, filePath, FS_OPEN_READ, 0);
		if (R_FAILED(rc)) break;

		u64 size = 0;
		rc = FSFILE_GetSize(rsrc, &size);
		if (R_FAILED(rc)) { FSFILE_Close(rsrc); break; }

		bin = malloc(size);
		if (!bin) { FSFILE_Close(rsrc); break; }

		u32 dummy = 0;
		rc = FSFILE_Read(rsrc, &dummy, 0, bin, size);
		FSFILE_Close(rsrc);
		if (R_FAILED(rc)) { free(bin); return false; }

		componentBin = bin;
		componentSize = size;
		componentFree = true;
		return true;
	} while (0);

	// Try loading the DSP component from hb:ndsp
	rsrc = envGetHandle("hb:ndsp");
	if (rsrc) do
	{
		extern char* fake_heap_end;
		u32 mapAddr = ((u32)fake_heap_end+0xFFF) &~ 0xFFF;
		rc = svcMapMemoryBlock(rsrc, mapAddr, MEMPERM_READWRITE, MEMPERM_READWRITE);
		if (R_FAILED(rc)) break;

		componentSize = *(u32*)(mapAddr + 0x104);
		bin = malloc(componentSize);
		if (bin)
			memcpy(bin, (void*)mapAddr, componentSize);
		svcUnmapMemoryBlock(rsrc, mapAddr);
		if (!bin) break;

		componentBin = bin;
		componentFree = true;
		return true;
	} while (0);

	return false;
}

static int ndspRefCount = 0;

Result ndspInit(void)
{
	Result rc = 0;
	if (AtomicPostIncrement(&ndspRefCount)) return 0;

	if (!componentBin && !ndspFindAndLoadComponent())
	{
		rc = MAKERESULT(RL_PERMANENT, RS_NOTFOUND, RM_DSP, RD_NOT_FOUND);
		goto _fail0;
	}

	LightLock_Init(&ndspMutex);
	LightEvent_Init(&sleepEvent, RESET_STICKY);

	rc = dspInit();
	if (R_FAILED(rc)) goto _fail1;

	rc = DSP_LoadComponent(componentBin, componentSize, componentProgMask, componentDataMask, NULL);
	if (R_FAILED(rc)) goto _fail2;

	rc = ndspInitialize(false);
	if (R_FAILED(rc)) goto _fail2;

	ndspiInitChn();
	ndspInitMaster();
	ndspUpdateMaster(); // official sw does this upfront, not sure what's the point
	// TODO: initialize effect params

	/* Keep the existing service thread, priority and DSP ownership. Only Old
     * hardware with a read-back CPU grant changes processor placement. */
    extern int fh_audio_ndsp_processor(void);
    int service_core=fh_audio_ndsp_processor();
    rc=svcCreateEvent(&ndspStopEvent,RESET_STICKY);
    if(R_FAILED(rc)) goto _fail3;
    /* Publish before creation: an immediate owner shutdown must not be
     * overwritten by a service thread that has not started yet. */
    __atomic_store_n(&ndspThreadRun,true,__ATOMIC_RELEASE);
    ndspThread = threadCreate(ndspThreadMain, 0x0, NDSP_THREAD_STACK_SIZE, 0x18, service_core, false);
    if(!ndspThread && service_core==1){
        service_core=0;
        ndspThread = threadCreate(ndspThreadMain, 0x0, NDSP_THREAD_STACK_SIZE, 0x18, 0, false);
    }
    if(ndspThread){
        extern void fh_platform_log(const char*);
        fh_platform_log(service_core==1?"NDSP service selected core 1; shared Old audio grant\n":
            service_core==0?"NDSP service selected core 0\n":"NDSP service retained default processor\n");
    }
    if (!ndspThread) {
        __atomic_store_n(&ndspThreadRun,false,__ATOMIC_RELEASE);
        rc=MAKERESULT(RL_PERMANENT,RS_OUTOFRESOURCE,RM_DSP,RD_OUT_OF_MEMORY);
        goto _fail3;
    }
    /* Cache the live kernel handle. libctru threadJoin may return when its
     * finished flag is set, before svcExitThread actually retires the stack. */
    ndspThreadHandle=threadGetHandle(ndspThread);

	dspHook(&ndspHookCookie, ndspHookCallback);
	return 0;

_fail3:
    if(ndspStopEvent){svcCloseHandle(ndspStopEvent);ndspStopEvent=0;}
	ndspFinalize(false);
_fail2:
	dspExit();
_fail1:
	if (componentFree)
	{
		free((void*)componentBin);
		componentBin = NULL;
	}
_fail0:
	AtomicDecrement(&ndspRefCount);
	return rc;
}

void ndspExit(void)
{
	if (AtomicDecrement(&ndspRefCount)) return;

    __atomic_store_n(&ndspThreadRun,false,__ATOMIC_RELEASE);
    svcSignalEvent(ndspStopEvent);
    LightEvent_Signal(&sleepEvent);
    Result joined=svcWaitSynchronization(ndspThreadHandle,U64_MAX);
    if(R_FAILED(joined)) {
        extern void fh_platform_panic(const char*) __attribute__((noreturn));
        fh_platform_panic("NDSP service retirement failed; resources retained");
    }
    threadFree(ndspThread);ndspThread=NULL;ndspThreadHandle=0;
    svcCloseHandle(ndspStopEvent);ndspStopEvent=0;

	dspUnhook(&ndspHookCookie);
	if (!bCancelReceived)
		ndspFinalize(false);

	bEnteringSleep = false;
	bSleeping = false;
	bCancelReceived = false;
	dspExit();

	if (componentFree)
	{
		free((void*)componentBin);
		componentBin = NULL;
	}
}

u32 ndspGetDroppedFrames(void)
{
	return droppedFrames;
}

u32 ndspGetFrameCount(void)
{
	return frameCount;
}

void ndspSetMasterVol(float volume)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.masterVol = volume;
	ndspMaster.flags |= MFLAG_MASTERVOL;
	LightLock_Unlock(&ndspMaster.lock);
}

float ndspGetMasterVol(void)
{
	return ndspMaster.masterVol;
}

void ndspSetOutputMode(ndspOutputMode mode)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.outputMode = mode;
	ndspMaster.flags |= MFLAG_OUTPUTMODE;
	LightLock_Unlock(&ndspMaster.lock);
}

ndspOutputMode ndspGetOutputMode(void)
{
	return ndspMaster.outputMode;
}

void ndspSetClippingMode(ndspClippingMode mode)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.clippingMode = mode;
	ndspMaster.flags |= MFLAG_CLIPPINGMODE;
	LightLock_Unlock(&ndspMaster.lock);
}

ndspClippingMode ndspGetClippingMode(void)
{
	return ndspMaster.clippingMode;
}

void ndspSetOutputCount(int count)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.outputCount = count;
	ndspMaster.flags |= MFLAG_OUTPUTCOUNT;
	LightLock_Unlock(&ndspMaster.lock);
}

int ndspGetOutputCount(void)
{
	return ndspMaster.outputCount;
}

void ndspSetCapture(ndspWaveBuf* capture)
{
	ndspMaster.capture = capture;
}

void ndspSetCallback(ndspCallback callback, void* data)
{
	ndspMaster.callback = callback;
	ndspMaster.callbackData = data;
}

void ndspSurroundSetDepth(u16 depth)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.surround.depth = depth;
	ndspMaster.flags |= MFLAG_SURR_DEPTH;
	LightLock_Unlock(&ndspMaster.lock);
}

u16 ndspSurroundGetDepth(void)
{
	return ndspMaster.surround.depth;
}

void ndspSurroundSetPos(ndspSpeakerPos pos)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.surround.pos = pos;
	ndspMaster.flags |= MFLAG_SURR_POS;
	LightLock_Unlock(&ndspMaster.lock);
}

ndspSpeakerPos ndspSurroundGetPos(void)
{
	return ndspMaster.surround.pos;
}

void ndspSurroundSetRearRatio(u16 ratio)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.surround.rearRatio = ratio;
	ndspMaster.flags |= MFLAG_SURR_RRATIO;
	LightLock_Unlock(&ndspMaster.lock);
}

u16 ndspSurroundGetRearRatio(void)
{
	return ndspMaster.surround.rearRatio;
}

void ndspAuxSetEnable(int id, bool enable)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.aux[id].enable = enable ? 1 : 0;
	ndspMaster.flags |= MFLAG_AUX_ENABLE(id);
	LightLock_Unlock(&ndspMaster.lock);
}

bool ndspAuxIsEnabled(int id)
{
	return ndspMaster.aux[id].enable;
}

void ndspAuxSetFrontBypass(int id, bool bypass)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.aux[id].frontBypass = bypass ? 1 : 0;
	ndspMaster.flags |= MFLAG_AUX_BYPASS(id);
	LightLock_Unlock(&ndspMaster.lock);
}

bool ndspGetFrontBypass(int id)
{
	return ndspMaster.aux[id].frontBypass;
}

void ndspAuxSetVolume(int id, float volume)
{
	LightLock_Lock(&ndspMaster.lock);
	ndspMaster.aux[id].volume = volume;
	ndspMaster.flags |= MFLAG_AUX_VOLUME(id);
	LightLock_Unlock(&ndspMaster.lock);
}

float ndspAuxGetVolume(int id)
{
	return ndspMaster.aux[id].volume;
}

void ndspAuxSetCallback(int id, ndspAuxCallback callback, void* data)
{
	ndspMaster.aux[id].callback = callback;
	ndspMaster.aux[id].callbackData = data;
}
