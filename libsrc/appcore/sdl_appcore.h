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
   HRESULT QueryQuit() override;
   void Quit() override;
   void Abort(const char *reason) override;
   void SetCaption(const char *text) override;
   const char *GetCaption() override;
   void SetDefaultFilePath(const char *path) override;
   const char *GetDefaultFilePath() override;
   const char *GetFullName() override;
   const char *GetPath() override;
   int MessageBox(const char *message, const char *caption, int) override;
   HRESULT Advise(IAppAdviseSink *sink, DWORD *cookie) override;
   HRESULT Unadvise(DWORD cookie) override;
   void AppCommand(unsigned command) override;

private:
   std::string m_caption;
   std::string m_defaultPath;
   std::string m_fullName;
   std::string m_path;
   std::vector<IAppAdviseSink *> m_sinks;
};

#endif
