#include <common.h>

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b4fe4-0x800b5090.

void RB_Default_LInB(struct Instance *inst)
{
	u8 *scratch = CTR_SCRATCHPAD_BASE;
	struct ScratchpadStruct *sps = (struct ScratchpadStruct *)&scratch[0x118];

	int var;

	// high-LOD coll (8 triangles)
	sps->Union.QuadBlockColl.searchFlags = 2;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	SPS_SET_MESHINFO(sps, sdata->gGT->level1->ptr_mesh_info);

	// Make a hitbox
	var = inst->matrix.t[0];
	*(s16 *)&scratch[0x108] = var;
	*(s16 *)&scratch[0x110] = var;
	var = inst->matrix.t[2];
	*(s16 *)&scratch[0x10c] = var;
	*(s16 *)&scratch[0x114] = var;
	var = inst->matrix.t[1];
	*(s16 *)&scratch[0x10a] = var - 0x180;
	*(s16 *)&scratch[0x112] = var + 0x80;

	COLL_SearchBSP_CallbackQUADBLK((u32 *)&scratch[0x108], (u32 *)&scratch[0x110], sps, 0);

	RB_MakeInstanceReflective((struct ScratchpadStruct *)&scratch[0x118], inst);
}
