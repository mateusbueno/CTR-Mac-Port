#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002cf28-0x8002d0f8
void Voiceline_StartPlay(struct Item *voiceLine)
{
	struct Voiceline *vl = (struct Voiceline *)voiceLine;
	u32 voiceID = (u16)vl->voiceID;
	u32 characterID = (u8)vl->characterID_1;
	u32 voiceSetIndex;

	sdata->backupVoiceline = *vl;

	if (((s32)sdata->gGT->gameMode1 < 0) && ((u32)(voiceID - 10) < 6) && (((u32)(characterID - 8) < 4) || (characterID == 0xf)))
	{
		u32 rng = Voiceline_RequestPlay_NextAudioRNG();
		voiceSetIndex = (rng & 3) + 4;
	}
	else
	{
		voiceSetIndex = data.voiceID[(s16)voiceID];
	}

	s16 *voiceIDs = data.voiceData[characterID].voiceSet[voiceSetIndex].ptr;
	u16 numVoiceIDs = data.voiceData[characterID].voiceSet[voiceSetIndex].num;

	if (numVoiceIDs == 0)
	{
		Voiceline_StopAll();
		return;
	}

	u32 rng = Voiceline_RequestPlay_NextAudioRNG();
	u32 voiceIndex = (rng % numVoiceIDs) * 2;
	u32 xaID = *(u16 *)((u8 *)voiceIDs + voiceIndex);

	if (CDSYS_XAPlay(CDSYS_XA_TYPE_GAME, xaID) == 0)
	{
		sdata->voicelineCooldown = 0x1e;
		return;
	}

	sdata->voicelineCooldown = (s16)(CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_GAME, xaID) / 5) + 0x1e;
}
