#pragma pack(push, 8)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#pragma pack(pop)

#include <aggmemb.h>
#include <appagg.h>
#include <appapi.h>

class cUnixApplication : public IApplication
{
   DECLARE_SIMPLE_AGGREGATION(cUnixApplication);
public:
   cUnixApplication(IUnknown *outer, const char *name, const char *defaultPath)
      : m_caption(name ? name : "Dark Engine"), m_defaultPath(defaultPath ? defaultPath : ".")
   {
      char path[4096]; ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
      if (count > 0) { path[count] = 0; m_fullName = path; }
      else m_fullName = "Thief2";
      std::string::size_type slash = m_fullName.rfind('/');
      m_path = slash == std::string::npos ? "." : m_fullName.substr(0, slash);
      INIT_AGGREGATION_1(outer, IID_IApplication, this, kPriorityReserved, nullptr);
   }
   ~cUnixApplication() { for (auto *sink : m_sinks) if (sink) sink->Release(); }

   HRESULT QueryQuit() override
   {
      for (auto *sink : m_sinks) if (sink->OnQueryQuit() != S_OK) return S_FALSE;
      Quit(); return S_OK;
   }
   void Quit() override
   {
      for (auto *sink : m_sinks) sink->OnQuit();
      std::exit(0);
   }
   void Abort(const char *reason) override
   {
      std::fprintf(stderr, "%s\n", reason ? reason : "Dark Engine aborted");
      for (auto *sink : m_sinks) sink->OnCriticalError(1);
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
      std::fprintf(stderr, "%s: %s\n", caption ? caption : m_caption.c_str(), message ? message : "");
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
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cUnixApplication);

extern "C" tResult LGAPI _GenericApplicationCreate(REFIID, IApplication **,
   IUnknown *outer, int, const char *[], const char *name, const char *defaultPath)
{
   USE_EXTENDED_START_UP();
   return new cUnixApplication(outer, name, defaultPath) ? S_OK : E_FAIL;
}
