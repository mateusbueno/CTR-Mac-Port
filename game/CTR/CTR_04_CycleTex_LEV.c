#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021984-0x80021a20.
void CTR_CycleTex_LEV(struct AnimTex *animtex, int timer)
{
#if defined(__LP64__)
	if (animtex == NULL)
		return;

	uintptr_t base_hi = (uintptr_t)animtex & 0xFFFFFFFF00000000ULL;
	uint32_t firstRel = (uint32_t)(uintptr_t)animtex;
	char *cur = (char *)animtex;

	int safety = 0;
	while (safety++ < 128)
	{
		uint32_t firstWord = *(uint32_t *)cur;
		if (firstWord == firstRel)
			break;

		s16 numFrames = *(s16 *)(cur + 4);
		s16 frameOffset = *(s16 *)(cur + 6);
		s16 frameSkip = *(s16 *)(cur + 8);

		if (numFrames <= 0 || numFrames > 256)
			break;

		int frameCurr = (timer + frameOffset) >> frameSkip;
		frameCurr = frameCurr % numFrames;
		*(s16 *)(cur + 10) = (s16)frameCurr;

		uint32_t *ptrArray32 = (uint32_t *)(cur + 12);
		uint32_t activeTex32 = ptrArray32[frameCurr];

		// For levels, this is just a pointer saved into ptrActiveTex (first word of cur)
		*(uint32_t *)cur = activeTex32;

		cur += 12 + numFrames * 4;
	}
#else
	int frameCurr;
	struct AnimTex *curAnimTex = animtex;

	// Termination is determined by pointer to First AnimTex
	while (*(int *)curAnimTex != (int)animtex)
	{
		// which texture to draw this frame
		frameCurr = timer + curAnimTex->frameOffset;

		// allow frames to skip updating (like 60fps hacks)
		frameCurr = frameCurr >> curAnimTex->frameSkip;

		// loop back to index[0] after finished cycle
		frameCurr = frameCurr % curAnimTex->numFrames;

		// save result
		curAnimTex->frameCurr = frameCurr;

		struct IconGroup4 **ptrArray = ANIMTEX_GETARRAY(curAnimTex);

		// Save new frame
		// For levels, this is just a pointer
		curAnimTex->ptrActiveTex = (int *)ptrArray[frameCurr];

		// Go to next AnimTex, which comes after this AnimTex's ptrarray
		curAnimTex = (struct AnimTex *)&ptrArray[curAnimTex->numFrames];
	}
#endif
}
