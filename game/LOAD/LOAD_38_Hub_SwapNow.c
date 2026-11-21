#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033108-0x80033318.
void LOAD_Hub_SwapNow()
{
	struct Level *level1;
	struct VisMem *visMem;
	struct CameraDC *cDC;
	struct GameTracker *gGT = sdata->gGT;

	// stall until load is done
	while (gGT->level2 == 0)
	{
		LOAD_NextQueuedFile();
		VSync(0);
	}

	// Aug 5
	// ptrintf("gGT->level2 = 0x%08x\n",gGT->level2);
	// ptrintf("SWAPPING 1...\n");

#ifdef CTR_NATIVE
	{
		extern void Particle_NativeClearAll(void);
		Particle_NativeClearAll();
	}
	/* Instances still alive that use a model of the hub we are leaving would draw freed
	 * model data after the swap (stray dots / render crash). Kill them with the old hub. */
	{
		struct Level *oldLev = gGT->level1;
		struct JitPool *ip = &gGT->JitPools.instance;
		int killedThreads = 0, killedBare = 0;

		if (oldLev != NULL && oldLev->ptrModelsPtrArray != NULL)
		{
			struct Item *it = ip->taken.first;
			while (it != NULL && !CTR_BAD_PTR(it))
			{
				struct Item *next = it->next;
				struct Instance *ti = (struct Instance *)it;
				int isOld = 0;

				for (int m = 0; m < oldLev->numModels; m++)
				{
					if (oldLev->ptrModelsPtrArray[m] != NULL && oldLev->ptrModelsPtrArray[m] == ti->model)
					{
						isOld = 1;
						break;
					}
				}

				if (isOld)
				{
					if (!CTR_BAD_PTR(ti->thread) && (((uintptr_t)ti->thread & 3) == 0))
					{
						ti->thread->flags |= 0x800;
						killedThreads++;
					}
					else
					{
						INSTANCE_Death(ti);
						killedBare++;
					}
				}
				it = next;
			}
		}
		fprintf(stderr, "[CTR Native] HubSwap: killed %d threaded + %d bare old-hub instances\n", killedThreads, killedBare);
	}
	fprintf(stderr, "[CTR Native] HubSwap: before RePack instance pool free=%d taken=%d max=%d\n",
	        (int)gGT->JitPools.instance.free.count, (int)gGT->JitPools.instance.taken.count, gGT->JitPools.instance.maxItems);
#endif
	LevInstDef_RePack(gGT->level1->ptr_mesh_info, 1);
#ifdef CTR_NATIVE
	fprintf(stderr, "[CTR Native] HubSwap: after RePack free=%d taken=%d\n",
	        (int)gGT->JitPools.instance.free.count, (int)gGT->JitPools.instance.taken.count);
#endif

	// Aug 5
	// ptrintf("SWAPPING 2...\n");

	LOAD_HubSwapPtrs(gGT);

	// 0,1,2
	gGT->activeMempackIndex = 3 - gGT->activeMempackIndex;

	gGT->prevLEV = gGT->levelID;
	gGT->levelID = gGT->levID_in_each_mempack[gGT->activeMempackIndex];

	Audio_AdvHub_SwapSong(gGT->levelID);

	// Aug 5
	// ptrintf("SWAPPING 3...\n");

	LibraryOfModels_Clear(gGT);

	/*
	In Aug 5
	if (sdata->PLYROBJECTLIST == 0)
	{
	    printf("ERROR: No PLYROBJECTLIST!\n");
	}
	*/

	if (sdata->PLYROBJECTLIST != 0)
	{
		LOAD_GlobalModelPtrs_MPK();
	}

	level1 = gGT->level1;

	/*
	In Aug 5
	if (level1 == 0)
	{
	    printf("ERROR: No LEVEL!\n");
	}
	*/

	if (level1 != 0)
	{
		LibraryOfModels_Store(gGT, level1->numModels, level1->ptrModelsPtrArray);

		INSTANCE_LevInitAll(level1->ptrInstDefs, level1->numInstances);
#ifdef CTR_NATIVE
		fprintf(stderr, "[CTR Native] HubSwap: after LevInitAll(%d) free=%d taken=%d\n", (int)level1->numInstances,
		        (int)gGT->JitPools.instance.free.count, (int)gGT->JitPools.instance.taken.count);
#endif

		LevInstDef_UnPack(level1->ptr_mesh_info);

		DecalGlobal_Store(gGT, level1->levTexLookup);
	}

	MEMPACK_SwapPacks(gGT->activeMempackIndex);
	MainInit_VisMem(gGT);

	cDC = &gGT->cameraDC[0];
	cDC->ptrQuadBlock = 0;
	cDC->visLeafSrc = 0;
	cDC->visFaceSrc = 0;
	cDC->visInstSrc = 0;
	cDC->visOVertSrc = 0;
	cDC->visSCVertSrc = 0;

	visMem = gGT->visMem1;
	if (visMem != NULL)
	{
		visMem->visLeafSrc[0] = 0;
		visMem->visFaceSrc[0] = 0;
		visMem->visOVertSrc[0] = 0;
		visMem->visSCVertSrc[0] = 0;
	}

	if (gGT->drivers[0] != NULL)
		gGT->drivers[0]->underDriver = 0;

	gGT->framesInThisLEV = 0;
	gGT->msInThisLEV = 0;
}
