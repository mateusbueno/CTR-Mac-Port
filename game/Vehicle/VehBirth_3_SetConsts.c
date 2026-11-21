#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058a60-0x80058ba4.
void VehBirth_SetConsts(struct Driver *driver)
{
	u32 metaPhysSize;
	u32 i;
	struct MetaPhys *metaPhys;
	u8 *d;

	d = (u8 *)driver;

#if defined(__LP64__)
	// metaPhys->offset values are retail (32-bit) struct Driver offsets. On LP64 the
	// Driver is larger (pointer members before this block), so every constant lands
	// at a fixed delta from its retail offset. The 0x416..0x484 block has no
	// pointers inside, so one delta covers all 65 entries.
	const int physOffsetDelta = (int)offsetof(struct Driver, const_Gravity) - GRAVITY_OFFSET;
#else
	const int physOffsetDelta = 0;
#endif

        int engineID = data.MetaDataCharacters[data.characterIDs[driver->driverID]].engineID;

        for (i = 0; i < 65; i++)
        {
                metaPhys = &data.metaPhys[i];

                metaPhysSize = metaPhys->size;

                short srcVal;
                void *src;
                if (engineID >= 0 && engineID < 4)
                {
                        src = &metaPhys->value[engineID];
                }
                else
                {
                        /* ENGINE_UNLIMITED: pick the best value from all columns */
                        srcVal = metaPhys->value[0];
                        {
                                int j;
                                for (j = 1; j < 4; j++)
                                        if (metaPhys->value[j] > srcVal)
                                                srcVal = metaPhys->value[j];
                        }
                        src = &srcVal;
                }
                void *dst = &d[metaPhys->offset + physOffsetDelta];

		if (metaPhysSize == 1)
		{
			*(char *)dst = *(char *)src;
			continue;
		}

		if (metaPhysSize == 2)
		{
			*(s16 *)dst = *(s16 *)src;
			continue;
		}

		*(int *)dst = *(int *)src;
	}

	return;
}
