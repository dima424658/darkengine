///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/codewrit.cpp $
// $Author: TOML $
// $Date: 1996/11/02 15:26:58 $
// $Revision: 1.1 $
//

#ifdef _WIN32

#include <windows.h>
#include <lg.h>
#include <codewrit.h>

///////////////////////////////////////
//
// MakeAllCodeWritable - This function gets the base address of the PE header,
// takes the BaseOfCode and SizeOfCode members of the OptionalHeader, and
// makes them all PAGE_READWRITE.
// GetModuleHandle returns the base address of our exe image.
// Note, on Win32s, GetModuleHandle doesn't return the base, we need to call
// undocumented APIs to convert it.
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
    BYTE *pBaseOfImage = 0;

    if ((GetVersion() & 0xC0000000) == 0x80000000)
    {
        // We're on Win32s, so get the real pointer
        HMODULE Win32sKernel = GetModuleHandle("W32SKRNL.DLL");

        typedef DWORD __stdcall translator( DWORD );
        translator *pImteFromHModule =
            (translator *)GetProcAddress(Win32sKernel,"_ImteFromHModule@4");
        translator *pBaseAddrFromImte =
            (translator *)GetProcAddress(Win32sKernel,"_BaseAddrFromImte@4");

        if(pImteFromHModule && pBaseAddrFromImte)
        {
            DWORD Imte = (*pImteFromHModule)((DWORD)OurModule);
            pBaseOfImage = (BYTE *)(*pBaseAddrFromImte)(Imte);
        }
    }
    else
    {
        pBaseOfImage = (BYTE *)OurModule;
    }

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
