#ifndef DARK_SDL_APPCORE_H
#define DARK_SDL_APPCORE_H

#include <string>
#include <vector>

#include <aggmemb.h>
#include <appapi.h>

class cSDLApplication : public IApplication
{
   DECLARE_SIMPLE_AGGREGATION(cSDLApplication);

public:
   cSDLApplication(IUnknown *outer, int argumentCount,
      const char *arguments[], const char *name, const char *defaultPath);
   ~cSDLApplication();
   STDMETHOD(QueryQuit)() override;
   STDMETHOD_(void, Quit)() override;
   STDMETHOD_(void, Abort)(const char *reason) override;
   STDMETHOD_(void, SetCaption)(const char *text) override;
   STDMETHOD_(const char *, GetCaption)() override;
   STDMETHOD_(void, SetDefaultFilePath)(const char *path) override;
   STDMETHOD_(const char *, GetDefaultFilePath)() override;
   STDMETHOD_(const char *, GetFullName)() override;
   STDMETHOD_(const char *, GetPath)() override;
   STDMETHOD_(int, MessageBox)(const char *message, const char *caption, int) override;
   STDMETHOD(Advise)(IAppAdviseSink *sink, DWORD *cookie) override;
   STDMETHOD(Unadvise)(DWORD cookie) override;
   STDMETHOD_(void, AppCommand)(unsigned command) override;

private:
   std::string m_caption;
   std::string m_defaultPath;
   std::string m_fullName;
   std::string m_path;
   std::vector<IAppAdviseSink *> m_sinks;
};

#endif
