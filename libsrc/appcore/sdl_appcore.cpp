#include <sdl_appcore.h>

#include <SDL3/SDL.h>
#include <cstdlib>
#include <filesystem>

#include <appagg.h>
#include <platform_services.h>
#include <vkrend.h>

namespace
{
void CloseRenderer()
{
   AutoAppIPtr_(VulkanRenderer,renderer);
   if (renderer)
      renderer->Close();
}
}

cSDLApplication::cSDLApplication(IUnknown *outer, int argumentCount,
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

cSDLApplication::~cSDLApplication()
{
   for (auto *sink : m_sinks)
      if (sink)
         sink->Release();
}

HRESULT cSDLApplication::QueryQuit()
{
   for (auto *sink : m_sinks)
   {
      if (sink->OnQueryQuit() != S_OK)
         return S_FALSE;
   }

   Quit();
   return S_OK;
}

void cSDLApplication::Quit()
{
   for (auto *sink : m_sinks)
      sink->OnQuit();
   CloseRenderer();
   std::exit(PlatformBatchMode() ? PlatformBatchExitCode() : 0);
}

void cSDLApplication::Abort(const char *reason)
{
   PlatformShowError(m_caption.c_str(), reason ? reason : "Dark Engine aborted");
   for (auto *sink : m_sinks)
      sink->OnCriticalError(1);
   CloseRenderer();
   std::exit(1);
}

void cSDLApplication::SetCaption(const char *text)
{
   m_caption = text ? text : "";
}

const char *cSDLApplication::GetCaption()
{
   return m_caption.c_str();
}

void cSDLApplication::SetDefaultFilePath(const char *path)
{
   m_defaultPath = path ? path : ".";
}

const char *cSDLApplication::GetDefaultFilePath()
{
   return m_defaultPath.c_str();
}

const char *cSDLApplication::GetFullName()
{
   return m_fullName.c_str();
}

const char *cSDLApplication::GetPath()
{
   return m_path.c_str();
}

int cSDLApplication::MessageBox(const char *message, const char *caption, int)
{
   PlatformShowError(caption ? caption : m_caption.c_str(), message);
   return 0;
}

HRESULT cSDLApplication::Advise(IAppAdviseSink *sink, DWORD *cookie)
{
   if (!sink || !cookie)
      return E_INVALIDARG;
   sink->AddRef();
   m_sinks.push_back(sink);
   *cookie = static_cast<DWORD>(m_sinks.size());
   return S_OK;
}

HRESULT cSDLApplication::Unadvise(DWORD cookie)
{
   if (!cookie || cookie > m_sinks.size() || !m_sinks[cookie - 1])
      return E_INVALIDARG;
   m_sinks[cookie - 1]->Release();
   m_sinks[cookie - 1] = nullptr;
   return S_OK;
}

void cSDLApplication::AppCommand(unsigned command)
{
   for (auto *sink : m_sinks)
   {
      if (sink && sink->GetVersion() >= kVerAppAdvise_CommandSupport)
         sink->OnCommand(command);
   }
}

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
