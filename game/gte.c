#include <ctr_gte.h>

void RotateVector(SVec3 *out, const SVec3 *in)
{
#ifndef CTR_NATIVE
	gte_ldv0(in);
	gte_rtv0();
	gte_stlvnl(CTR_SCRATCHPAD_PTR(void, 0));

	out->x = *CTR_SCRATCHPAD_PTR(s16, 0);
	out->y = *CTR_SCRATCHPAD_PTR(s16, 4);
	out->z = *CTR_SCRATCHPAD_PTR(s16, 8);
#endif
}

void MulMatrixVec(Vec3 *out, const Matrix *m, const SVec3 *in)
{
#ifndef CTR_NATIVE
	gte_SetColorMatrix(m);
	gte_ldv0(in);
	gte_lcv0();
	gte_stlvnl(out);
#endif
}
