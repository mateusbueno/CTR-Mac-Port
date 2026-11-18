#include <common.h>

struct LngFile
{
	int numStrings;
	int offsetToPtrArr;
	char strings[1];
};

// param_1 - Pointer to "cd position of bigfile"
// param_2 - language index - 0 ja, 1 en, 2 en2, 3 fr, 4 de, 5 it, 6 es, 7 ne
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032b50-0x80032c24
void LOAD_LangFile(struct BigHeader *bigfilePtr, int lang)
{
	struct LngFile *lngFile;
	int size;

	int i;
	int numStrings;
	char **strArray;

#if BUILD == EurRetail
	// This is to turn the screen black for a bit (optional)
	CTR_ErrorScreen(0, 0, 0);
	VSync(0);
#endif

	if (sdata->lngFile == 0)
	{
		sdata->lngFile = MEMPACK_AllocMem(sdata->langBufferSize /* "lang buffer" */);
	}

	lngFile = sdata->lngFile;

	lngFile = LOAD_ReadFile_ex(bigfilePtr, LT_SETADDR, BI_LANGUAGEFILE + lang, lngFile, &size, NULL);
	if (lngFile == NULL)
		return;

	numStrings = lngFile->numStrings;
	sdata->numLngStrings = numStrings;

	// In 32-bit PS1, the file contained a table of 32-bit offsets in-place.
	// On 64-bit, char* is 8 bytes, so we must maintain a separate 64-bit pointer table.
	static char **s_nativeLngStrings = NULL;
	static int s_nativeLngCapacity = 0;

	if (s_nativeLngCapacity < numStrings)
	{
		s_nativeLngStrings = (char **)realloc(s_nativeLngStrings, numStrings * sizeof(char *));
		s_nativeLngCapacity = numStrings;
	}

	const unsigned int *offsets = (const unsigned int *)((uintptr_t)lngFile + lngFile->offsetToPtrArr);
	for (i = 0; i < numStrings; i++)
	{
		s_nativeLngStrings[i] = (char *)((uintptr_t)lngFile + offsets[i]);
	}

	sdata->lngStrings = s_nativeLngStrings;
#if BUILD == EurRetail
	// set voicelines to new lang
	CDSYS_SetXAToLang(lang);
#endif
}
