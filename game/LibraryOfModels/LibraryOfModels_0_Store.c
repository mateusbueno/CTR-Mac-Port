#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003147c-0x800314c0.
void LibraryOfModels_Store(struct GameTracker *gGT, u32 numModels, struct Model **ptrModelArray)
{
	if (gGT == NULL || ptrModelArray == NULL)
		return;

	while (numModels != 0)
	{
		struct Model *m = *ptrModelArray;
		if (m == NULL)
			return;
		if (m->id >= 0 && (size_t)m->id < sizeof(gGT->modelPtr) / sizeof(gGT->modelPtr[0]))
			gGT->modelPtr[m->id] = m;
		numModels--;
		ptrModelArray++;
	}
}
