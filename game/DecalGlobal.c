#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022b94-0x80022b9c.
void DecalGlobal_EmptyFunc_MainFrame_ResetDB(void)
{
}


#if defined(__LP64__)
#define MAX_CACHED_ICON_GROUPS 64
#define MAX_ICONS_PER_GROUP 128

typedef struct {
	const struct IconGroup *ig;
	struct Icon *icons[MAX_ICONS_PER_GROUP];
} NativeIconGroupEntry;

static NativeIconGroupEntry s_iconGroupCache[MAX_CACHED_ICON_GROUPS];
static int s_iconGroupCacheCount = 0;
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022b9c-0x80022bdc.
void DecalGlobal_Clear(struct GameTracker *gGT)
{
	memset(&gGT->ptrIcons, 0, sizeof(gGT->ptrIcons));
	memset(&gGT->iconGroup, 0, sizeof(gGT->iconGroup));
#if defined(__LP64__)
	s_iconGroupCacheCount = 0;
	memset(s_iconGroupCache, 0, sizeof(s_iconGroupCache));
#endif
}


#if defined(__LP64__)
struct Icon **DecalGlobal_GetIcons(struct IconGroup *ig)
{
	if (ig == NULL)
		return NULL;

	for (int i = 0; i < s_iconGroupCacheCount; i++)
	{
		if (s_iconGroupCache[i].ig == ig)
			return &s_iconGroupCache[i].icons[0];
	}

	int slot = s_iconGroupCacheCount;
	if (slot >= MAX_CACHED_ICON_GROUPS)
		slot = 0;
	else
		s_iconGroupCacheCount++;

	NativeIconGroupEntry *entry = &s_iconGroupCache[slot];
	entry->ig = ig;

	uintptr_t base_hi = (uintptr_t)ig & 0xFFFFFFFF00000000ULL;
	if (base_hi == 0)
		base_hi = 0x100000000ULL;

	const uint32_t *raw32 = (const uint32_t *)((uintptr_t)ig + sizeof(struct IconGroup));
	int count = ig->numIcons;
	if (count > MAX_ICONS_PER_GROUP)
		count = MAX_ICONS_PER_GROUP;
	if (count < 0)
		count = 0;

	for (int j = 0; j < count; j++)
	{
		uint32_t val = raw32[j];
		if (val != 0)
			entry->icons[j] = (struct Icon *)(base_hi | (uintptr_t)val);
		else
			entry->icons[j] = NULL;
	}
	for (int j = count; j < MAX_ICONS_PER_GROUP; j++)
		entry->icons[j] = NULL;

	return &entry->icons[0];
}
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022bdc-0x80022c88.
void DecalGlobal_Store(struct GameTracker *gGT, struct LevTexLookup *LTL)
{
	if (LTL == NULL)
		return;

#if defined(__LP64__)
	uintptr_t base_hi = (uintptr_t)LTL & 0xFFFFFFFF00000000ULL;
	const uint32_t *raw = (const uint32_t *)LTL;
	uint32_t numIcon = raw[0];
	uintptr_t firstIconAddr = base_hi | (uintptr_t)raw[1];
	uint32_t numIconGroup = raw[2];
	uintptr_t firstIconGroupPtrAddr = base_hi | (uintptr_t)raw[3];

	struct Icon *currIcon = (struct Icon *)firstIconAddr;
	for (uint32_t i = 0; i < numIcon; i++, currIcon++)
	{
		if ((u32)currIcon->global_IconArray_Index < 0x88)
			gGT->ptrIcons[currIcon->global_IconArray_Index] = currIcon;
	}

	const uint32_t *groupPtrs32 = (const uint32_t *)firstIconGroupPtrAddr;
	for (uint32_t i = 0; i < numIconGroup; i++)
	{
		struct IconGroup *grp = (struct IconGroup *)(base_hi | (uintptr_t)groupPtrs32[i]);
		if (grp != NULL && (u32)grp->groupID < 0x11)
		{
			gGT->iconGroup[grp->groupID] = grp;
			DecalGlobal_GetIcons(grp);
		}
	}
#else
	struct Icon *currIcon;
	struct IconGroup **currGroup;

	for (
	    // array of Icon
	    currIcon = &LTL->firstIcon[0]; currIcon < &LTL->firstIcon[LTL->numIcon]; currIcon++)
	{
		// uint, in case of negatives
		if ((u32)currIcon->global_IconArray_Index < 0x88)
			gGT->ptrIcons[currIcon->global_IconArray_Index] = currIcon;
	}

	for (
	    // array of POINTER to iconGroup
	    currGroup = &LTL->firstIconGroupPtr[0]; currGroup < &LTL->firstIconGroupPtr[LTL->numIconGroup]; currGroup++)
	{
		// use '[0]' to dereference pointer
		if ((u32)currGroup[0]->groupID < 0x11)
			gGT->iconGroup[currGroup[0]->groupID] = currGroup[0];
	}
#endif
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022c88-0x80022d2c.
int *DecalGlobal_FindInLEV(struct Level *level, char *str)
{
	struct LevTexLookup *ltl = level->levTexLookup;

	if (ltl == NULL)
		return NULL;

#if defined(__LP64__)
	uintptr_t base_hi = (uintptr_t)ltl & 0xFFFFFFFF00000000ULL;
	const uint32_t *raw = (const uint32_t *)ltl;
	uint32_t numIconGroup = raw[2];
	const uint32_t *groupPtrs32 = (const uint32_t *)(base_hi | (uintptr_t)raw[3]);

	for (uint32_t i = 0; i < numIconGroup; i++)
	{
		struct IconGroup *group = (struct IconGroup *)(base_hi | (uintptr_t)groupPtrs32[i]);
		if (group != NULL &&
		    *(int *)&group->name[0x0] == *(int *)&str[0x0] && *(int *)&group->name[0x4] == *(int *)&str[0x4] &&
		    *(int *)&group->name[0x8] == *(int *)&str[0x8] && *(int *)&group->name[0xc] == *(int *)&str[0xc])
		{
			return (int *)group;
		}
	}
#else
	struct IconGroup **curr = ltl->firstIconGroupPtr;
	struct IconGroup **end = &ltl->firstIconGroupPtr[ltl->numIconGroup];

	for (; curr < end; curr++)
	{
		struct IconGroup *group = *curr;

		if (*(int *)&group->name[0x0] == *(int *)&str[0x0] && *(int *)&group->name[0x4] == *(int *)&str[0x4] &&
		    *(int *)&group->name[0x8] == *(int *)&str[0x8] && *(int *)&group->name[0xc] == *(int *)&str[0xc])
		{
			return (int *)group;
		}
	}
#endif

	return NULL;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022d2c-0x80022db0.
int *DecalGlobal_FindInMPK(u32 *icons, char *str)
{
	struct Icon *icon = (struct Icon *)icons;

	for (; icon->name[0] != '\0'; icon++)
	{
		if (*(int *)&icon->name[0x0] == *(int *)&str[0x0] && *(int *)&icon->name[0x4] == *(int *)&str[0x4] && *(int *)&icon->name[0x8] == *(int *)&str[0x8] &&
		    *(int *)&icon->name[0xc] == *(int *)&str[0xc])
		{
			return (int *)icon;
		}
	}

	return NULL;
}
