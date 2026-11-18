#include <common.h>
#ifdef CTR_NATIVE
#include <platform/native_netplay.h>
#endif

void MainInit_JitPoolsNew(struct GameTracker *gGT)
{
        // NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003b43c-0x8003b6d0 for the retail path.
        u32 gameMode = gGT->gameMode1;
        int poolScale = 0x800;
        if ((gameMode & ADVENTURE_ARENA) == 0)
        {
                poolScale = 0x1000;
                if ((gameMode & MAIN_MENU) != 0)
                        poolScale = 0x400;
        }

        int renderBucketSize = 0x800;
        if ((gameMode & ADVENTURE_ARENA) == 0)
        {
                renderBucketSize = 0x1000;
                if ((gameMode & MAIN_MENU) != 0)
                {
                        renderBucketSize = 0x400;
                        if (gGT->levelID == ADVENTURE_GARAGE)
                                renderBucketSize = 0x800;
                }
        }

        MEMPACK_PushState();

        JitPool_Init(&gGT->JitPools.thread, (renderBucketSize * 3) >> 7, sizeof(struct Thread), rdata.s_ThreadPool);
        /* Instance pool: must have InstDrawPerPlayer slots for ALL drivers
         * (including remote peers), not just the local viewport. If we only
         * allocate for 1 player in netplay, VehBirth_Player(1) for the
         * remote driver would write InstDrawPerPlayer[1] out of bounds.
         * Rendering loops still only iterate NUM_LOCAL_PLAYERS(gGT) times. */
        JitPool_Init(&gGT->JitPools.instance, renderBucketSize >> 5, sizeof(struct Instance) + (sizeof(struct InstDrawPerPlayer) * gGT->numPlyrCurrGame),
                     rdata.s_InstancePool);
        JitPool_Init(&gGT->JitPools.smallStack, (poolScale * 0x19) >> 10, 0x48 + (sizeof(struct Item) - 8), rdata.s_SmallStackPool);
        JitPool_Init(&gGT->JitPools.mediumStack, poolScale >> 7, 0x88 + (sizeof(struct Item) - 8), rdata.s_MediumStackPool);

        int numDriver = poolScale >> 9;
        if ((gameMode & MAIN_MENU) != 0)
                numDriver = 4;
#ifdef CTR_NATIVE
        int driverItemSize = (int)(sizeof(struct Driver) + sizeof(struct Item) + 0x20);
        JitPool_Init(&gGT->JitPools.largeStack, numDriver, driverItemSize, rdata.s_LargeStackPool);
#else
        JitPool_Init(&gGT->JitPools.largeStack, numDriver, 0x680 + (sizeof(struct Item) - 8), rdata.s_LargeStackPool);
#endif

        int numParticle = poolScale >> 5;
        JitPool_Init(&gGT->JitPools.particle, numParticle, sizeof(struct Particle), rdata.s_ParticlePool);
        JitPool_Init(&gGT->JitPools.oscillator, numParticle, 0x18, rdata.s_OscillatorPool);
        JitPool_Init(&gGT->JitPools.rain, poolScale >> 9, sizeof(struct RainLocal), rdata.s_RainPool);

#ifndef CTR_NATIVE
        gGT->ptrRenderBucketInstance = MEMPACK_AllocMem(renderBucketSize);
#else
        // NOTE(aalhendi): Dedicated static pool for native 64-bit entries to avoid corrupting rdata.
        static struct RenderBucketEntry s_nativeRenderBucketPool[2048];
        memset(s_nativeRenderBucketPool, 0, sizeof(s_nativeRenderBucketPool));
        gGT->ptrRenderBucketInstance = s_nativeRenderBucketPool;
#endif

        for (int i = 0; i < 3; i++)
        {
                struct JitPool *pool = (struct JitPool *)((char *)&gGT->JitPools.smallStack + (sizeof(struct JitPool) * i));
                struct Item *item = pool->free.first;
                while (item != NULL)
                {
                        void **objPtr = (void **)((char *)item + sizeof(struct Item));
                        *objPtr = objPtr;
                        item = item->next;
                }
        }

        /* Clip buffers: must exist for ALL drivers (including remote peers),
         * not just the local viewport. Using NUM_LOCAL_PLAYERS here would
         * leave PtrClipBuffer[1+] uninitialized; if the engine clips a
         * remote driver's instance it would dereference NULL. */
        for (int i = 0; i < gGT->numPlyrCurrGame; i++)
        {
                data.PtrClipBuffer[i] = MEMPACK_AllocMem(MainDB_GetClipSize(gGT->levelID, NUM_LOCAL_PLAYERS(gGT)) << 2);
        }
}
