#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029ab4-0x80029b2c
void howl_ParseCseqHeader(struct CseqHeader *ch)
{
	uintptr_t addr = (uintptr_t)ch;

	sdata->ptrCseqHeader = (struct CseqHeader *)addr;
	addr += sizeof(struct CseqHeader);

	sdata->ptrCseqLongSamples = (struct SampleInstrument *)addr;
	addr += sizeof(struct SampleInstrument) * ch->numLongSamples;

	sdata->ptrCseqShortSamples = (struct SampleDrums *)addr;
	addr += sizeof(struct SampleDrums) * ch->numShortSamples;

	sdata->ptrCseqSongStartOffset = (s16 *)addr;
	addr += sizeof(s16) * ch->numSongs;

	// Align relative to the file start (retail RAM address was 4-aligned), not to the host address.
	addr = (uintptr_t)ch + ((addr - (uintptr_t)ch + 3) & ~(uintptr_t)3);

	sdata->ptrCseqSongData = (char *)addr;
}
