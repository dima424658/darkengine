#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <aggmemb.h>
#include <appagg.h>
#include <appapi.h>
#include <platform_services.h>
#include "../lgd3d/runtime.h"

class cSDLApplication : public IApplication
{
   DECLARE_SIMPLE_AGGREGATION(cSDLApplication);

public:
   cSDLApplication(IUnknown *outer, int argumentCount,
      const char *arguments[], const char *name, const char *defaultPath)
      : m_caption(name ? name : "Dark Engine"), m_defaultPath(defaultPath ? defaultPath : ".")
   {
      char executableDirectory[4096]{};
      if (PlatformGetExecutableDirectory(executableDirectory,
            static_cast<int>(sizeof(executableDirectory))))
         m_path = executableDirectory;
      else if (const char *basePath = SDL_GetBasePath())
         m_path = basePath;
      else
         m_path = ".";

      const char *argumentZero = argumentCount > 0 && arguments
         ? arguments[0] : nullptr;
      const std::filesystem::path executableName = argumentZero && *argumentZero
         ? std::filesystem::path(argumentZero).filename()
         : std::filesystem::path("thief-runner");
      m_fullName = (std::filesystem::path(m_path) / executableName).string();
      INIT_AGGREGATION_1(outer, IID_IApplication, this, kPriorityReserved, nullptr);
   }

   ~cSDLApplication()
   {
      for (auto *sink : m_sinks)
         if (sink)
            sink->Release();
   }

   HRESULT QueryQuit() override
   {
      for (auto *sink : m_sinks) if (sink->OnQueryQuit() != S_OK) return S_FALSE;
      Quit(); return S_OK;
   }
   void Quit() override
   {
      for (auto *sink : m_sinks) sink->OnQuit();
      DarkRenderClose();
      std::exit(0);
   }
   void Abort(const char *reason) override
   {
      PlatformShowError(m_caption.c_str(), reason ? reason : "Dark Engine aborted");
      for (auto *sink : m_sinks) sink->OnCriticalError(1);
      DarkRenderClose();
      std::exit(1);
   }
   void SetCaption(const char *text) override { m_caption = text ? text : ""; }
   const char *GetCaption() override { return m_caption.c_str(); }
   void SetDefaultFilePath(const char *path) override { m_defaultPath = path ? path : "."; }
   const char *GetDefaultFilePath() override { return m_defaultPath.c_str(); }
   const char *GetFullName() override { return m_fullName.c_str(); }
   const char *GetPath() override { return m_path.c_str(); }
   int MessageBox(const char *message, const char *caption, int) override
   {
      PlatformShowError(caption ? caption : m_caption.c_str(), message);
      return 0;
   }
   HRESULT Advise(IAppAdviseSink *sink, DWORD *cookie) override
   {
      if (!sink || !cookie) return E_INVALIDARG;
      sink->AddRef(); m_sinks.push_back(sink); *cookie = static_cast<DWORD>(m_sinks.size()); return S_OK;
   }
   HRESULT Unadvise(DWORD cookie) override
   {
      if (!cookie || cookie > m_sinks.size() || !m_sinks[cookie - 1]) return E_INVALIDARG;
      m_sinks[cookie - 1]->Release(); m_sinks[cookie - 1] = nullptr; return S_OK;
   }
   void AppCommand(unsigned command) override
   {
      for (auto *sink : m_sinks) if (sink && sink->GetVersion() >= kVerAppAdvise_CommandSupport) sink->OnCommand(command);
   }
private:
   std::string m_caption, m_defaultPath, m_fullName, m_path;
   std::vector<IAppAdviseSink *> m_sinks;
};
static_assert(alignof(cSDLApplication) >= alignof(std::string),
   "SDL application storage must retain native string alignment");
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cSDLApplication);

extern "C" tResult LGAPI _GenericApplicationCreate(REFIID, IApplication **,
   IUnknown *outer, int argumentCount, const char *arguments[],
   const char *name, const char *defaultPath)
{
   USE_EXTENDED_START_UP();
   return new cSDLApplication(outer, argumentCount, arguments, name,
      defaultPath) ? S_OK : E_FAIL;
}
