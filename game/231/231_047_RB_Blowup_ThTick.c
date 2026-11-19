#include <common.h>

static void RB_Blowup_UpdateSlot(struct Instance **slot)
{
	struct Instance *inst;
	int nextFrame;

	inst = *slot;
	if (inst == NULL)
		return;

	nextFrame = inst->animFrame + 1;
	if (nextFrame < INSTANCE_GetNumAnimFrames(inst, 0))
	{
#ifdef CTR_NATIVE
		{ static int s_60fpsBlowupToggle = 0; if (!IS_NATIVE_60FPS || (s_60fpsBlowupToggle ^= 1)) inst->animFrame++; }
#else
		inst->animFrame++;
#endif
		return;
	}

	INSTANCE_Death(inst);
	*slot = NULL;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b17f0-0x800b18f8.
void RB_Blowup_ThTick(struct Thread *t)
{
	struct Instance **blowup;
	blowup = (struct Instance **)t->object;

	RB_Blowup_UpdateSlot(&blowup[1]);
	RB_Blowup_UpdateSlot(&blowup[0]);

	if ((blowup[1] == NULL) && (blowup[0] == NULL))
		t->flags |= 0x800;

	ThTick_FastRET(t);
}
