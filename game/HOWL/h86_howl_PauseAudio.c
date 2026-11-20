#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002c510-0x8002c64c
void howl_PauseAudio()
{
	u32 *ptrFlag;
	struct ChannelStats *curr, *backupNext;
	struct ChannelStats *pausedStats;

	CDSYS_XAPauseRequest();

	// if already paused, quit
	if (sdata->numBackup_ChannelStats != 0)
		return;

	pausedStats = &sdata->channelStatsCurr[0];

	CseqMusic_Pause();

	Smart_EnterCriticalSection();
	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		ptrFlag = &sdata->ChannelUpdateFlags[curr->channelID];
		*ptrFlag |= 1;
		*ptrFlag &= ~(2);

		*pausedStats++ = *curr;

		LIST_RemoveMember(&sdata->channelTaken, (struct Item *)curr);
		LIST_AddBack(&sdata->channelFree, (struct Item *)curr);

		sdata->numBackup_ChannelStats++;
	}
	Smart_ExitCriticalSection();
}
