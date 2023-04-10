///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/getlgvid.cpp $
// $Author: TOML $
// $Date: 1996/10/23 09:03:51 $
// $Revision: 1.2 $
//

#include <windows.h>
#include <lg.h>
#include <filespec.h>
#include <filepath.h>
#include <getlgvid.h>

///////////////////////////////////////////////////////////////////////////////

static
long RegCreateAndSet(HKEY hKey,
                     const char * pszSubkey,
                     const char * pszValueName,
                     DWORD fdwType,
                     const void * pData,
                     DWORD cbData )
{
    HKEY    hNewKey;
    DWORD   dwIgnored;
    long    result = E_FAIL;

    if (RegCreateKeyEx(hKey, pszSubkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hNewKey, &dwIgnored) == S_OK)
    {
        if (fdwType == REG_SZ)
            cbData = strlen((const char *) pData) + 1;

        if (RegSetValueEx(hNewKey, pszValueName, 0, fdwType, (const BYTE *)pData, cbData) == S_OK)
            {
            result = S_OK;
            }
        RegCloseKey(hKey);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

struct sRegEntryToClean
{
    HKEY hKey;
    const char * pszSubKey;
};

static sRegEntryToClean regCleanEntries[] =
{
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input\\Types\\{73646976-0000-0010-8000-00AA00389B71}\\{00000000-0000-0000-0000-000000000000}" },
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input\\Types\\{73646976-0000-0010-8000-00AA00389B71}" },
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input\\Types" },
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input" },
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins" },
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\InprocServer32" },
    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}" },
    { HKEY_CLASSES_ROOT, "Filter\\{9f711c60-0668-11d0-94d4-0000c02ba972}" }
};

///////////////////////////////////////

struct sRegEntryToAdd
{
    HKEY hKey;
    const char * pszSubKey;
    const char * pszValueName;
    DWORD fdwType;
    const void * pData;
    DWORD cbData;
};

static DWORD dwMerit = 0x00400000;
static DWORD dwTrue = 1;
static DWORD dwFalse = 0;

static sRegEntryToAdd regAddEntries[] =
{
    { HKEY_CLASSES_ROOT, "Filter\\{9f711c60-0668-11d0-94d4-0000c02ba972}",
      "Note", REG_SZ, "This registry key should be deleted", 0 },

    { HKEY_CLASSES_ROOT, "Filter\\{9f711c60-0668-11d0-94d4-0000c02ba972}",
      NULL, REG_SZ, "LG Video Renderer", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}",
      NULL, REG_SZ, "LG Video Renderer", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}",
      "Note", REG_SZ, "This registry key should be deleted", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}",
      "Merit", REG_DWORD, &dwMerit, sizeof(DWORD) },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\InprocServer32",
      "ThreadingModel", REG_SZ, "Both", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins",
      NULL, REG_SZ, "", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input",
      "AllowedMany", REG_DWORD, &dwFalse, sizeof(DWORD) },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input",
      "AllowedZero", REG_DWORD, &dwFalse, sizeof(DWORD) },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input",
      "Direction", REG_DWORD, &dwFalse, sizeof(DWORD) },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input",
      "IsRendered", REG_DWORD, &dwTrue, sizeof(DWORD) },


    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input\\Types",
      NULL, REG_SZ, "", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input\\Types\\{73646976-0000-0010-8000-00AA00389B71}",
      NULL, REG_SZ, "", 0 },

    { HKEY_CLASSES_ROOT, "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\Pins\\Input\\Types\\{73646976-0000-0010-8000-00AA00389B71}\\{00000000-0000-0000-0000-000000000000}",
      NULL, REG_SZ, "", 0 }

};

///////////////////////////////////////////////////////////////////////////////

static void CleanLGVidRegistryEntries()
{
    for (int i = 0; i < sizeof(regCleanEntries) / sizeof(sRegEntryToClean); i++)
        RegDeleteKey(regCleanEntries[i].hKey, regCleanEntries[i].pszSubKey);
}



static HRESULT SetLGVidRegistryEntries(const char * pszFilterFilename)
{
    CleanLGVidRegistryEntries();

    for (int i = 0; i < sizeof(regAddEntries) / sizeof(sRegEntryToAdd); i++)
    {
        if (RegCreateAndSet(regAddEntries[i].hKey,
                            regAddEntries[i].pszSubKey,
                            regAddEntries[i].pszValueName,
                            regAddEntries[i].fdwType,
                            regAddEntries[i].pData,
                            regAddEntries[i].cbData) != S_OK)
            return E_FAIL;
    }

    if (RegCreateAndSet(HKEY_CLASSES_ROOT,
                        "CLSID\\{9f711c60-0668-11d0-94d4-0000c02ba972}\\InprocServer32",
                        NULL,
                        REG_SZ,
                        pszFilterFilename,
                        0) != S_OK)
        return E_FAIL;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

static const char * pszVideoRendererName = "lgvid.ax";

///////////////////////////////////////

HRESULT LGAPI GetLGVideoRenderer()
{
    char      buf[MAX_PATH];
    cFileSpec rendererSpec(pszVideoRendererName);

    rendererSpec.SetFilePath(".\\");
    rendererSpec.MakeFullPath();
    if (rendererSpec.FileExists())
        return SetLGVidRegistryEntries(rendererSpec.GetName());

    GetSystemDirectory(buf, sizeof(buf));
    rendererSpec.SetFilePath(buf);
    rendererSpec.MakeFullPath();
    if (rendererSpec.FileExists())
        return SetLGVidRegistryEntries(rendererSpec.GetName());

    GetWindowsDirectory(buf, sizeof(buf));
    rendererSpec.SetFilePath(buf);
    rendererSpec.MakeFullPath();
    if (rendererSpec.FileExists())
        return SetLGVidRegistryEntries(rendererSpec.GetName());

    rendererSpec.SetFilePath("x:\\prj\\tech\\bin\\");
    rendererSpec.MakeFullPath();
    if (rendererSpec.FileExists())
        return SetLGVidRegistryEntries(rendererSpec.GetName());

    return E_FAIL;
}

///////////////////////////////////////

HRESULT LGAPI ReleaseLGVideoRenderer()
{
    CleanLGVidRegistryEntries();
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
