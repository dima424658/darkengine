
//#include "lgSS2P.h"

#include "d6States.h"

#include <types.h>

unsigned long dwErrorCode;
unsigned long hD3DError;

static BOOL lgd3d_get_error(DWORD* pdwCode, DWORD* phResult)
{
	if (!dwErrorCode)
		return FALSE;

	*pdwCode = dwErrorCode;
	*phResult = hD3DError;

	return TRUE;
}

void SetLGD3DErrorCode(DWORD dwCode, long hRes)
{
	dwErrorCode = dwCode;
	hD3DError = hRes;
}

// делаешь совместимость с компилятором от 22 студии? к чему тогда такая точность реверса??
const char *GetLgd3dErrorCode(DWORD dwErrorCode)
{
  switch (dwErrorCode)
    {
    case LGD3D_EC_DD_KAPUT:
        return (char*)"Could not obtain DirectDraw from display";
    case LGD3D_EC_RESTORE_ALL_SURFS:
        return (char*)"Could not restore all surfaces";
    case LGD3D_EC_QUERY_D3D:
        return (char*)"QueryInterface for D3D failed";
    case LGD3D_EC_GET_DD_CAPS:
        return (char*)" GetCaps for DirectDraw failed";
    case LGD3D_EC_ZBUFF_ENUMERATION:
        return (char*)" Could not enumerate Zbuffer formats";
    case LGD3D_EC_CREATE_3D_DEVICE:
        return (char*)"Create D3D device failed";
    case LGD3D_EC_CREATE_VIEWPORT:
        return (char*)"Creation of thr viewport failed";
    case LGD3D_EC_ADD_VIEWPORT:
        return (char*)" Addition of the viewport failed";
    case LGD3D_EC_SET_VIEWPORT:
        return (char*)"Setting of the viewport2 failed";
    case LGD3D_EC_SET_CURR_VP:
        return (char*)"Setting of current viewport failed";
    case LGD3D_EC_CREATE_BK_MATERIAL:
        return (char*)"Creation of the background material failed";
    case LGD3D_EC_SET_BK_MATERIAL:
        return (char*)"Setting of the background material failed";
    case LGD3D_EC_GET_BK_MAT_HANDLE:
        return (char*)"Could not obtain a material handle";
    case LGD3D_EC_GET_SURF_DESC:
        return (char*)"Could not get surface description";
    case LGD3D_EC_GET_3D_CAPS:
        return (char*)"Could not get D3D device caps";
    case LGD3D_EC_VD_MPASS_MT:
        return (char*)"ValidateDevice for multipass lightmaps failed";
    case LGD3D_EC_VD_S_DEFAULT:
        return (char*)"ValidateDevice for simple states failed";
    case LGD3D_EC_VD_SPASS_MT:
        return (char*)"ValidateDevice for single pass lightmaps failed";
    case LGD3D_EC_VD_M_DEFAULT:
        return (char*)"ValidateDevice for multitexture states failed";
    default:
        return (char*)"Unknown error";
    }
}

const char *GetDDErrorMsg(long hRes)
{
    // dx7 error list
}
