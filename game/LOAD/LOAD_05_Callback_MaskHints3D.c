#include <common.h>

#if defined(CTR_NATIVE) && defined(__LP64__)
struct Model *LOAD_NativeModel_ConvertPublic(void *rawModelPtr);
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031a50-0x80031a64.
void LOAD_Callback_MaskHints3D(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
#if defined(CTR_NATIVE) && defined(__LP64__)
	// The raw model file has 32-bit header/animation pointers. Convert it so
	// VehTalkMask_ThTick/VehFrameInst_GetNumAnimFrames can walk ptrAnimations.
	sdata->modelMaskHints3D = LOAD_NativeModel_ConvertPublic(lqs->ptrDestination);
#else
	sdata->modelMaskHints3D = (struct Model *)lqs->ptrDestination;
#endif
}
