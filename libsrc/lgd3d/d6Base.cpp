#include <types.h>

static int lgd3d_get_error(DWORD *pdwCode, unsigned long *phResult);

void SetLGD3DErrorCode(DWORD dwCode, long hRes);
const char *GetLgd3dErrorCode(DWORD dwErrorCode);
const char *GetDDErrorMsg(long hRes);
