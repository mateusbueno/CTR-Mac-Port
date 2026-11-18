#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003266c-0x800326b4.
int LOAD_HowlSectorChainEnd()
{
	int *howlChainParams;
	int howlChainState = sdata->howlChainState;
	howlChainParams = sdata->howlChainParams;

	if (howlChainState == -1)
	{
		LOAD_HowlSectorChainStart(s_howlChain_cdlFile ? s_howlChain_cdlFile : (CdlFILE *)(uintptr_t)(u32)howlChainParams[0],
		                          s_howlChain_dst ? s_howlChain_dst : (void *)(uintptr_t)(u32)howlChainParams[1],
		                          howlChainParams[2], howlChainParams[3]);

		return 0;
	}

	return (howlChainState == 0);
}
