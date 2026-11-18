#include <common.h>


void LevInstDef_UnPack(struct mesh_info *ptr_mesh_info)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003116c-0x80031268.
	int i;
	int numQuadBlock;
	struct QuadBlock *ptrQuadBlockArray;
	struct QuadBlock *qbCurr;
	struct InstDef **visInstSrc;
	struct Level *level1;

	numQuadBlock = ptr_mesh_info ? ptr_mesh_info->numQuadBlock : 0;
	ptrQuadBlockArray = ptr_mesh_info ? ptr_mesh_info->ptrQuadBlockArray : NULL;

#if !defined(__LP64__)
	// loop through all quadblocks
	for (i = 0; i < numQuadBlock; i++)
	{
		qbCurr = &ptrQuadBlockArray[i];

		if ((qbCurr->pvs != 0) && (qbCurr->pvs->visInstSrc != 0))
		{
			// loop through all instance pointers visible on quadblock
			for (visInstSrc = (struct InstDef **)qbCurr->pvs->visInstSrc; visInstSrc[0] != NULL; visInstSrc++)
			{
				visInstSrc[0] = (struct InstDef *)visInstSrc[0]->ptrInstance;
			}
		}
	}
#endif

	level1 = sdata->gGT->level1;

	if (level1 != NULL && level1->ptrInstDefPtrArray != NULL)
	{
		// loop through all instDef pointers in the LEV
		for (visInstSrc = level1->ptrInstDefPtrArray; visInstSrc[0] != NULL; visInstSrc++)
		{
			if (visInstSrc[0] != NULL)
				visInstSrc[0] = (struct InstDef *)visInstSrc[0]->ptrInstance;
		}
	}
}


void LevInstDef_RePack(struct mesh_info *ptr_mesh_info, int boolAdvHub)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031268-0x800313c8.
	int i;
	int numQuadBlock;
	struct QuadBlock *ptrQuadBlockArray;
	struct QuadBlock *qbCurr;
	struct Instance **visInstSrc;
	struct Level *level1;
	struct Thread *th;

	numQuadBlock = ptr_mesh_info ? ptr_mesh_info->numQuadBlock : 0;
	ptrQuadBlockArray = ptr_mesh_info ? ptr_mesh_info->ptrQuadBlockArray : NULL;

#if !defined(__LP64__)
	// loop through all quadblocks
	for (i = 0; i < numQuadBlock; i++)
	{
		qbCurr = &ptrQuadBlockArray[i];

		if ((qbCurr->pvs != 0) && (qbCurr->pvs->visInstSrc != 0))
		{
			// loop through all instance pointers visible on quadblock
			for (visInstSrc = qbCurr->pvs->visInstSrc; visInstSrc[0] != NULL; visInstSrc++)
			{
				visInstSrc[0] = (struct Instance *)visInstSrc[0]->instDef; // maybe `visInstSrc[0]->instDef->ptrInstance`?
			}
		}
	}
#endif

	level1 = sdata->gGT->level1;

	if (level1 != NULL && level1->ptrInstDefPtrArray != NULL)
	{
		// loop through all instDef pointers in the LEV
		for (visInstSrc = (struct Instance **)level1->ptrInstDefPtrArray; visInstSrc[0] != NULL; visInstSrc++)
		{
			struct Instance *inst = visInstSrc[0];
			if (inst == NULL) continue;
			struct InstDef *instDef = inst->instDef;

			// if on adv hub
			if (boolAdvHub != 0)
			{
				// kill thread if it exists
				th = inst->thread;
				if (th != 0)
					th->flags |= 0x800;

				// erase instance in pool
				LIST_AddFront(&sdata->gGT->JitPools.instance.free, (struct Item *)inst);
			}

			// go back to instDef
			visInstSrc[0] = (struct Instance *)instDef;
		}
	}

	PROC_CheckAllForDead();
}
