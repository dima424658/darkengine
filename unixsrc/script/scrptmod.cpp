#include <scrptman.h>
#include <allocapi.h>
#include <filespec.h>
#include <mprintf.h>
#include <osm_emu/osm32mod.h>
#include <squirrel/squirrelhost.h>

#pragma pack(push, 8)
#include <dlfcn.h>
#include <stdlib.h>
#include <string>
#pragma pack(pop)

BOOL cScriptMan::LoadModule(const cFileSpec &module, sScrModuleInfo *info)
{
    LGALLOC_AUTO_CREDIT();
    info->hModule = nullptr;
    info->pModule = nullptr;

    const char *profile = getenv("DARK_GAME_PROFILE");
    const osm32::eGuestAbi guestAbi =
        osm32::GuestAbiFromProfile(profile);
    if (_stricmp(module.GetFileName(), "squirrel.osm") == 0)
    {
#ifdef DARK_ENABLE_SQUIRREL
        // NewDark's squirrel.osm is a PE32 module. Android uses the native,
        // engine-owned provider so its loose sq_scripts sources can run
        // without crossing the legacy x86 module ABI.
        if (CreateSquirrelScriptModule(module.GetFileName(), this, m_pfnPrint,
                                       &info->pModule))
            return TRUE;
        mprintf("Built-in Squirrel 3.2 provider failed to initialize\n");
#else
        mprintf("Squirrel script module requested, but this build has Squirrel support disabled\n");
#endif
        return FALSE;
    }

    info->hModule = dlopen(module.GetName(), RTLD_NOW | RTLD_LOCAL);
    if (info->hModule)
    {
        auto init = reinterpret_cast<tScriptModuleInitFunc>(
            dlsym(info->hModule, "ScriptModuleInit"));
        if (init && init(module.GetFileName(), this, m_pfnPrint, g_pMalloc,
                         &info->pModule))
            return TRUE;
        dlclose(info->hModule);
        info->hModule = nullptr;
    }

    std::string error;
    return LoadOsm32Module(module.GetName(), module.GetFileName(), this,
                           guestAbi,
                           &info->pModule, &error) ? TRUE : FALSE;
}

void cScriptMan::FreeModule(sScrModuleInfo *info)
{
    SafeRelease(info->pModule);
    if (info->hModule)
    {
        dlclose(info->hModule);
        info->hModule = nullptr;
    }
}
