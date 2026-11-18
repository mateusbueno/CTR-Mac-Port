#include <common.h>

/* Minimum/maximum valid pointer bounds for the native platform.
 * MEMPACK arena: [0x00b67f20, 0x01000000)
 * Heap (malloc): >= 0x10000000
 * Anything outside these ranges is a corrupted pointer. */
#define ANIMTEX_MIN_VALID  0x00b67f20u
#define ANIMTEX_MAX_MEMPACK 0x01000000u

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021a20-0x80021ac0.
void CTR_CycleTex_Model(struct AnimTex *animtex, int timer)
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

		uint32_t ptrActiveTex32 = firstWord;
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

		if (ptrActiveTex32 != 0)
		{
			uint32_t *dst = (uint32_t *)(base_hi | (uintptr_t)ptrActiveTex32);
			*dst = activeTex32;
		}

		cur += 12 + numFrames * 4;
	}
#else
	int frameCurr;
	struct AnimTex *curAnimTex = animtex;

	/* Validate animtex pointer — corrupted headers (e.g. ptrMap
	 * applied to zero-valued fields) can produce garbage like
	 * 0x7534a680 that would crash on dereference. */
	uintptr_t addr = (uintptr_t)animtex;
	if (addr < ANIMTEX_MIN_VALID || (addr >= ANIMTEX_MAX_MEMPACK && addr < 0x10000000u))
		return;

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
		// For Model, this is a pointer to a pointer
		*curAnimTex->ptrActiveTex = (int)ptrArray[frameCurr];

		// Go to next AnimTex, which comes after this AnimTex's ptrarray
		curAnimTex = (struct AnimTex *)&ptrArray[curAnimTex->numFrames];
	}
#endif
}
