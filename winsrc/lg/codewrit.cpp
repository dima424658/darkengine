///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/codewrit.cpp $
// $Author: TOML $
// $Date: 1996/11/02 15:26:58 $
// $Revision: 1.1 $
//

#ifdef _WIN32

#include <win32_platform.h>
#include <lg.h>
#include <codewrit.h>

///////////////////////////////////////
//
// MakeAllCodeWritable - This function gets the base address of the PE header,
// takes the BaseOfCode and SizeOfCode members of the OptionalHeader, and
// makes them all PAGE_READWRITE.
// GetModuleHandle returns the base address of our exe image.
//

BOOL LGAPI MakeFunctionWritable(void * pfnFunction, unsigned sizeFunction)
{
    DWORD oldRights;
    return VirtualProtect(pfnFunction, sizeFunction, PAGE_EXECUTE_READWRITE, &oldRights);
}

BOOL LGAPI MakeAllCodeWritable(void)
{
    DebugMsg("MakeCodeWritable()");

    int ReturnValue = 0;

    HMODULE OurModule = GetModuleHandle(0);
    BYTE *pBaseOfImage = (BYTE *)OurModule;

    if (pBaseOfImage)
    {
        IMAGE_OPTIONAL_HEADER *pHeader = (IMAGE_OPTIONAL_HEADER *)
                (pBaseOfImage + ((IMAGE_DOS_HEADER *)pBaseOfImage)->e_lfanew +
                sizeof(IMAGE_NT_SIGNATURE) + sizeof(IMAGE_FILE_HEADER));

        DWORD OldRights;

        if (VirtualProtect(pBaseOfImage+pHeader->BaseOfCode,pHeader->SizeOfCode,
                       PAGE_EXECUTE_READWRITE,&OldRights))
        {
            ReturnValue = 1;
        }
    }

    return ReturnValue;
}

#endif
