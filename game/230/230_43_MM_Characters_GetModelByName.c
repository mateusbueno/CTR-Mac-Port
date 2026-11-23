#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800adb64-0x800adc0c.
// Search for character model by string,
// specific to main menu lev, altered in oxide mod
#ifdef CTR_NATIVE
extern uintptr_t g_menuOxideModelRaw;
struct Model *LOAD_NativeModel_ConvertPublic(void *rawModelPtr);
#endif

struct Model *MM_Characters_GetModelByName(int *name)
{
	struct Model **models;
	struct Model *model;
	struct Level *level1 = sdata->gGT->level1;

	// if LEV is invalid
	if (level1 == NULL)
		return NULL;

	models = level1->ptrModelsPtrArray;
	if (models == NULL)
		return NULL;

	// loop through all models in array
	// of model pointers, until nullptr
	for (model = models[0]; model != NULL; models++, model = models[0])
	{
		int *modelName = (int *)model;

		if ((modelName[0] == name[0]) && (modelName[1] == name[1]) && (modelName[2] == name[2]) && (modelName[3] == name[3]))
		{
			// found it
			return model;
		}
	}

#ifdef CTR_NATIVE
	// N. Oxide has no preview model in the menu LEV: use his racer model, loaded with the menu
	if (g_menuOxideModelRaw != 0)
	{
		struct Model *oxide = LOAD_NativeModel_ConvertPublic((void *)g_menuOxideModelRaw);

		if ((oxide != NULL) && (strncmp(oxide->name, (const char *)name, 16) == 0))
			return oxide;
	}
#endif

	return NULL;
}
