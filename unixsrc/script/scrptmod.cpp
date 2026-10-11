#include <scrptman.h>
#include <allocapi.h>
#include <filespec.h>
#include <osm_emu/osm32mod.h>

#pragma pack(push, 8)
#include <dlfcn.h>
#include <string>
#pragma pack(pop)

BOOL cScriptMan::LoadModule(const cFileSpec &module, sScrModuleInfo *info)
{
    LGALLOC_AUTO_CREDIT();
    info->hModule = nullptr;
    info->pModule = nullptr;

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
