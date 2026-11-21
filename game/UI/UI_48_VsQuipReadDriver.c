#include <common.h>
#if defined(__LP64__)
#include <driver_retail_offsets.h>

// Quip metadata stores retail (32-bit) struct Driver offsets. Translate to the
// native offset: take the closest field at or before it and keep the remainder.
static int UI_VsQuipNativeOffset(int retailOffset)
{
	int best = 0;
	const int count = (int)(sizeof(s_driverRetailOffsets) / sizeof(s_driverRetailOffsets[0]));

	for (int i = 0; i < count; i++)
	{
		if (s_driverRetailOffsets[i].retail <= retailOffset)
			best = i;
		else
			break;
	}

	return (int)s_driverRetailOffsets[best].native + (retailOffset - (int)s_driverRetailOffsets[best].retail);
}
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054a08-0x80054a78
u32 UI_VsQuipReadDriver(struct Driver *d, int offset, int size)
{
#if defined(__LP64__)
	offset = UI_VsQuipNativeOffset(offset);
#endif
	char *data = (char *)d + offset;

	if (size == 2)
		return *(s16 *)data;

	if (size < 3)
	{
		if (size == 1)
			return *(u8 *)data;

		return 0;
	}

	if (size == 4)
		return *(u32 *)data;

	return 0;
}
