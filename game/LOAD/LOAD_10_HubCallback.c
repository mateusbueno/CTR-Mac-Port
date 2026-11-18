#include <common.h>

#if defined(__LP64__)
struct Level *LOAD_NativeLevel_Build(void *rawFile, int slot);
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031b14-0x80031b50.
void LOAD_HubCallback(struct LoadQueueSlot *lqs)
{
	sdata->load_inProgress = 0;
	LOAD_Callback_PatchMem(lqs);

#if defined(__LP64__)
	sdata->gGT->level2 = LOAD_NativeLevel_Build(sdata->ptrLevelFile, -1);
	sdata->gGT->visMem2 = sdata->gGT->level2 ? sdata->gGT->level2->visMem : NULL;
#else
	sdata->gGT->level2 = sdata->ptrLevelFile;
#endif
	MEMPACK_SwapPacks(sdata->gGT->activeMempackIndex);
}
