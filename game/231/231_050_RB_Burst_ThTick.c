#include <common.h>

static void RB_Burst_UpdateSlot(struct Instance **slot)
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
		{ if FRAME_GATE_TICK() inst->animFrame++; }
#else
		inst->animFrame++;
#endif
		return;
	}

	INSTANCE_Death(inst);
	*slot = NULL;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b1d2c-0x800b1e90.
void RB_Burst_ThTick(struct Thread *t)
{
	struct Instance **burst;
	burst = (struct Instance **)t->object;

	RB_Burst_UpdateSlot(&burst[1]);
	RB_Burst_UpdateSlot(&burst[2]);
	RB_Burst_UpdateSlot(&burst[0]);

	if ((burst[1] == NULL) && (burst[2] == NULL))
		t->flags |= 0x800;
}
